﻿// WARNING: Please don't edit this file. It was generated by C++/WinRT v1.0.190111.3

#pragma once
#include "winrt/impl/Windows.UI.Xaml.1.h"
#include "winrt/impl/Windows.UI.Xaml.Automation.Provider.1.h"
#include "winrt/impl/Windows.UI.Xaml.Controls.1.h"
#include "winrt/impl/Windows.UI.Xaml.Media.1.h"
#include "winrt/impl/Windows.UI.Composition.1.h"
#include "winrt/impl/Windows.UI.Xaml.Automation.Peers.1.h"
#include "winrt/impl/react.uwp.1.h"

WINRT_EXPORT namespace winrt::react::uwp {

struct AccessibilityInvokeEventHandler : Windows::Foundation::IUnknown
{
    AccessibilityInvokeEventHandler(std::nullptr_t = nullptr) noexcept {}
    template <typename L> AccessibilityInvokeEventHandler(L lambda);
    template <typename F> AccessibilityInvokeEventHandler(F* function);
    template <typename O, typename M> AccessibilityInvokeEventHandler(O* object, M method);
    template <typename O, typename M> AccessibilityInvokeEventHandler(com_ptr<O>&& object, M method);
    template <typename O, typename M> AccessibilityInvokeEventHandler(weak_ref<O>&& object, M method);
    void operator()() const;
};

}

namespace winrt::impl {

}

