// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "TextInputViewManager.h"

#include "Unicode.h"
#include "Utils/Helpers.h"

#include <UI.Xaml.Controls.Primitives.h>
#include <UI.Xaml.Controls.h>
#include <UI.Xaml.Input.h>
#include <UI.Xaml.Shapes.h>
#include <Utils/PropertyHandlerUtils.h>
#include <Utils/PropertyUtils.h>
#include <Utils/ValueUtils.h>

#include <INativeUIManager.h>
#include <IReactInstance.h>
#include <Modules/NativeUIManager.h>
#include <Modules/PaperUIManagerModule.h>
#include <Views/FrameworkElementTransferProperties.h>

#ifdef USE_WINUI3
namespace winrt::Microsoft::UI::Xaml::Controls {
using IPasswordBox4 = ::xaml::Controls::IPasswordBox;
using ITextBox6 = ::xaml::Controls::ITextBox;
}; // namespace winrt::Microsoft::UI::Xaml::Controls
#endif

namespace winrt {
using namespace xaml;
using namespace xaml::Media;
using namespace xaml::Shapes;
} // namespace winrt

namespace Microsoft::ReactNative {
struct Selection {
  int64_t start = -1;
  int64_t end = -1;
};
} // namespace Microsoft::ReactNative

// Such code is better to move to a seperate parser layer
template <>
struct json_type_traits<Microsoft::ReactNative::Selection> {
  static Microsoft::ReactNative::Selection parseJson(const winrt::Microsoft::ReactNative::JSValue &json) {
    Microsoft::ReactNative::Selection selection;
    for (auto &item : json.AsObject()) {
      if (item.first == "start") {
        auto start = item.second.AsInt64();
        if (start == start)
          selection.start = start;
      } else if (item.first == "end") {
        auto end = item.second.AsInt64();
        if (end == end)
          selection.end = end;
      }
    }
    return selection;
  }
};

static const std::unordered_map<std::string, xaml::Input::InputScopeNameValue> textBoxKeyboardTypeMap = {
    {"default", xaml::Input::InputScopeNameValue::Default},
    {"number-pad", xaml::Input::InputScopeNameValue::TelephoneNumber},
    {"decimal-pad", xaml::Input::InputScopeNameValue::Digits},
    {"email-address", xaml::Input::InputScopeNameValue::EmailNameOrAddress},
    {"phone-pad", xaml::Input::InputScopeNameValue::TelephoneNumber},
    {"numeric", xaml::Input::InputScopeNameValue::Number},
    {"url", xaml::Input::InputScopeNameValue::Url},
    {"web-search", xaml::Input::InputScopeNameValue::Search}};

static const std::unordered_map<std::string, xaml::Input::InputScopeNameValue> passwordBoxKeyboardTypeMap = {
    {"default", xaml::Input::InputScopeNameValue::Password},
    {"numeric", xaml::Input::InputScopeNameValue::NumericPin}};

static xaml::Input::InputScopeNameValue parseKeyboardType(
    const winrt::Microsoft::ReactNative::JSValue &val,
    const bool isTextBox) {
  auto keyboardTypeMap = isTextBox ? textBoxKeyboardTypeMap : passwordBoxKeyboardTypeMap;

  auto iter = keyboardTypeMap.find(val.AsString());

  if (iter != keyboardTypeMap.end()) {
    return iter->second;
  }

  return isTextBox ? xaml::Input::InputScopeNameValue::Default : xaml::Input::InputScopeNameValue::Password;
}

namespace Microsoft::ReactNative {

class TextInputShadowNode : public ShadowNodeBase {
  using Super = ShadowNodeBase;

 public:
  TextInputShadowNode() = default;
  void createView(const winrt::Microsoft::ReactNative::JSValueObject &) override;
  void updateProperties(winrt::Microsoft::ReactNative::JSValueObject &props) override;

  void dispatchCommand(const std::string &commandId, winrt::Microsoft::ReactNative::JSValueArray &&commandArgs)
      override;

  void AddView(ShadowNode & /*child*/, int64_t /*index*/) override {
    // NYI #5689
    YellowBox("Nested elements in TextInput are currently unsupported on Windows");
  }
  void RemoveChildAt(int64_t /*indexToRemove*/) override {
    // NYI #5689
    YellowBox("Nested elements in TextInput are currently unsupported on Windows");
  }
  void ReplaceChild(const XamlView & /*oldChildView*/, const XamlView & /*newChildView*/) override {
    // NYI #5689
    YellowBox("Nested elements in TextInput are currently unsupported on Windows");
  }

  bool ImplementsPadding() override {
    return true;
  }

  virtual bool NeedsForceLayout() override {
    return true;
  }

 private:
  void dispatchTextInputChangeEvent(winrt::hstring newText);
  void registerEvents();
  void registerPreviewKeyDown();
  void HideCaretIfNeeded();
  void setPasswordBoxPlaceholderForeground(
      xaml::Controls::PasswordBox passwordBox,
      const winrt::Microsoft::ReactNative::JSValue &color);
  void SetText(const winrt::Microsoft::ReactNative::JSValue &text);
  void SetSelection(int64_t start, int64_t end);
  winrt::Shape FindCaret(xaml::DependencyObject element);

