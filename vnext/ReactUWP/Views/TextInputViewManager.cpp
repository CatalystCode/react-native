// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include "TextInputViewManager.h"

#include "unicode.h"

#include <Utils/PropertyHandlerUtils.h>
#include <Utils/PropertyUtils.h>
#include <Utils/ValueUtils.h>

#include <IReactInstance.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Input.h>

namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
}

namespace react {
  namespace uwp {
    struct Selection
    {
      int start = -1;
      int end = -1;
      bool isValid();
    };

    bool Selection::isValid() {
      if (start < 0)
        return false;
      if (end < 0)
        return false;
      if (start > end)
        return false;
      return true;
    }
} }

// Such code is better to move to a seperate parser layer
template<>
struct json_type_traits<react::uwp::Selection>
{
  static react::uwp::Selection parseJson(const folly::dynamic& json)
  {
    react::uwp::Selection selection;
    for (auto& item : json.items())
    {
      if (item.first == "start")
      {
        auto start = item.second.asInt();
        if (start == static_cast<int>(start))
          selection.start = static_cast<int>(start);
      }
      else if (item.first == "end")
      {
        auto end = item.second.asInt();
        if (end == static_cast<int>(end))
          selection.end = static_cast<int>(end);
      }
    }
    return selection;
  }
};

namespace react { namespace uwp {

class TextInputShadowNode : public ShadowNodeBase
{
  using Super = ShadowNodeBase;
public:
  TextInputShadowNode() = default;
  void createView() override;
  void updateProperties(const folly::dynamic&& props) override;