WINRT_EXPORT namespace winrt::react::uwp {

struct WINRT_EBO DynamicAutomationPeer :
    react::uwp::IDynamicAutomationPeer,
    impl::base<DynamicAutomationPeer, Windows::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer, Windows::UI::Xaml::Automation::Peers::AutomationPeer, Windows::UI::Xaml::DependencyObject>,
    impl::require<DynamicAutomationPeer, Windows::UI::Xaml::Automation::Peers::IAutomationPeer, Windows::UI::Xaml::Automation::Peers::IAutomationPeer2, Windows::UI::Xaml::Automation::Peers::IAutomationPeer3, Windows::UI::Xaml::Automation::Peers::IAutomationPeer4, Windows::UI::Xaml::Automation::Peers::IAutomationPeer5, Windows::UI::Xaml::Automation::Peers::IAutomationPeer6, Windows::UI::Xaml::Automation::Peers::IAutomationPeer7, Windows::UI::Xaml::Automation::Peers::IAutomationPeer8, Windows::UI::Xaml::Automation::Peers::IAutomationPeer9, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides2, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides3, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides4, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides5, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides6, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides8, Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides9, Windows::UI::Xaml::Automation::Peers::IAutomationPeerProtected, Windows::UI::Xaml::Automation::Peers::IFrameworkElementAutomationPeer, Windows::UI::Xaml::Automation::Provider::IInvokeProvider, Windows::UI::Xaml::Automation::Provider::ISelectionItemProvider, Windows::UI::Xaml::Automation::Provider::ISelectionProvider, Windows::UI::Xaml::IDependencyObject, Windows::UI::Xaml::IDependencyObject2>
{
    DynamicAutomationPeer(std::nullptr_t) noexcept {}
    DynamicAutomationPeer(Windows::UI::Xaml::FrameworkElement const& owner);
};

struct WINRT_EBO DynamicAutomationProperties :
    react::uwp::IDynamicAutomationProperties
{
    DynamicAutomationProperties(std::nullptr_t) noexcept {}
    static Windows::UI::Xaml::DependencyProperty AccessibilityRoleProperty();
    static void SetAccessibilityRole(Windows::UI::Xaml::UIElement const& element, react::uwp::AccessibilityRoles const& value);
    static react::uwp::AccessibilityRoles GetAccessibilityRole(Windows::UI::Xaml::UIElement const& element);
    static Windows::UI::Xaml::DependencyProperty AccessibilityStateDisabledProperty();
    static void SetAccessibilityStateDisabled(Windows::UI::Xaml::UIElement const& element, bool value);
    static bool GetAccessibilityStateDisabled(Windows::UI::Xaml::UIElement const& element);
    static Windows::UI::Xaml::DependencyProperty AccessibilityStateSelectedProperty();
    static void SetAccessibilityStateSelected(Windows::UI::Xaml::UIElement const& element, bool value);
    static bool GetAccessibilityStateSelected(Windows::UI::Xaml::UIElement const& element);
    static Windows::UI::Xaml::DependencyProperty AccessibilityInvokeEventHandlerProperty();
    static void SetAccessibilityInvokeEventHandler(Windows::UI::Xaml::UIElement const& element, react::uwp::AccessibilityInvokeEventHandler const& value);
    static react::uwp::AccessibilityInvokeEventHandler GetAccessibilityInvokeEventHandler(Windows::UI::Xaml::UIElement const& element);
};

struct WINRT_EBO ViewControl :
    react::uwp::IViewControl,
    impl::base<ViewControl, Windows::UI::Xaml::Controls::ContentControl, Windows::UI::Xaml::Controls::Control, Windows::UI::Xaml::FrameworkElement, Windows::UI::Xaml::UIElement, Windows::UI::Xaml::DependencyObject>,
    impl::require<ViewControl, Windows::UI::Composition::IAnimationObject, Windows::UI::Composition::IVisualElement, Windows::UI::Xaml::Controls::IContentControl, Windows::UI::Xaml::Controls::IContentControl2, Windows::UI::Xaml::Controls::IContentControlOverrides, Windows::UI::Xaml::Controls::IControl, Windows::UI::Xaml::Controls::IControl2, Windows::UI::Xaml::Controls::IControl3, Windows::UI::Xaml::Controls::IControl4, Windows::UI::Xaml::Controls::IControl5, Windows::UI::Xaml::Controls::IControl7, Windows::UI::Xaml::Controls::IControlOverrides, Windows::UI::Xaml::Controls::IControlOverrides6, Windows::UI::Xaml::Controls::IControlProtected, Windows::UI::Xaml::IDependencyObject, Windows::UI::Xaml::IDependencyObject2, Windows::UI::Xaml::IFrameworkElement, Windows::UI::Xaml::IFrameworkElement2, Windows::UI::Xaml::IFrameworkElement3, Windows::UI::Xaml::IFrameworkElement4, Windows::UI::Xaml::IFrameworkElement6, Windows::UI::Xaml::IFrameworkElement7, Windows::UI::Xaml::IFrameworkElementOverrides, Windows::UI::Xaml::IFrameworkElementOverrides2, Windows::UI::Xaml::IFrameworkElementProtected7, Windows::UI::Xaml::IUIElement, Windows::UI::Xaml::IUIElement10, Windows::UI::Xaml::IUIElement2, Windows::UI::Xaml::IUIElement3, Windows::UI::Xaml::IUIElement4, Windows::UI::Xaml::IUIElement5, Windows::UI::Xaml::IUIElement7, Windows::UI::Xaml::IUIElement8, Windows::UI::Xaml::IUIElement9, Windows::UI::Xaml::IUIElementOverrides, Windows::UI::Xaml::IUIElementOverrides7, Windows::UI::Xaml::IUIElementOverrides8, Windows::UI::Xaml::IUIElementOverrides9>
{
    ViewControl(std::nullptr_t) noexcept {}
    ViewControl();
};

struct WINRT_EBO ViewPanel :
    react::uwp::IViewPanel,
    impl::base<ViewPanel, Windows::UI::Xaml::Controls::Panel, Windows::UI::Xaml::FrameworkElement, Windows::UI::Xaml::UIElement, Windows::UI::Xaml::DependencyObject>,
    impl::require<ViewPanel, Windows::UI::Composition::IAnimationObject, Windows::UI::Composition::IVisualElement, Windows::UI::Xaml::Controls::IPanel, Windows::UI::Xaml::Controls::IPanel2, Windows::UI::Xaml::IDependencyObject, Windows::UI::Xaml::IDependencyObject2, Windows::UI::Xaml::IFrameworkElement, Windows::UI::Xaml::IFrameworkElement2, Windows::UI::Xaml::IFrameworkElement3, Windows::UI::Xaml::IFrameworkElement4, Windows::UI::Xaml::IFrameworkElement6, Windows::UI::Xaml::IFrameworkElement7, Windows::UI::Xaml::IFrameworkElementOverrides, Windows::UI::Xaml::IFrameworkElementOverrides2, Windows::UI::Xaml::IFrameworkElementProtected7, Windows::UI::Xaml::IUIElement, Windows::UI::Xaml::IUIElement10, Windows::UI::Xaml::IUIElement2, Windows::UI::Xaml::IUIElement3, Windows::UI::Xaml::IUIElement4, Windows::UI::Xaml::IUIElement5, Windows::UI::Xaml::IUIElement7, Windows::UI::Xaml::IUIElement8, Windows::UI::Xaml::IUIElement9, Windows::UI::Xaml::IUIElementOverrides, Windows::UI::Xaml::IUIElementOverrides7, Windows::UI::Xaml::IUIElementOverrides8, Windows::UI::Xaml::IUIElementOverrides9>
{
    ViewPanel(std::nullptr_t) noexcept {}
    ViewPanel();
    static Windows::UI::Xaml::DependencyProperty ViewBackgroundProperty();
    static Windows::UI::Xaml::DependencyProperty BorderThicknessProperty();
    static Windows::UI::Xaml::DependencyProperty BorderBrushProperty();
    static Windows::UI::Xaml::DependencyProperty CornerRadiusProperty();
    static Windows::UI::Xaml::DependencyProperty ClipChildrenProperty();
    static Windows::UI::Xaml::DependencyProperty TopProperty();
    static void SetTop(Windows::UI::Xaml::UIElement const& element, double value);
    static double GetTop(Windows::UI::Xaml::UIElement const& element);
    static Windows::UI::Xaml::DependencyProperty LeftProperty();
    static void SetLeft(Windows::UI::Xaml::UIElement const& element, double value);
    static double GetLeft(Windows::UI::Xaml::UIElement const& element);
};

}