  bool m_initialUpdateComplete = false;
  bool m_autoFocus = false;
  bool m_shouldClearTextOnFocus = false;
  bool m_shouldSelectTextOnFocus = false;
  bool m_contextMenuHidden = false;
  bool m_hideCaret = false;
  bool m_isTextBox = true;
  bool m_shouldClearTextOnSubmit = false;

  winrt::Microsoft::ReactNative::JSValue m_placeholderTextColor;
  std::vector<HandledKeyboardEvent> m_submitKeyEvents{};

  // Javascripts is running in a different thread. If the typing is very fast,
  // It's possible that two TextChanged are raised but TextInput just got the
  // first response from Javascript. So the first response should be dropped.
  // EventCount is introduced to resolve this problem
  uint32_t m_nativeEventCount{0}; // EventCount to javascript
  uint32_t m_mostRecentEventCount{0}; // EventCount from javascript

 private:
  xaml::Controls::TextBox::TextChanging_revoker m_textBoxTextChangingRevoker{};
  xaml::Controls::TextBox::SelectionChanged_revoker m_textBoxSelectionChangedRevoker{};
  xaml::Controls::TextBox::ContextMenuOpening_revoker m_textBoxContextMenuOpeningRevoker{};

  xaml::Controls::PasswordBox::PasswordChanging_revoker m_passwordBoxPasswordChangingRevoker{};
  xaml::Controls::PasswordBox::PasswordChanged_revoker m_passwordBoxPasswordChangedRevoker{};
  xaml::Controls::PasswordBox::ContextMenuOpening_revoker m_passwordBoxContextMenuOpeningRevoker{};

