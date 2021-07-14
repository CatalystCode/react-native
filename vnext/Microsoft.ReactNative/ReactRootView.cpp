// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ReactRootView.h"
#include "ReactRootView.g.cpp"

#include <ReactHost/MsoUtils.h>
#include <UI.Xaml.Input.h>
#include <UI.Xaml.Media.Media3D.h>
#include <Utils/Helpers.h>
#include <dispatchQueue/dispatchQueue.h>
#include <winrt/Windows.UI.Core.h>
#include "DynamicWriter.h"
#include "ReactNativeHost.h"
#include "ReactViewInstance.h"

#include <winrt/Microsoft.UI.Xaml.Controls.h>

#ifdef USE_FABRIC
#include <Fabric/FabricUIManagerModule.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/core/LayoutContext.h>
#endif

namespace winrt::Microsoft::ReactNative::implementation {

ReactRootView::ReactRootView() noexcept : m_uiQueue(Mso::DispatchQueue::GetCurrentUIThreadQueue()) {
  VerifyElseCrashSz(m_uiQueue, "Cannot get UI dispatch queue for the current thread");

  m_xamlRootView = winrt::Grid{};
  Children().Append(m_xamlRootView);

  UpdatePerspective();
  Loaded([this](auto &&, auto &&) {
    ::Microsoft::ReactNative::SetCompositor(::Microsoft::ReactNative::GetCompositor(*this));
  });
}

ReactNative::ReactNativeHost ReactRootView::ReactNativeHost() noexcept {
  return m_reactNativeHost;
}

void ReactRootView::ReactNativeHost(ReactNative::ReactNativeHost const &value) noexcept {
  if (m_reactNativeHost != value) {
    ReactViewHost(nullptr);
    m_reactNativeHost = value;
    ReloadView();
  }
}

winrt::hstring ReactRootView::ComponentName() noexcept {
  return m_componentName;
}

void ReactRootView::ComponentName(winrt::hstring const &value) noexcept {
  if (m_componentName != value) {
    m_componentName = value;
    ReloadView();
  }
}

ReactNative::JSValueArgWriter ReactRootView::InitialProps() noexcept {
  return m_initialPropsWriter;
}

void ReactRootView::InitialProps(ReactNative::JSValueArgWriter const &value) noexcept {
  if (m_initialPropsWriter != value) {
    m_initialPropsWriter = value;
    m_initialProps = DynamicWriter::ToDynamic(m_initialPropsWriter);
    ReloadView();
  }
}

bool ReactRootView::ExperimentalUseFabric() const noexcept {
  return m_useFabric;
}
void ReactRootView::ExperimentalUseFabric(bool value) noexcept {
  if (m_useFabric != value) {
    m_useFabric = value;
    ReloadView();
  }
}

void ReactRootView::ReloadView() noexcept {
  if (m_reactNativeHost && !m_componentName.empty()) {
    Mso::React::ReactViewOptions viewOptions{};
    viewOptions.ComponentName = to_string(m_componentName);
    viewOptions.InitialProps = m_initialProps;
    viewOptions.UseFabric = m_useFabric;
    if (auto reactViewHost = ReactViewHost()) {
      reactViewHost->ReloadViewInstanceWithOptions(std::move(viewOptions));
    } else {
      auto reactNativeHost = winrt::get_self<implementation::ReactNativeHost>(m_reactNativeHost);
      auto newReactViewHost = reactNativeHost->ReactHost()->MakeViewHost(std::move(viewOptions));
      ReactViewHost(newReactViewHost.Get());
    }
  } else {
    ReactViewHost(nullptr);
  }
}

void ReactRootView::UpdatePerspective() {
  // Xaml's default projection in 3D is orthographic (all lines are parallel)
  // However React Native's default projection is a one-point perspective.
  // Set a default perspective projection on the main control to mimic this.
  if (m_isPerspectiveEnabled) {
    auto perspectiveTransform3D = xaml::Media::Media3D::PerspectiveTransform3D();
    perspectiveTransform3D.Depth(850);
    xaml::Media::Media3D::Transform3D t3d(perspectiveTransform3D);
    m_xamlRootView.Transform3D(t3d);
  } else {
    m_xamlRootView.ClearValue(xaml::UIElement::Transform3DProperty());
  }
}

::Microsoft::ReactNative::XamlView ReactRootView::GetXamlView() const noexcept {
  return ::Microsoft::ReactNative::XamlView(*this);
}

std::string ReactRootView::JSComponentName() const noexcept {
  return to_string(m_componentName);
}

int64_t ReactRootView::GetActualHeight() const noexcept {
  return static_cast<int64_t>(m_xamlRootView.ActualHeight());
}

int64_t ReactRootView::GetActualWidth() const noexcept {
  return static_cast<int64_t>(m_xamlRootView.ActualWidth());
}

int64_t ReactRootView::GetTag() const noexcept {
  return m_rootTag;
}

void ReactRootView::SetTag(int64_t tag) noexcept {
  m_rootTag = tag;
}

// Xaml doesn't provide Blur.
// If 'focus safe harbor' exists, make harbor to allow tabstop and focus on
// harbor with ::Pointer otherwise, just changing the FocusState to ::Pointer
// for the element.
void ReactRootView::blur(::Microsoft::ReactNative::XamlView const &xamlView) noexcept {
  EnsureFocusSafeHarbor();
  if (m_focusSafeHarbor) {
    m_focusSafeHarbor.IsTabStop(true);
    xaml::Input::FocusManager::TryFocusAsync(m_focusSafeHarbor, winrt::FocusState::Pointer);
  } else
    xaml::Input::FocusManager::TryFocusAsync(xamlView, winrt::FocusState::Pointer);
}

void ReactRootView::InitRootView(
    Mso::CntPtr<Mso::React::IReactInstance> &&reactInstance,
    Mso::React::ReactViewOptions &&reactViewOptions) noexcept {
  VerifyElseCrash(m_uiQueue.HasThreadAccess());

  if (m_isInitialized) {
    UninitRootView();
  }

  m_reactOptions = std::make_unique<Mso::React::ReactOptions>(reactInstance->Options());
  m_weakReactInstance = Mso::WeakPtr{reactInstance};
  m_context = &reactInstance->GetReactContext();
  m_reactViewOptions = std::make_unique<Mso::React::ReactViewOptions>(std::move(reactViewOptions));

  m_touchEventHandler = std::make_shared<::Microsoft::ReactNative::TouchEventHandler>(
      *m_context, m_reactViewOptions->UseFabric && !reactInstance->Options().UseWebDebugger());
  m_SIPEventHandler = std::make_shared<::Microsoft::ReactNative::SIPEventHandler>(*m_context);
  m_previewKeyboardEventHandlerOnRoot =
      std::make_shared<::Microsoft::ReactNative::PreviewKeyboardEventHandlerOnRoot>(*m_context);

  m_touchEventHandler->AddTouchHandlers(*this);
  m_previewKeyboardEventHandlerOnRoot->hook(*this);
  m_SIPEventHandler->AttachView(*this, /*fireKeyboradEvents:*/ true);

  UpdateRootViewInternal();
  AttachBackHandlers();

  m_isInitialized = true;
}

void ReactRootView::UpdateRootView() noexcept {
  VerifyElseCrash(m_uiQueue.HasThreadAccess());
  VerifyElseCrash(m_isInitialized);
  UpdateRootViewInternal();
}

void ReactRootView::UpdateRootViewInternal() noexcept {
  if (auto reactInstance = m_weakReactInstance.GetStrongPtr()) {
    switch (reactInstance->State()) {
      case Mso::React::ReactInstanceState::Loading:
        ShowInstanceLoading();
        break;
      case Mso::React::ReactInstanceState::WaitingForDebugger:
        ShowInstanceWaiting();
        break;
      case Mso::React::ReactInstanceState::Loaded:
        ShowInstanceLoaded();
        break;
      case Mso::React::ReactInstanceState::HasError:
        ShowInstanceError();
        break;
      default:
        VerifyElseCrashSz(false, "Unexpected value");
    }
  }
}

void ReactRootView::UninitRootView() noexcept {
  if (!m_isInitialized) {
    return;
  }

  if (m_isJSViewAttached) {
    if (auto reactInstance = m_weakReactInstance.GetStrongPtr()) {
      reactInstance->DetachRootView(this, m_reactViewHost->Options().UseFabric);
    }
  }

  if (m_touchEventHandler != nullptr) {
    m_touchEventHandler->RemoveTouchHandlers();
  }

  if (!m_previewKeyboardEventHandlerOnRoot) {
    m_previewKeyboardEventHandlerOnRoot->unhook();
  }

  RemoveBackHandlers();

  // Clear members with a dependency on the reactInstance
  m_touchEventHandler.reset();
  m_SIPEventHandler.reset();

  m_rootTag = -1;
  m_reactOptions = nullptr;
  m_context.Clear();
  m_reactViewOptions = nullptr;
  m_weakReactInstance = nullptr;

  m_isInitialized = false;
}

void ReactRootView::ClearLoadingUI() noexcept {
  if (m_xamlRootView) {
    auto children = m_xamlRootView.Children();
    uint32_t index{0};
    if (m_greenBoxGrid && children.IndexOf(m_greenBoxGrid, index)) {
      children.RemoveAt(index);
    }
  }
}

void ReactRootView::EnsureLoadingUI() noexcept {
  if (m_xamlRootView) {
    // Create Grid & TextBlock to hold text
    if (m_waitingTextBlock == nullptr) {
      m_waitingTextBlock = winrt::TextBlock();

      m_greenBoxGrid = winrt::Grid{};
      auto c = xaml::Controls::ColumnDefinition{};
      m_greenBoxGrid.ColumnDefinitions().Append(c);
      c = xaml::Controls::ColumnDefinition{};
      c.Width(xaml::GridLengthHelper::Auto());
      m_greenBoxGrid.ColumnDefinitions().Append(c);
      c = xaml::Controls::ColumnDefinition{};
      c.Width(xaml::GridLengthHelper::Auto());
      m_greenBoxGrid.ColumnDefinitions().Append(c);
      c = xaml::Controls::ColumnDefinition{};
      m_greenBoxGrid.ColumnDefinitions().Append(c);

      m_waitingTextBlock.SetValue(xaml::Controls::Grid::ColumnProperty(), winrt::box_value(1));
      m_greenBoxGrid.Background(xaml::Media::SolidColorBrush(winrt::ColorHelper::FromArgb(0x80, 0x03, 0x29, 0x29)));
      m_greenBoxGrid.Children().Append(m_waitingTextBlock);
      m_greenBoxGrid.VerticalAlignment(xaml::VerticalAlignment::Center);
      Microsoft::UI::Xaml::Controls::ProgressRing ring{};
      ring.SetValue(xaml::Controls::Grid::ColumnProperty(), winrt::box_value(2));
      ring.IsActive(true);
      m_greenBoxGrid.Children().Append(ring);

      // Format TextBlock
      m_waitingTextBlock.TextAlignment(winrt::TextAlignment::Center);
      m_waitingTextBlock.TextWrapping(xaml::TextWrapping::Wrap);
      m_waitingTextBlock.FontFamily(winrt::FontFamily(L"Segoe UI"));
      m_waitingTextBlock.Foreground(xaml::Media::SolidColorBrush(winrt::Colors::White()));
      winrt::Thickness margin = {10.0f, 10.0f, 10.0f, 10.0f};
      m_waitingTextBlock.Margin(margin);
    }

    auto children = m_xamlRootView.Children();
    uint32_t index;
    if (m_greenBoxGrid && !children.IndexOf(m_greenBoxGrid, index)) {
      children.Append(m_greenBoxGrid);
    }
  }
}

void ReactRootView::ShowInstanceLoaded() noexcept {
  if (m_xamlRootView) {
    ClearLoadingUI();

    if (auto reactInstance = m_weakReactInstance.GetStrongPtr()) {
      reactInstance->AttachMeasuredRootView(
          this, Mso::Copy(m_reactViewOptions->InitialProps), m_reactViewOptions->UseFabric);
    }
    m_isJSViewAttached = true;
  }
}

void ReactRootView::ShowInstanceError() noexcept {
  if (m_xamlRootView) {
    ClearLoadingUI();
  }
}

void ReactRootView::ShowInstanceWaiting() noexcept {
  if (m_xamlRootView) {
    EnsureLoadingUI();

    // Place message into TextBlock
    std::wstring wstrMessage(L"Connecting to remote debugger");
    m_waitingTextBlock.Text(wstrMessage);
  }
}

void ReactRootView::ShowInstanceLoading() noexcept {
  if (!m_context->SettingsSnapshot().UseDeveloperSupport())
    return;

  if (m_xamlRootView) {
    EnsureLoadingUI();

    // Place message into TextBlock
    std::wstring wstrMessage(L"Loading bundle.");
    m_waitingTextBlock.Text(wstrMessage);
  }
}

void ReactRootView::EnsureFocusSafeHarbor() noexcept {
  if (!m_focusSafeHarbor) {
    // focus safe harbor is delayed to be inserted to the visual tree
    VerifyElseCrash(Children().Size() == 1);

    m_focusSafeHarbor = xaml::Controls::ContentControl{};
    m_focusSafeHarbor.Width(0.0);
    m_focusSafeHarbor.IsTabStop(false);
    Children().InsertAt(0, m_focusSafeHarbor);

    m_focusSafeHarborLosingFocusRevoker = m_focusSafeHarbor.LosingFocus(
        winrt::auto_revoke, [this](const auto & /*sender*/, const winrt::LosingFocusEventArgs & /*args*/) {
          m_focusSafeHarbor.IsTabStop(false);
        });
  }
}

void ReactRootView::AttachBackHandlers() noexcept {
  /*
   * If we are running in a Xaml Island or some other environment where the SystemNavigationManager is unavailable,
   * we should just skip hooking up the BackButton handler. SystemNavigationManager->GetForCurrentView seems to
   * crash with XamlIslands so we can't just bail if that call fails.
   */
  if (::Microsoft::ReactNative::IsXamlIsland())
    return;

  auto weakThis = this->get_weak();
  m_backRequestedRevoker = winrt::Windows::UI::Core::SystemNavigationManager::GetForCurrentView().BackRequested(
      winrt::auto_revoke,
      [weakThis](winrt::IInspectable const & /*sender*/, winrt::BackRequestedEventArgs const &args) {
        if (auto self = weakThis.get()) {
          args.Handled(self->OnBackRequested());
        }
      });

  // In addition to handling the BackRequested event, UWP suggests that we listen for other user inputs that should
  // trigger back navigation that don't fire that event:
  // https://docs.microsoft.com/en-us/windows/uwp/design/basics/navigation-history-and-backwards-navigation

  // Handle keyboard "back" button press
  xaml::Input::KeyboardAccelerator goBack{};
  goBack.Key(winrt::Windows::System::VirtualKey::GoBack);
  goBack.Invoked([weakThis](
                     xaml::Input::KeyboardAccelerator const & /*sender*/,
                     xaml::Input::KeyboardAcceleratorInvokedEventArgs const &args) {
    if (auto self = weakThis.get()) {
      args.Handled(self->OnBackRequested());
    }
  });
  KeyboardAccelerators().Append(goBack);

  // Handle Alt+Left keyboard shortcut
  xaml::Input::KeyboardAccelerator altLeft{};
  altLeft.Key(winrt::Windows::System::VirtualKey::Left);
  altLeft.Invoked([weakThis](
                      xaml::Input::KeyboardAccelerator const & /*sender*/,
                      xaml::Input::KeyboardAcceleratorInvokedEventArgs const &args) {
    if (auto self = weakThis.get()) {
      args.Handled(self->OnBackRequested());
    }
  });
  KeyboardAccelerators().Append(altLeft);
  altLeft.Modifiers(winrt::Windows::System::VirtualKeyModifiers::Menu);

  // Hide keyboard accelerator tooltips
  if (::Microsoft::ReactNative::IsRS4OrHigher()) {
    KeyboardAcceleratorPlacementMode(xaml::Input::KeyboardAcceleratorPlacementMode::Hidden);
  }
}

void ReactRootView::RemoveBackHandlers() noexcept {
  m_backRequestedRevoker.revoke();
  KeyboardAccelerators().Clear();
}

bool ReactRootView::OnBackRequested() noexcept {
  if (m_context->State() != Mso::React::ReactInstanceState::Loaded)
    return false;

  m_context->CallJSFunction("RCTDeviceEventEmitter", "emit", folly::dynamic::array("hardwareBackPress"));
  return true;
}

Mso::React::IReactViewHost *ReactRootView::ReactViewHost() noexcept {
  return m_reactViewHost.Get();
}

void ReactRootView::ReactViewHost(Mso::React::IReactViewHost *viewHost) noexcept {
  if (m_reactViewHost.Get() == viewHost) {
    return;
  }

  if (m_reactViewHost) {
    UninitRootView();
    m_reactViewHost->DetachViewInstance();
  }

  m_reactViewHost = viewHost;

  if (m_reactViewHost) {
    auto viewInstance = Mso::Make<::Microsoft::ReactNative::ReactViewInstance>(this->get_weak(), m_uiQueue);
    m_reactViewHost->AttachViewInstance(*viewInstance);
  }
}

Windows::Foundation::Size ReactRootView::MeasureOverride(Windows::Foundation::Size const &availableSize) const {
  Windows::Foundation::Size size{0.0f, 0.0f};

  for (xaml::UIElement child : Children()) {
    child.Measure(availableSize);
    auto desired = child.DesiredSize();
    if (desired.Height > size.Height)
      size.Height = desired.Height;
    if (desired.Width > size.Width)
      size.Width = desired.Width;
  }

#ifdef USE_FABRIC
  if (m_isInitialized && m_useFabric && !Mso::React::ReactOptions::UseWebDebugger(m_context->Properties()) &&
      m_rootTag != -1) {
    if (auto fabricuiManager = ::Microsoft::ReactNative::FabricUIManager::FromProperties(
            winrt::Microsoft::ReactNative::ReactPropertyBag(m_context->Properties()))) {
      facebook::react::LayoutContext context;
      // TODO scaling factor
      context.pointScaleFactor = 1; // pointScaleFactor_;

      facebook::react::LayoutConstraints constraints;
      // TODO should support MinHeight/MinWidth
      constraints.minimumSize.height = static_cast<facebook::react::Float>(0.0f);
      constraints.minimumSize.width = static_cast<facebook::react::Float>(0.0f);

      // TODO should support MaxHeight/MaxWidth props?
      constraints.minimumSize.height = constraints.maximumSize.height =
          static_cast<facebook::react::Float>(availableSize.Height);
      constraints.minimumSize.width = constraints.maximumSize.width =
          static_cast<facebook::react::Float>(availableSize.Width);
      constraints.layoutDirection = FlowDirection() == xaml::FlowDirection::LeftToRight
          ? facebook::react::LayoutDirection::LeftToRight
          : facebook::react::LayoutDirection::RightToLeft;

      auto yogaSize =
          fabricuiManager->measureSurface(static_cast<facebook::react::SurfaceId>(m_rootTag), constraints, context);
      return {std::min(yogaSize.width, availableSize.Width), std::min(yogaSize.height, availableSize.Height)};
    }
  }
#endif

  return size;
}

Windows::Foundation::Size ReactRootView::ArrangeOverride(Windows::Foundation::Size finalSize) const {
  for (xaml::UIElement child : Children()) {
    child.Arrange(winrt::Rect(0, 0, finalSize.Width, finalSize.Height));
  }

#ifdef USE_FABRIC
  if (m_isInitialized && m_useFabric && !Mso::React::ReactOptions::UseWebDebugger(m_context->Properties()) &&
      m_rootTag != -1) {
    if (auto fabricuiManager = ::Microsoft::ReactNative::FabricUIManager::FromProperties(
            winrt::Microsoft::ReactNative::ReactPropertyBag(m_context->Properties()))) {
      facebook::react::LayoutContext context;
      // TODO scaling factor
      context.pointScaleFactor = 1; // pointScaleFactor_;

      facebook::react::LayoutConstraints constraints;
      // TODO should support MinHeight/MinWidth
      constraints.minimumSize.height = static_cast<facebook::react::Float>(0.0f);
      constraints.minimumSize.width = static_cast<facebook::react::Float>(0.0f);

      // TODO should support MaxHeight/MaxWidth props?
      constraints.minimumSize.height = constraints.maximumSize.height =
          static_cast<facebook::react::Float>(finalSize.Height);
      constraints.minimumSize.width = constraints.maximumSize.width =
          static_cast<facebook::react::Float>(finalSize.Width);
      constraints.layoutDirection = FlowDirection() == xaml::FlowDirection::LeftToRight
          ? facebook::react::LayoutDirection::LeftToRight
          : facebook::react::LayoutDirection::RightToLeft;

      fabricuiManager->constraintSurfaceLayout(
          static_cast<facebook::react::SurfaceId>(m_rootTag), constraints, context);
    }
  }
#endif
  return finalSize;
}

} // namespace winrt::Microsoft::ReactNative::implementation