  bool ImplementsPadding() override { return true; }

private:
  bool m_shouldClearTextOnFocus = false;
  bool m_shouldSelectTextOnFocus = false;
};

enum class TextInputCommands
{
  SetFocus = 1,
  Blur = 2,
};

void TextInputShadowNode::createView()
{
  Super::createView();

  auto textBox = GetView().as<winrt::TextBox>();
  auto wkinstance = GetViewManager()->GetReactInstance();
  auto tag = m_tag;
  textBox.TextChanged([=](auto &&, auto &&)
  {
    auto instance = wkinstance.lock();
    folly::dynamic eventData = folly::dynamic::object("target", tag)("text", HstringToDynamic(textBox.Text()));
    if (!m_updating && instance != nullptr)
      instance->DispatchEvent(tag, "topTextInputChange", std::move(eventData));
  });

  textBox.GotFocus([=](auto &&, auto &&)
  {
    if (m_shouldClearTextOnFocus)
      textBox.ClearValue(winrt::TextBox::TextProperty());

    if (m_shouldSelectTextOnFocus)
      textBox.SelectAll();

    auto instance = wkinstance.lock();
    folly::dynamic eventData = folly::dynamic::object("target", tag);
    if (!m_updating && instance != nullptr)
      instance->DispatchEvent(tag, "topTextInputFocus", std::move(eventData));
  });

  textBox.LostFocus([=](auto &&, auto &&)
  {
    auto instance = wkinstance.lock();
    folly::dynamic eventDataBlur = folly::dynamic::object("target", tag);
    folly::dynamic eventDataEndEditing = folly::dynamic::object("target", tag)("text", HstringToDynamic(textBox.Text()));
    if (!m_updating && instance != nullptr)
    {
      instance->DispatchEvent(tag, "topTextInputBlur", std::move(eventDataBlur));
      instance->DispatchEvent(tag, "topTextInputEndEditing", std::move(eventDataEndEditing));
    }
  });

  textBox.SelectionChanged([=](auto &&, auto &&)
  {
    auto instance = wkinstance.lock();
    folly::dynamic selectionData = folly::dynamic::object("start", textBox.SelectionStart())("end", textBox.SelectionStart() + textBox.SelectionLength());
    folly::dynamic eventData = folly::dynamic::object("target", tag)("selection", std::move(selectionData));
    if (!m_updating && instance != nullptr)
      instance->DispatchEvent(tag, "topTextInputSelectionChange", std::move(eventData));
  });

  textBox.SizeChanged([=](auto &&, winrt::SizeChangedEventArgs const& args)
  {
    if (textBox.TextWrapping() == winrt::TextWrapping::Wrap)
    {
      auto instance = wkinstance.lock();
      folly::dynamic contentSizeData = folly::dynamic::object("width", args.NewSize().Width)("height", args.NewSize().Height);
      folly::dynamic eventData = folly::dynamic::object("target", tag)("contentSize", std::move(contentSizeData));
      if (!m_updating && instance != nullptr)
        instance->DispatchEvent(tag, "topTextInputContentSizeChange", std::move(eventData));
    }
  });

  if (textBox.ApplyTemplate()) {
    textBox.GetTemplateChild(asHstring("ContentElement")).as<winrt::ScrollViewer>().ViewChanging([=](auto&&, winrt::ScrollViewerViewChangingEventArgs const& args) {
      auto instance = wkinstance.lock();
      if (!m_updating && instance != nullptr) {
        folly::dynamic offsetData = folly::dynamic::object("x", args.FinalView().HorizontalOffset())("y", args.FinalView().VerticalOffset());
        folly::dynamic eventData = folly::dynamic::object("target", tag)("contentOffset", std::move(offsetData));
        instance->DispatchEvent(tag, "topTextInputOnScroll", std::move(eventData));
      }
    });
  }

  textBox.CharacterReceived([=](auto&&, winrt::CharacterReceivedRoutedEventArgs const& args) {
    auto instance = wkinstance.lock();
    std::string key;
    wchar_t s[2] = L" ";
    s[0] = args.Character();
    key = facebook::react::unicode::utf16ToUtf8(s, 1);

    if (key.compare("\r") == 0) {
      key = "Enter";
    }
    else if (key.compare("\b") == 0) {
      key = "Backspace";
    }

    if (!m_updating && instance != nullptr) {
      folly::dynamic eventData = folly::dynamic::object("target", tag)("key", folly::dynamic(key));
      instance->DispatchEvent(tag, "topTextInputKeyPress", std::move(eventData));
    }
  });
}
void TextInputShadowNode::updateProperties(const folly::dynamic&& props)
{
  m_updating = true;
  auto textBox = GetView().as<winrt::TextBox>();
  if (textBox == nullptr)
    return;

  auto control = textBox.as<winrt::Control>();
  for (auto& pair : props.items())
  {
    if (TryUpdateFontProperties(control, pair.first, pair.second))
    {
      continue;
    }
    else if (TryUpdateTextAlignment(textBox, pair.first, pair.second))
    {
      continue;
    }
    else if (TryUpdateCharacterSpacing(control, pair.first, pair.second))
    {
      continue;
    }
    else if (pair.first == "multiline")
    {
      if (pair.second.isBool())
        textBox.TextWrapping(pair.second.asBool() ? winrt::TextWrapping::Wrap : winrt::TextWrapping::NoWrap);
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::TextWrappingProperty());
    }
    else if (pair.first == "allowFontScaling")
    {
      if (pair.second.isBool())
        textBox.IsTextScaleFactorEnabled(pair.second.asBool());
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::Control::IsTextScaleFactorEnabledProperty());
    }
    else if (pair.first == "clearTextOnFocus")
    {
      if (pair.second.isBool())
        m_shouldClearTextOnFocus = pair.second.asBool();
    }
    else if (pair.first == "editable")
    {
      if (pair.second.isBool())
        textBox.IsReadOnly(!pair.second.asBool());
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::IsReadOnlyProperty());
    }
    else if (pair.first == "maxLength")
    {
      if (pair.second.isInt())
        textBox.MaxLength(static_cast<int32_t>(pair.second.asInt()));
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::MaxLengthProperty());
    }
    else if (pair.first == "placeholder")
    {
      if (pair.second.isString())
        textBox.PlaceholderText(asHstring(pair.second));
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::PlaceholderTextProperty());
    }
    else if (pair.first == "placeholderTextColor")
    {
      if (pair.second.isInt())
        textBox.PlaceholderForeground(SolidColorBrushFrom(pair.second));
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::PlaceholderForegroundProperty());
    }
    else if (pair.first == "scrollEnabled")
    {
      if (pair.second.isBool() && textBox.TextWrapping() == winrt::TextWrapping::Wrap)
      {
        auto scrollMode = pair.second.asBool() ? winrt::ScrollMode::Auto : winrt::ScrollMode::Disabled;
        winrt::ScrollViewer::SetVerticalScrollMode(textBox, scrollMode);
        winrt::ScrollViewer::SetHorizontalScrollMode(textBox, scrollMode);
      }
    }
    else if (pair.first == "selection")
    {
      if (pair.second.isObject())
      {
        auto selection = json_type_traits<Selection>::parseJson(pair.second);

        if (selection.isValid())
          textBox.Select(selection.start, selection.end - selection.start);
      }
    }
    else if (pair.first == "selectionColor")
    {
      if (pair.second.isInt())
        textBox.SelectionHighlightColor(SolidColorBrushFrom(pair.second));
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::SelectionHighlightColorProperty());
    }
    else if (pair.first == "selectTextOnFocus")
    {
      if (pair.second.isBool())
        m_shouldSelectTextOnFocus = pair.second.asBool();
    }
    else if (pair.first == "spellCheck")
    {
      if (pair.second.isBool())
        textBox.IsSpellCheckEnabled(pair.second.asBool());
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::IsSpellCheckEnabledProperty());
    }
    else if (pair.first == "text")
    {
      if (pair.second.isString())
        textBox.Text(asHstring(pair.second));
      else if (pair.second.isNull())
        textBox.ClearValue(winrt::TextBox::TextProperty());
    }
  }

  Super::updateProperties(std::move(props));
  m_updating = false;
}