  xaml::Controls::Control::GotFocus_revoker m_controlGotFocusRevoker{};
  xaml::Controls::Control::LostFocus_revoker m_controlLostFocusRevoker{};
  xaml::Controls::Control::PreviewKeyDown_revoker m_controlPreviewKeyDownRevoker{};
  xaml::Controls::Control::SizeChanged_revoker m_controlSizeChangedRevoker{};
  xaml::Controls::Control::CharacterReceived_revoker m_controlCharacterReceivedRevoker{};
  xaml::Controls::ScrollViewer::ViewChanging_revoker m_scrollViewerViewChangingRevoker{};
  xaml::Controls::Control::Loaded_revoker m_controlLoadedRevoker{};
};

void TextInputShadowNode::createView(const winrt::Microsoft::ReactNative::JSValueObject &props) {
  Super::createView(props);
  registerEvents();
}

void TextInputShadowNode::dispatchTextInputChangeEvent(winrt::hstring newText) {
  if (!m_initialUpdateComplete) {
    return;
  }
  m_nativeEventCount++;
  folly::dynamic eventData =
      folly::dynamic::object("target", m_tag)("text", HstringToDynamic(newText))("eventCount", m_nativeEventCount);
  GetViewManager()->GetReactContext().DispatchEvent(m_tag, "topTextInputChange", std::move(eventData));
}

void TextInputShadowNode::registerEvents() {
  auto control = GetView().as<xaml::Controls::Control>();
  auto tag = m_tag;

  // TextChanged is implemented as async event in Xaml. If Javascript is like
  // this:
  //    onChangeText={text => this.setState({text})}
  // And user type 'AB' very fast, then 'B' is possible to be lost in below
  // situation.
  //    Input 'A' -> TextChanged for 'A' -> Javascript processing 'A' -> Input
  //    becomes 'AB' -> Processing javascript response and set text to 'A'
  //    TextChanged for 'AB' but textbox.Text is 'A' -> Javascript processing
  //    'A'
  //
  // TextChanging is used to drop the Javascript response of 'A' and expect
  // another TextChanged event with correct event count.
  if (m_isTextBox) {
    m_passwordBoxPasswordChangedRevoker = {};
    m_passwordBoxPasswordChangingRevoker = {};
    auto textBox = control.as<xaml::Controls::TextBox>();
    m_textBoxTextChangingRevoker = textBox.TextChanging(
        winrt::auto_revoke, [=](auto &&, auto &&) { dispatchTextInputChangeEvent(textBox.Text()); });
  } else {
    m_textBoxTextChangingRevoker = {};
    auto passwordBox = control.as<xaml::Controls::PasswordBox>();
    if (control.try_as<xaml::Controls::IPasswordBox4>()) {
      m_passwordBoxPasswordChangingRevoker = passwordBox.PasswordChanging(
          winrt::auto_revoke, [=](auto &&, auto &&) { dispatchTextInputChangeEvent(passwordBox.Password()); });
    } else {
      m_passwordBoxPasswordChangedRevoker = passwordBox.PasswordChanged(
          winrt::auto_revoke, [=](auto &&, auto &&) { dispatchTextInputChangeEvent(passwordBox.Password()); });
    }
  }

  if (m_isTextBox) {
    m_passwordBoxContextMenuOpeningRevoker = {};
    auto textBox = control.as<xaml::Controls::TextBox>();
    m_textBoxContextMenuOpeningRevoker =
        textBox.ContextMenuOpening(winrt::auto_revoke, [=](auto &&, xaml::Controls::ContextMenuEventArgs const &e) {
          if (m_contextMenuHidden) {
            e.Handled(true);
          }
        });
  } else {
    m_textBoxContextMenuOpeningRevoker = {};
    auto passwordBox = control.as<xaml::Controls::PasswordBox>();
    m_passwordBoxContextMenuOpeningRevoker =
        passwordBox.ContextMenuOpening(winrt::auto_revoke, [=](auto &&, xaml::Controls::ContextMenuEventArgs const &e) {
          if (m_contextMenuHidden) {
            e.Handled(true);
          }
        });
  }

  m_controlGotFocusRevoker = control.GotFocus(winrt::auto_revoke, [=](auto &&, auto &&) {
    if (m_shouldClearTextOnFocus) {
      if (m_isTextBox) {
        control.as<xaml::Controls::TextBox>().ClearValue(xaml::Controls::TextBox::TextProperty());
      } else {
        control.as<xaml::Controls::PasswordBox>().ClearValue(xaml::Controls::PasswordBox::PasswordProperty());
      }
    }

    if (m_shouldSelectTextOnFocus) {
      if (m_isTextBox) {
        control.as<xaml::Controls::TextBox>().SelectAll();
      } else {
        control.as<xaml::Controls::PasswordBox>().SelectAll();
      }
    }
    HideCaretIfNeeded();

    folly::dynamic eventData = folly::dynamic::object("target", tag);
    if (!m_updating)
      GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputFocus", std::move(eventData));
  });

  m_controlLostFocusRevoker = control.LostFocus(winrt::auto_revoke, [=](auto &&, auto &&) {
    folly::dynamic eventDataBlur = folly::dynamic::object("target", tag);
    folly::dynamic eventDataEndEditing = {};
    if (m_isTextBox) {
      eventDataEndEditing =
          folly::dynamic::object("target", tag)("text", HstringToDynamic(control.as<xaml::Controls::TextBox>().Text()));
    } else {
      eventDataEndEditing = folly::dynamic::object("target", tag)(
          "text", HstringToDynamic(control.as<xaml::Controls::PasswordBox>().Password()));
    }
    if (!m_updating) {
      GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputBlur", std::move(eventDataBlur));
      GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputEndEditing", std::move(eventDataEndEditing));
    }
  });

  registerPreviewKeyDown();

  if (m_isTextBox) {
    auto textBox = control.as<xaml::Controls::TextBox>();
    m_textBoxSelectionChangedRevoker = textBox.SelectionChanged(winrt::auto_revoke, [=](auto &&, auto &&) {
      folly::dynamic selectionData = folly::dynamic::object("start", textBox.SelectionStart())(
          "end", textBox.SelectionStart() + textBox.SelectionLength());
      folly::dynamic eventData = folly::dynamic::object("target", tag)("selection", std::move(selectionData));
      if (!m_updating)
        GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputSelectionChange", std::move(eventData));
    });
  }

  m_controlSizeChangedRevoker =
      control.SizeChanged(winrt::auto_revoke, [=](auto &&, winrt::SizeChangedEventArgs const &args) {
        if (m_isTextBox) {
          if (control.as<xaml::Controls::TextBox>().TextWrapping() == xaml::TextWrapping::Wrap) {
            folly::dynamic contentSizeData =
                folly::dynamic::object("width", args.NewSize().Width)("height", args.NewSize().Height);
            folly::dynamic eventData = folly::dynamic::object("target", tag)("contentSize", std::move(contentSizeData));
            if (!m_updating)
              GetViewManager()->GetReactContext().DispatchEvent(
                  tag, "topTextInputContentSizeChange", std::move(eventData));
          }
        }
      });

  m_controlLoadedRevoker = control.Loaded(winrt::auto_revoke, [=](auto &&, auto &&) {
    if (m_autoFocus) {
      control.Focus(xaml::FocusState::Keyboard);
    }

    auto contentElement = control.GetTemplateChild(L"ContentElement");
    auto textBoxView = contentElement.as<xaml::Controls::ScrollViewer>();
    if (textBoxView) {
      m_scrollViewerViewChangingRevoker = textBoxView.ViewChanging(
          winrt::auto_revoke, [=](auto &&, xaml::Controls::ScrollViewerViewChangingEventArgs const &args) {
            if (!m_updating) {
              folly::dynamic offsetData = folly::dynamic::object("x", args.FinalView().HorizontalOffset())(
                  "y", args.FinalView().VerticalOffset());
              folly::dynamic eventData = folly::dynamic::object("target", tag)("contentOffset", std::move(offsetData));
              GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputScroll", std::move(eventData));
            }
          });
    }
    HideCaretIfNeeded();
  });

  if (control.try_as<xaml::IUIElement7>()) {
    m_controlCharacterReceivedRevoker = control.CharacterReceived(
        winrt::auto_revoke, [=](auto &&, xaml::Input::CharacterReceivedRoutedEventArgs const &args) {
          std::string key;
          wchar_t s[2] = L" ";
          s[0] = args.Character();
          key = Microsoft::Common::Unicode::Utf16ToUtf8(s, 1);

          if (key.compare("\r") == 0) {
            key = "Enter";
          } else if (key.compare("\b") == 0) {
            key = "Backspace";
          }

          if (!m_updating) {
            folly::dynamic eventData = folly::dynamic::object("target", tag)("key", folly::dynamic(key));
            GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputKeyPress", std::move(eventData));
          }
        });
  }

  control.as<xaml::UIElement>().AddHandler(
      xaml::UIElement::PointerPressedEvent(),
      winrt::box_value(xaml::Input::PointerEventHandler([=](auto &&, xaml::Input::PointerRoutedEventArgs const &args) {
        folly::dynamic eventData = folly::dynamic::object("target", tag);
        GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputPressIn", std::move(eventData));
      })),
      true);

  control.as<xaml::UIElement>().AddHandler(
      xaml::UIElement::PointerReleasedEvent(),
      winrt::box_value(xaml::Input::PointerEventHandler([=](auto &&, xaml::Input::PointerRoutedEventArgs const &args) {
        folly::dynamic eventData = folly::dynamic::object("target", tag);
        GetViewManager()->GetReactContext().DispatchEvent(tag, "topTextInputPressOut", std::move(eventData));
      })),
      true);
}

void TextInputShadowNode::registerPreviewKeyDown() {
  auto control = GetView().as<xaml::Controls::Control>();
  auto tag = m_tag;
  m_controlPreviewKeyDownRevoker =
      control.PreviewKeyDown(winrt::auto_revoke, [=](auto &&, xaml::Input::KeyRoutedEventArgs const &args) {
        auto isMultiline = m_isTextBox && control.as<xaml::Controls::TextBox>().AcceptsReturn();
        auto shouldSubmit = !args.Handled();
        if (shouldSubmit) {
          if (!isMultiline && m_submitKeyEvents.size() == 0) {
            // If no 'submitKeyEvents' are supplied, use the default behavior for single-line TextInput
            shouldSubmit = args.Key() == winrt::Windows::System::VirtualKey::Enter;
          } else if (m_submitKeyEvents.size() > 0) {
            // If 'submitKeyEvents' are supplied, use them to determine whether to emit
            // 'onSubmitEditing' for either single-line or multi-line TextInput

            // This must be kept in sync with the default value for HandledKeyboardEvent.handledEventPhase
            auto defaultEventPhase = HandledEventPhase::Bubbling;
            auto currentEvent = KeyboardHelper::CreateKeyboardEvent(defaultEventPhase, args);
            shouldSubmit = KeyboardHelper::ShouldMarkKeyboardHandled(m_submitKeyEvents, currentEvent);
          } else {
            // If no 'submitKeyEvents' are supplied, do not emit 'onSubmitEditing' for multi-line TextInput
            shouldSubmit = false;
          }
        }

        if (shouldSubmit) {
          folly::dynamic eventDataSubmitEditing = {};
          if (m_isTextBox) {
            eventDataSubmitEditing = folly::dynamic::object("target", tag)(
                "text", HstringToDynamic(control.as<xaml::Controls::TextBox>().Text()));
          } else {
            eventDataSubmitEditing = folly::dynamic::object("target", tag)(
                "text", HstringToDynamic(control.as<xaml::Controls::PasswordBox>().Password()));
          }

          GetViewManager()->GetReactContext().DispatchEvent(
              tag, "topTextInputSubmitEditing", std::move(eventDataSubmitEditing));

          if (m_shouldClearTextOnSubmit) {
            if (m_isTextBox) {
              control.as<xaml::Controls::TextBox>().ClearValue(xaml::Controls::TextBox::TextProperty());
            } else {
              control.as<xaml::Controls::PasswordBox>().ClearValue(xaml::Controls::PasswordBox::PasswordProperty());
            }
          }

          // For multi-line TextInput, we have to mark the PreviewKeyDown event as
          // handled to prevent the TextInput from adding a newline character
          if (isMultiline) {
            args.Handled(true);
          }
        }
      });
}

xaml::Shapes::Shape TextInputShadowNode::FindCaret(xaml::DependencyObject element) {
  if (element == nullptr)
    return nullptr;

  if (auto shape = element.try_as<winrt::Shape>())
    return shape;

  int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(element);
  for (int i = 0; i < childrenCount; i++) {
    auto result = FindCaret(winrt::VisualTreeHelper::GetChild(element, i));
    if (result != nullptr)
      return result;
  }

  return nullptr;
}

// hacking solution to hide the caret
void TextInputShadowNode::HideCaretIfNeeded() {
  if (m_hideCaret) {
    auto control = GetView().as<xaml::Controls::Control>();
    if (auto caret = FindCaret(control)) {
      caret.CompositeMode(xaml::Media::ElementCompositeMode::Inherit);
      xaml::Media::SolidColorBrush transparentColor(winrt::Colors::Transparent());
      caret.Fill(transparentColor);
    }
  }
}

void TextInputShadowNode::setPasswordBoxPlaceholderForeground(
    xaml::Controls::PasswordBox passwordBox,
    const winrt::Microsoft::ReactNative::JSValue &color) {
  m_placeholderTextColor = color.Copy();
  auto defaultRD = xaml::ResourceDictionary();
  auto solidColorBrush = ColorFrom(m_placeholderTextColor);
  defaultRD.Insert(winrt::box_value(L"TextControlPlaceholderForeground"), winrt::box_value(solidColorBrush));
  defaultRD.Insert(winrt::box_value(L"TextControlPlaceholderForegroundFocused"), winrt::box_value(solidColorBrush));
  defaultRD.Insert(winrt::box_value(L"TextControlPlaceholderForegroundPointerOver"), winrt::box_value(solidColorBrush));
  auto passwordBoxResource = xaml::ResourceDictionary();
  auto themeDictionaries = passwordBoxResource.ThemeDictionaries();
  themeDictionaries.Insert(winrt::box_value(L"Default"), defaultRD);
  passwordBoxResource.Insert(winrt::box_value(L"ThemeDictionaries"), themeDictionaries);
  passwordBox.Resources(passwordBoxResource);
}

void TextInputShadowNode::updateProperties(winrt::Microsoft::ReactNative::JSValueObject &props) {
  m_updating = true;
  auto control = GetView().as<xaml::Controls::Control>();
  auto textBox = control.try_as<xaml::Controls::TextBox>();
  auto passwordBox = control.try_as<xaml::Controls::PasswordBox>();
  auto hasKeyDownEvents = false;

  for (auto &pair : props) {
    const std::string &propertyName = pair.first;
    const auto &propertyValue = pair.second;

    // Applicable properties for both TextBox and PasswordBox
    if (TryUpdateFontProperties(control, propertyName, propertyValue)) {
      continue;
    } else if (TryUpdateCharacterSpacing(control, propertyName, propertyValue)) {
      continue;
    } else if (propertyName == "allowFontScaling") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
        control.IsTextScaleFactorEnabled(propertyValue.AsBoolean());
      else if (propertyValue.IsNull())
        control.ClearValue(xaml::Controls::Control::IsTextScaleFactorEnabledProperty());
    } else if (propertyName == "clearTextOnFocus") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
        m_shouldClearTextOnFocus = propertyValue.AsBoolean();
    } else if (propertyName == "selectTextOnFocus") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
        m_shouldSelectTextOnFocus = propertyValue.AsBoolean();
    } else if (propertyName == "mostRecentEventCount") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Double ||
          propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Int64) {
        m_mostRecentEventCount = propertyValue.AsInt32();
      }
    } else if (propertyName == "contextMenuHidden") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
        m_contextMenuHidden = propertyValue.AsBoolean();
    } else if (propertyName == "caretHidden") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean) {
        m_hideCaret = propertyValue.AsBoolean();
        HideCaretIfNeeded();
      }
    } else if (propertyName == "secureTextEntry") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean) {
        if (propertyValue.AsBoolean()) {
          if (m_isTextBox) {
            xaml::Controls::PasswordBox newPasswordBox;
            ReparentView(newPasswordBox);
            m_isTextBox = false;
            registerEvents();
            control = newPasswordBox.as<xaml::Controls::Control>();
            passwordBox = newPasswordBox;
            if (!m_placeholderTextColor.IsNull()) {
              setPasswordBoxPlaceholderForeground(newPasswordBox, m_placeholderTextColor);
            }
          }
        } else {
          if (!m_isTextBox) {
            xaml::Controls::TextBox newTextBox;
            ReparentView(newTextBox);
            m_isTextBox = true;
            registerEvents();
            control = newTextBox.as<xaml::Controls::Control>();
            textBox = newTextBox;
            if (!m_placeholderTextColor.IsNull()) {
              textBox.PlaceholderForeground(SolidColorBrushFrom(m_placeholderTextColor));
            }
          }
        }
      }
    } else if (propertyName == "maxLength") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Double ||
          propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Int64) {
        control.SetValue(
            m_isTextBox ? xaml::Controls::TextBox::MaxLengthProperty()
                        : xaml::Controls::PasswordBox::MaxLengthProperty(),
            winrt::PropertyValue::CreateInt32(propertyValue.AsInt32()));
      } else if (propertyValue.IsNull()) {
        control.ClearValue(
            m_isTextBox ? xaml::Controls::TextBox::MaxLengthProperty()
                        : xaml::Controls::PasswordBox::MaxLengthProperty());
      }
    } else if (propertyName == "placeholder") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::String) {
        control.SetValue(
            m_isTextBox ? xaml::Controls::TextBox::PlaceholderTextProperty()
                        : xaml::Controls::PasswordBox::PlaceholderTextProperty(),
            winrt::PropertyValue::CreateString(asHstring(propertyValue)));
      } else if (propertyValue.IsNull()) {
        control.ClearValue(
            m_isTextBox ? xaml::Controls::TextBox::PlaceholderTextProperty()
                        : xaml::Controls::PasswordBox::PlaceholderTextProperty());
      }
    } else if (propertyName == "selectionColor") {
      if (IsValidColorValue(propertyValue)) {
        control.SetValue(
            m_isTextBox ? xaml::Controls::TextBox::SelectionHighlightColorProperty()
                        : xaml::Controls::PasswordBox::SelectionHighlightColorProperty(),
            SolidColorBrushFrom(propertyValue));
      } else if (propertyValue.IsNull())
        control.ClearValue(
            m_isTextBox ? xaml::Controls::TextBox::SelectionHighlightColorProperty()
                        : xaml::Controls::PasswordBox::SelectionHighlightColorProperty());
    } else if (propertyName == "keyboardType") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::String) {
        auto inputScopeNameVaue = parseKeyboardType(propertyValue, m_isTextBox);
        auto scope = xaml::Input::InputScope();
        auto scopeName = xaml::Input::InputScopeName(inputScopeNameVaue);
        auto names = scope.Names();
        names.Append(scopeName);
        control.SetValue(
            m_isTextBox ? xaml::Controls::TextBox::InputScopeProperty()
                        : xaml::Controls::PasswordBox::InputScopeProperty(),
            scope);
      } else if (propertyValue.IsNull())
        control.ClearValue(
            m_isTextBox ? xaml::Controls::TextBox::InputScopeProperty()
                        : xaml::Controls::PasswordBox::InputScopeProperty());
    } else if (propertyName == "placeholderTextColor") {
      m_placeholderTextColor = nullptr;
      if (textBox.try_as<xaml::Controls::ITextBox6>() && m_isTextBox) {
        if (IsValidColorValue(propertyValue)) {
          m_placeholderTextColor = propertyValue.Copy();
          textBox.PlaceholderForeground(SolidColorBrushFrom(propertyValue));
        } else if (propertyValue.IsNull())
          textBox.ClearValue(xaml::Controls::TextBox::PlaceholderForegroundProperty());
      } else if (m_isTextBox != true && IsValidColorValue(propertyValue)) {
        setPasswordBoxPlaceholderForeground(passwordBox, propertyValue);
      }
    } else if (propertyName == "clearTextOnSubmit") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
        m_shouldClearTextOnSubmit = propertyValue.AsBoolean();
    } else if (propertyName == "submitKeyEvents") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Array)
        m_submitKeyEvents = KeyboardHelper::FromJS(propertyValue);
      else if (propertyValue.IsNull())
        m_submitKeyEvents.clear();
    } else if (propertyName == "keyDownEvents") {
      hasKeyDownEvents = propertyValue.ItemCount() > 0;
    } else if (propertyName == "autoFocus") {
      if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
        m_autoFocus = propertyValue.AsBoolean();
    } else {
      if (m_isTextBox) { // Applicable properties for TextBox
        if (TryUpdateTextAlignment(textBox, propertyName, propertyValue)) {
          continue;
        } else if (propertyName == "multiline") {
          if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean) {
            const bool isMultiline = propertyValue.AsBoolean();
            textBox.TextWrapping(isMultiline ? xaml::TextWrapping::Wrap : xaml::TextWrapping::NoWrap);
            textBox.AcceptsReturn(isMultiline);
          } else if (propertyValue.IsNull())
            textBox.ClearValue(xaml::Controls::TextBox::TextWrappingProperty());
        } else if (propertyName == "editable") {
          if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
            textBox.IsReadOnly(!propertyValue.AsBoolean());
          else if (propertyValue.IsNull())
            textBox.ClearValue(xaml::Controls::TextBox::IsReadOnlyProperty());
        } else if (propertyName == "scrollEnabled") {
          if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean &&
              textBox.TextWrapping() == xaml::TextWrapping::Wrap) {
            auto scrollMode =
                propertyValue.AsBoolean() ? xaml::Controls::ScrollMode::Auto : xaml::Controls::ScrollMode::Disabled;
            xaml::Controls::ScrollViewer::SetVerticalScrollMode(textBox, scrollMode);
            xaml::Controls::ScrollViewer::SetHorizontalScrollMode(textBox, scrollMode);
          }
        } else if (propertyName == "selection") {
          if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Object) {
            auto selection = json_type_traits<Selection>::parseJson(propertyValue);
            SetSelection(selection.start, selection.end);
          }
        } else if (propertyName == "spellCheck") {
          if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::Boolean)
            textBox.IsSpellCheckEnabled(propertyValue.AsBoolean());
          else if (propertyValue.IsNull())
            textBox.ClearValue(xaml::Controls::TextBox::IsSpellCheckEnabledProperty());
        } else if (propertyName == "text") {
          SetText(propertyValue);
        } else if (propertyName == "autoCapitalize") {
          if (textBox.try_as<xaml::Controls::ITextBox6>()) {
            if (propertyValue.Type() == winrt::Microsoft::ReactNative::JSValueType::String) {
              if (propertyValue.AsString() == "characters") {
                textBox.CharacterCasing(xaml::Controls::CharacterCasing::Upper);
              } else { // anything else turns off autoCap (should be "None" but
                       // we don't support "words"/"senetences" yet)
                textBox.CharacterCasing(xaml::Controls::CharacterCasing::Normal);
              }
            } else if (propertyValue.IsNull())
              textBox.ClearValue(xaml::Controls::TextBox::CharacterCasingProperty());
          }
        }
      } else { // Applicable properties for PasswordBox
        if (propertyName == "text" && !m_isTextBox) {
          SetText(propertyValue);
        }
      }
    }
  }

  Super::updateProperties(props);

  // We need to re-register the PreviewKeyDown handler so it is invoked after the ShadowNodeBase handler
  if (hasKeyDownEvents) {
    m_controlPreviewKeyDownRevoker.revoke();
    registerPreviewKeyDown();
  }

  m_updating = false;
  m_initialUpdateComplete = true;
}

void TextInputShadowNode::SetText(const winrt::Microsoft::ReactNative::JSValue &text) {
  auto textBox = this->GetView().try_as<xaml::Controls::TextBox>();
  auto passwordBox = this->GetView().try_as<xaml::Controls::PasswordBox>();

  if (m_mostRecentEventCount == m_nativeEventCount) {
    if (textBox) {
      if (text.Type() == winrt::Microsoft::ReactNative::JSValueType::String) {
        auto oldCursor = textBox.SelectionStart();
        auto oldSelectionLength = textBox.SelectionLength();
        auto oldValue = textBox.Text();
        auto newValue = asHstring(text);
        if (oldValue != newValue) {
          textBox.Text(newValue);
          if (oldValue.size() == newValue.size()) {
            textBox.SelectionStart(oldCursor);
          } else {
            textBox.SelectionStart(newValue.size());
          }
        }
      } else if (text.IsNull())
        textBox.ClearValue(xaml::Controls::TextBox::TextProperty());
    } else {
      if (text.Type() == winrt::Microsoft::ReactNative::JSValueType::String) {
        auto oldValue = passwordBox.Password();
        auto newValue = asHstring(text);
        if (oldValue != newValue) {
          passwordBox.Password(newValue);
        }
      } else if (text.IsNull())
        passwordBox.ClearValue(xaml::Controls::PasswordBox::PasswordProperty());
    }
  }
}