TextInputViewManager::TextInputViewManager(const std::shared_ptr<IReactInstance>& reactInstance)
  : Super(reactInstance)
{
}

const char* TextInputViewManager::GetName() const
{
  return "RCTTextInput";
}

folly::dynamic TextInputViewManager::GetCommands() const
{
  auto commands = Super::GetCommands();
  commands.update(folly::dynamic::object
    ("SetFocus", static_cast<std::underlying_type_t<TextInputCommands>>(TextInputCommands::SetFocus))
  );
  commands.update(folly::dynamic::object
    ("Blur", static_cast<std::underlying_type_t<TextInputCommands>>(TextInputCommands::Blur))
  );
  return commands;
}

folly::dynamic TextInputViewManager::GetNativeProps() const
{
  auto props = Super::GetNativeProps();

  props.update(folly::dynamic::object
    ("allowFontScaling", "boolean")
    ("clearTextOnFocus", "boolean")
    ("editable", "boolean")
    ("maxLength", "int")
    ("multiline", "boolean")
    ("placeholder", "string")
    ("placeholderTextColor", "Color")
    ("scrollEnabled", "boolean")
    ("selection", "Map")
    ("selectionColor", "Color")
    ("selectTextOnFocus", "boolean")
    ("spellCheck", "boolean")
    ("text", "string")
  );

  return props;
}

folly::dynamic TextInputViewManager::GetExportedCustomDirectEventTypeConstants() const
{
  auto directEvents = Super::GetExportedCustomDirectEventTypeConstants();
  directEvents["topTextInputChange"] = folly::dynamic::object("registrationName", "onChange");
  directEvents["topTextInputFocus"] = folly::dynamic::object("registrationName", "onFocus");
  directEvents["topTextInputBlur"] = folly::dynamic::object("registrationName", "onBlur");
  directEvents["topTextInputEndEditing"] = folly::dynamic::object("registrationName", "onEndEditing");
  directEvents["topTextInputSelectionChange"] = folly::dynamic::object("registrationName", "onSelectionChange");
  directEvents["topTextInputContentSizeChange"] = folly::dynamic::object("registrationName", "onContentSizeChange");
  directEvents["topTextInputKeyPress"] = folly::dynamic::object("registrationName", "onKeyPress");
  directEvents["topTextInputOnScroll"] = folly::dynamic::object("registrationName", "onScroll");

  return directEvents;
}

facebook::react::ShadowNode* TextInputViewManager::createShadow() const
{
  return new TextInputShadowNode();
}

XamlView TextInputViewManager::CreateViewCore(int64_t tag)
{
  winrt::TextBox textBox;
  return textBox;
}

void TextInputViewManager::DispatchCommand(XamlView viewToUpdate, int64_t commandId, const folly::dynamic& commandArgs)
{
  auto textBox = viewToUpdate.as<winrt::TextBox>();
  if (textBox == nullptr)
    return;

  switch (static_cast<TextInputCommands>(commandId))
  {
    case TextInputCommands::SetFocus:
    {
      textBox.Focus(winrt::FocusState::Programmatic);
      break;
    }

    case TextInputCommands::Blur:
    {
      auto content = winrt::Windows::UI::Xaml::Window::Current().Content();
      if (content == nullptr)
        break;

      auto frame = content.try_as<winrt::Windows::UI::Xaml::Controls::Frame>();
      if (frame != nullptr)
        frame.Focus(winrt::FocusState::Programmatic);
      break;
    }
  }
}

YGMeasureFunc TextInputViewManager::GetYogaCustomMeasureFunc() const
{
  return DefaultYogaSelfMeasureFunc;
}

} }