void TextInputShadowNode::SetSelection(int64_t start, int64_t end) {
  if (start < 0 || end < 0 || start > end) {
    return;
  }

  if (auto textBox = this->GetView().try_as<xaml::Controls::TextBox>()) {
    textBox.Select(static_cast<int32_t>(start), static_cast<int32_t>(end - start));
  }
}

void TextInputShadowNode::dispatchCommand(
    const std::string &commandId,
    winrt::Microsoft::ReactNative::JSValueArray &&commandArgs) {
  if (commandId == "focus") {
    if (auto uiManager = GetNativeUIManager(GetViewManager()->GetReactContext()).lock()) {
      uiManager->focus(m_tag);
    }
  } else if (commandId == "blur") {
    if (auto uiManager = GetNativeUIManager(GetViewManager()->GetReactContext()).lock()) {
      uiManager->blur(m_tag);
    }
  } else if (commandId == "setTextAndSelection") {
    m_mostRecentEventCount = commandArgs[0].AsInt32();
    SetText(commandArgs[1]);
    SetSelection(commandArgs[2].AsInt64(), commandArgs[3].AsInt64());
  } else {
    Super::dispatchCommand(commandId, std::move(commandArgs));
  }
}

TextInputViewManager::TextInputViewManager(const Mso::React::IReactContext &context) : Super(context) {}

const wchar_t *TextInputViewManager::GetName() const {
  return L"RCTTextInput";
}

void TextInputViewManager::GetNativeProps(const winrt::Microsoft::ReactNative::IJSValueWriter &writer) const {
  Super::GetNativeProps(writer);

  React::WriteProperty(writer, L"allowFontScaling", L"boolean");
  React::WriteProperty(writer, L"clearTextOnFocus", L"boolean");
  React::WriteProperty(writer, L"editable", L"boolean");
  React::WriteProperty(writer, L"maxLength", L"int");
  React::WriteProperty(writer, L"multiline", L"boolean");
  React::WriteProperty(writer, L"placeholder", L"string");
  React::WriteProperty(writer, L"placeholderTextColor", L"Color");
  React::WriteProperty(writer, L"scrollEnabled", L"boolean");
  React::WriteProperty(writer, L"selection", L"Map");
  React::WriteProperty(writer, L"selectionColor", L"Color");
  React::WriteProperty(writer, L"selectTextOnFocus", L"boolean");
  React::WriteProperty(writer, L"spellCheck", L"boolean");
  React::WriteProperty(writer, L"text", L"string");
  React::WriteProperty(writer, L"mostRecentEventCount", L"int");
  React::WriteProperty(writer, L"secureTextEntry", L"boolean");
  React::WriteProperty(writer, L"keyboardType", L"string");
  React::WriteProperty(writer, L"contextMenuHidden", L"boolean");
  React::WriteProperty(writer, L"caretHidden", L"boolean");
  React::WriteProperty(writer, L"autoCapitalize", L"string");
  React::WriteProperty(writer, L"clearTextOnSubmit", L"boolean");
  React::WriteProperty(writer, L"submitKeyEvents", L"array");
  React::WriteProperty(writer, L"autoFocus", L"boolean");
}

void TextInputViewManager::GetExportedCustomDirectEventTypeConstants(
    const winrt::Microsoft::ReactNative::IJSValueWriter &writer) const {
  Super::GetExportedCustomDirectEventTypeConstants(writer);

  const PCWSTR eventNames[] = {// Generic events
                               L"Change",
                               L"Focus",
                               L"Blur",
                               L"EndEditing",
                               L"SelectionChange",
                               L"ContentSizeChange",
                               L"KeyPress",
                               L"PressIn",
                               L"PressOut",
                               L"Scroll",
                               L"SubmitEditing"};

  for (auto &eventBaseName : eventNames) {
    using namespace std::string_literals;
    writer.WritePropertyName(L"topTextInput"s + eventBaseName);
    writer.WriteObjectBegin();
    writer.WritePropertyName(L"registrationName");
    writer.WriteString(L"on"s + eventBaseName);
    writer.WriteObjectEnd();
  }
}

ShadowNode *TextInputViewManager::createShadow() const {
  return new TextInputShadowNode();
}

XamlView TextInputViewManager::CreateViewCore(int64_t /*tag*/, const winrt::Microsoft::ReactNative::JSValueObject &) {
  xaml::Controls::TextBox textBox;
  // This works around a XAML Islands bug where the XamlRoot of the first
  // window the flyout is shown on takes ownership of the flyout and attempts
  // to show the flyout on other windows cause the first window to get focus.
  // https://github.com/microsoft/microsoft-ui-xaml/issues/5341
  if (IsXamlIsland() &&
      winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
          L"Windows.Foundation.UniversalApiContract", 7)) {
    xaml::Controls::TextCommandBarFlyout flyout;
    flyout.Placement(xaml::Controls::Primitives::FlyoutPlacementMode::BottomEdgeAlignedLeft);
    textBox.ContextFlyout(flyout);
    textBox.SelectionFlyout(flyout);
  }
  return textBox;
}

YGMeasureFunc TextInputViewManager::GetYogaCustomMeasureFunc() const {
  return DefaultYogaSelfMeasureFunc;
}

void TextInputViewManager::TransferInputScope(
    const XamlView &oldView,
    const XamlView &newView,
    const bool copyToPasswordBox) {
  // transfer input scope, only common keyboardType between secureTextEntry
  // on/off is numeric, so only need to transfer input scope "Number" <=>
  // "NumericPin", everything else leave it as default.
  xaml::Input::InputScope inputScope;
  if (copyToPasswordBox) {
    inputScope = oldView.try_as<xaml::Controls::TextBox>().InputScope();
  } else {
    inputScope = oldView.try_as<xaml::Controls::PasswordBox>().InputScope();
  }

  if (inputScope != nullptr) {
    auto nameValue = inputScope.Names().GetAt(0).NameValue();

    if ((nameValue == xaml::Input::InputScopeNameValue::Number && copyToPasswordBox) ||
        (nameValue == xaml::Input::InputScopeNameValue::NumericPin && !copyToPasswordBox)) {
      auto newScope = xaml::Input::InputScope();
      auto scopeName = xaml::Input::InputScopeName(
          copyToPasswordBox ? xaml::Input::InputScopeNameValue::NumericPin : xaml::Input::InputScopeNameValue::Number);
      auto names = newScope.Names();
      names.Append(scopeName);
      newView.SetValue(
          copyToPasswordBox ? xaml::Controls::PasswordBox::InputScopeProperty()
                            : xaml::Controls::TextBox::InputScopeProperty(),
          newScope);
    }
  }
}

void TextInputViewManager::TransferProperties(const XamlView &oldView, const XamlView &newView) {
  if ((oldView.try_as<xaml::Controls::TextBox>() != nullptr &&
       newView.try_as<xaml::Controls::PasswordBox>() != nullptr) ||
      (oldView.try_as<xaml::Controls::PasswordBox>() != nullptr &&
       newView.try_as<xaml::Controls::TextBox>() != nullptr)) {
    bool copyToPasswordBox = oldView.try_as<xaml::Controls::TextBox>() != nullptr;

    // sync common properties between TextBox and PasswordBox
    if (copyToPasswordBox) {
      TransferProperty(
          oldView,
          newView,
          xaml::Controls::TextBox::MaxLengthProperty(),
          xaml::Controls::PasswordBox::MaxLengthProperty());
      TransferProperty(
          oldView,
          newView,
          xaml::Controls::TextBox::PlaceholderTextProperty(),
          xaml::Controls::PasswordBox::PlaceholderTextProperty());
      TransferProperty(
          oldView,
          newView,
          xaml::Controls::TextBox::SelectionHighlightColorProperty(),
          xaml::Controls::PasswordBox::SelectionHighlightColorProperty());
      newView.as<xaml::Controls::PasswordBox>().Password(oldView.as<xaml::Controls::TextBox>().Text());
    } else {
      TransferProperty(
          oldView,
          newView,
          xaml::Controls::PasswordBox::MaxLengthProperty(),
          xaml::Controls::TextBox::MaxLengthProperty());
      TransferProperty(
          oldView,
          newView,
          xaml::Controls::PasswordBox::PlaceholderTextProperty(),
          xaml::Controls::TextBox::PlaceholderTextProperty());
      TransferProperty(
          oldView,
          newView,
          xaml::Controls::PasswordBox::SelectionHighlightColorProperty(),
          xaml::Controls::TextBox::SelectionHighlightColorProperty());
      newView.as<xaml::Controls::TextBox>().Text(oldView.as<xaml::Controls::PasswordBox>().Password());
    }

    TransferInputScope(oldView, newView, copyToPasswordBox);
    // Set focus if current control has focus
    auto focusState = oldView.as<xaml::Controls::Control>().FocusState();
    if (focusState != winrt::FocusState::Unfocused) {
      newView.as<xaml::Controls::Control>().Focus(focusState);
    }
  }
  Super::TransferProperties(oldView, newView);
}
} // namespace Microsoft::ReactNative
