// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include "MenuFlyoutViewManager.h"
#include "MenuFlyoutItemViewManager.h"

#include <Views/ShadowNodeBase.h>
#include "TouchEventHandler.h"
#include "ViewPanel.h"
#include <Utils/ValueUtils.h>

#include <Modules/NativeUIManager.h>
#include <Utils/PropertyHandlerUtils.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>

namespace winrt {
  using namespace Windows::UI::Xaml::Controls::Primitives;
  using namespace Windows::UI::Xaml::Interop;
}

template<>
struct json_type_traits<winrt::FlyoutPlacementMode>
{
  static winrt::FlyoutPlacementMode parseJson(const folly::dynamic& json)
  {
    winrt::FlyoutPlacementMode placement;
    if (json == "top")
    {
      placement = winrt::FlyoutPlacementMode::Top;
    }
    else if (json == "bottom")
    {
      placement = winrt::FlyoutPlacementMode::Bottom;
    }
    else if (json == "left")
    {
      placement = winrt::FlyoutPlacementMode::Left;
    }
    else if (json == "right")
    {
      placement = winrt::FlyoutPlacementMode::Right;
    }
    else if (json == "top-edge-aligned-left")
    {
      placement = winrt::FlyoutPlacementMode::TopEdgeAlignedLeft;
    }
    else if (json == "top-edge-aligned-right")
    {
      placement = winrt::FlyoutPlacementMode::TopEdgeAlignedRight;
    }
    else if (json == "bottom-edge-aligned-left")
    {
      placement = winrt::FlyoutPlacementMode::BottomEdgeAlignedLeft;
    }
    else if (json == "bottom-edge-aligned-right")
    {
      placement = winrt::FlyoutPlacementMode::BottomEdgeAlignedRight;
    }
    else if (json == "left-edge-aligned-top")
    {
      placement = winrt::FlyoutPlacementMode::LeftEdgeAlignedTop;
    }
    else if (json == "right-edge-aligned-top")
    {
      placement = winrt::FlyoutPlacementMode::RightEdgeAlignedTop;
    }
    else if (json == "left-bottom")
    {
      placement = winrt::FlyoutPlacementMode::LeftEdgeAlignedBottom;
    }
    else if (json == "right-edge-aligned-bottom")
    {
      placement = winrt::FlyoutPlacementMode::RightEdgeAlignedBottom;
    }
    else if (json == "full")
    {
      placement = winrt::FlyoutPlacementMode::Full;
    }
    else
    {
    placement = winrt::FlyoutPlacementMode::Top;
    }

    return placement;
  }
};

namespace react { namespace uwp {

class MenuFlyoutShadowNode : public ShadowNodeBase
{
  using Super = ShadowNodeBase;
public:
  MenuFlyoutShadowNode() = default;
  virtual ~MenuFlyoutShadowNode();

  void AddView(ShadowNode& child, int64_t index) override;
  void createView() override;
  static void OnMenuFlyoutClosed(IReactInstance& instance, int64_t tag, bool newValue);
  void onDropViewInstance() override;
  void removeAllChildren() override;
  void updateProperties(const folly::dynamic&& props) override;
  
private:
  void SetTargetFrameworkElement(bool updateContextFlyout);
  void AdjustDefaultFlyoutStyle();
  winrt::Popup GetMenuFlyoutParentPopup() const;

  winrt::FrameworkElement m_targetElement = nullptr;
  winrt::MenuFlyout m_menuFlyout = nullptr;

  bool m_isLightDismissEnabled = true;
  bool m_isOpen = false;
  bool m_attachAsContextFlyout = false;
  int64_t m_targetTag = -1;
  float m_horizontalOffset = 0;
  float m_verticalOffset = 0;
  bool m_isFlyoutShowOptionsSupported = false; // not sure if needed
  winrt::FlyoutShowOptions m_showOptions = nullptr;

  std::shared_ptr<TouchEventHandler> m_touchEventHanadler;
};

MenuFlyoutShadowNode::~MenuFlyoutShadowNode()
{
  m_touchEventHanadler->RemoveTouchHandlers();
}

void MenuFlyoutShadowNode::AddView(ShadowNode& child, int64_t index)
{

  auto childView = static_cast<ShadowNodeBase&>(child).GetView();
  m_touchEventHanadler->AddTouchHandlers(childView);

  if (m_menuFlyout != nullptr)
    m_menuFlyout.Items().Append(childView.as<winrt::MenuFlyoutItem>());

}

void MenuFlyoutShadowNode::createView()
{
  Super::createView();

  m_menuFlyout = winrt::MenuFlyout();
  m_isFlyoutShowOptionsSupported = !!(m_menuFlyout.try_as<winrt::IFlyoutBase5>());

  if (m_isFlyoutShowOptionsSupported)
    m_showOptions = winrt::FlyoutShowOptions();

  auto wkinstance = GetViewManager()->GetReactInstance();
  m_touchEventHanadler = std::make_shared<TouchEventHandler>(wkinstance);

  
  // cancel closing if lighDismiss is false
  m_menuFlyout.Closing([=](winrt::FlyoutBase /*flyoutbase*/, winrt::FlyoutBaseClosingEventArgs args)
  {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr && !m_isLightDismissEnabled && m_isOpen)
    {
      args.Cancel(true);
    }
  });

  // register the close callback function to the Closed event handler
  m_menuFlyout.Closed([=](auto&&, auto&&)
  {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr)
      OnMenuFlyoutClosed(*instance, m_tag, false);
  });

  // Set XamlRoot on the Flyout to handle XamlIsland/AppWindow scenarios.
  if (auto flyoutBase6 = m_menuFlyout.try_as<winrt::IFlyoutBase6>())
  {
    if (auto instance = wkinstance.lock())
    {
      if (auto xamlRoot = static_cast<NativeUIManager*>(instance->NativeUIManager())->tryGetXamlRoot())
      {
        flyoutBase6.XamlRoot(xamlRoot);
      }
    }
  }
}

/*static*/ void MenuFlyoutShadowNode::OnMenuFlyoutClosed(IReactInstance& instance, int64_t tag, bool newValue)
{
  folly::dynamic eventData = folly::dynamic::object("target", tag)("isOpen", newValue);
  instance.DispatchEvent(tag, "topDismiss", std::move(eventData));
}

void MenuFlyoutShadowNode::onDropViewInstance() {
  m_isOpen = false;
  m_menuFlyout.Hide();
}

void MenuFlyoutShadowNode::removeAllChildren()
{
  m_menuFlyout.Items().Clear();
}

void MenuFlyoutShadowNode::updateProperties(const folly::dynamic&& props)
{
  m_updating = true;
  bool updateTargetElement = false;
  bool updateIsOpen = false;
  bool updateOffset = false;
  bool updateContextFlyout = false;

  if (m_menuFlyout == nullptr)
    return;

  for (auto& pair : props.items())
  {
    const std::string& propertyName = pair.first.getString();
    const folly::dynamic& propertyValue = pair.second;

    if (propertyName == "horizontalOffset")
    {
      if (propertyValue.isNumber())
        m_horizontalOffset = static_cast<float>(propertyValue.asDouble());
      else
        m_horizontalOffset = 0;

      updateOffset = true;
    }
    else if (propertyName == "isLightDismissEnabled")
    {
      if (propertyValue.isBool())
        m_isLightDismissEnabled = propertyValue.asBool();
      else if (propertyValue.isNull())
        m_isLightDismissEnabled = true;
      if (m_isOpen)
      {
        auto popup = GetMenuFlyoutParentPopup();
        if (popup != nullptr)
          popup.IsLightDismissEnabled(m_isLightDismissEnabled);
      }
    }
    else if (propertyName == "isOpen")
    {
      if (propertyValue.isBool())
      {
        m_isOpen = propertyValue.asBool();
        updateIsOpen = true;
      }
    }
    else if (propertyName == "attachAsContextFlyout")
    {
      if (propertyValue.isBool())
      {
        m_attachAsContextFlyout = propertyValue.asBool();
        updateContextFlyout = true;
      }
    }
    else if (propertyName == "placement")
    {
      auto placement = json_type_traits<winrt::FlyoutPlacementMode>::parseJson(propertyValue);
      m_menuFlyout.Placement(placement);
    }
    else if (propertyName == "target")
    {
      if (propertyValue.isNumber())
      {
        m_targetTag = static_cast<int64_t>(propertyValue.asDouble());
        updateTargetElement = true;
      }
      else
        m_targetTag = -1;
    }
    else if (propertyName == "verticalOffset")
    {
      if (propertyValue.isNumber())
        m_verticalOffset = static_cast<float>(propertyValue.asDouble());
      else
        m_verticalOffset = 0;

      updateOffset = true;
    }
  }

  if (updateTargetElement || m_targetElement == nullptr || updateContextFlyout)
  {
    SetTargetFrameworkElement(m_attachAsContextFlyout);
    if (!m_attachAsContextFlyout) {
      winrt::FlyoutBase::SetAttachedFlyout(m_targetElement, m_menuFlyout);
    }
  }

  if (updateOffset && m_isFlyoutShowOptionsSupported)
  {
    winrt::Point newPoint(m_horizontalOffset, m_verticalOffset);
    m_showOptions.Position(newPoint);
  }

  if (m_isFlyoutShowOptionsSupported)
  {
    winrt::Rect exclusionRect = winrt::Rect(100, 100, 20, 20);
    m_showOptions.ExclusionRect(exclusionRect);
  }

  if (updateIsOpen)
  {
    if (m_isOpen)
    {
      AdjustDefaultFlyoutStyle();
      if (m_isFlyoutShowOptionsSupported)
      {
        m_menuFlyout.ShowAt(m_targetElement, m_showOptions);
      }
      else
      {
        winrt::FlyoutBase::ShowAttachedFlyout(m_targetElement);
      }

      auto popup = GetMenuFlyoutParentPopup();
      if (popup != nullptr)
        popup.IsLightDismissEnabled(m_isLightDismissEnabled);
    }
    else
    {
      m_menuFlyout.Hide();
    }
  }

  // TODO: hook up view props to the flyout (m_menuFlyout) instead of setting them on the dummy view.
  //Super::updateProperties(std::move(props));
  m_updating = false;
}

void MenuFlyoutShadowNode::SetTargetFrameworkElement(bool updateContextFlyout)
{
  auto wkinstance = GetViewManager()->GetReactInstance();
  auto instance = wkinstance.lock();

  if (instance == nullptr)
    return;

  if (m_targetTag > 0)
  {
    auto pNativeUIManagerHost = static_cast<NativeUIManager*>(instance->NativeUIManager())->getHost();
    ShadowNodeBase* pShadowNodeChild = static_cast<ShadowNodeBase*>(pNativeUIManagerHost->FindShadowNodeForTag(m_targetTag));

    if (pShadowNodeChild != nullptr)
    {
      auto targetView = pShadowNodeChild->GetView();
      m_targetElement = targetView.as<winrt::FrameworkElement>();
    }
  }
  else
  {
    m_targetElement = winrt::Window::Current().Content().as<winrt::FrameworkElement>();
  }

  if (updateContextFlyout) {
    m_targetElement.ContextFlyout(m_menuFlyout);
  } 
	else
  {
    m_targetElement.ClearValue(winrt::UIElement::ContextFlyoutProperty());
  }
}

void MenuFlyoutShadowNode::AdjustDefaultFlyoutStyle()
{
  winrt::Style flyoutStyle({ L"Windows.UI.Xaml.Controls.MenuFlyoutPresenter", winrt::TypeKind::Metadata });
  flyoutStyle.Setters().Append(winrt::Setter(winrt::FrameworkElement::MaxWidthProperty(), winrt::box_value(50000)));
  flyoutStyle.Setters().Append(winrt::Setter(winrt::FrameworkElement::MaxHeightProperty(), winrt::box_value(50000)));
  flyoutStyle.Setters().Append(winrt::Setter(winrt::Control::PaddingProperty(), winrt::box_value(0)));
  m_menuFlyout.MenuFlyoutPresenterStyle(flyoutStyle);
}

winrt::Popup MenuFlyoutShadowNode::GetMenuFlyoutParentPopup() const
{
  // TODO: Use VisualTreeHelper::GetOpenPopupsFromXamlRoot when running against RS6
  winrt::Windows::Foundation::Collections::IVectorView<winrt::Popup> popups = winrt::VisualTreeHelper::GetOpenPopups(winrt::Window::Current());
  if (popups.Size() > 0)
    return popups.GetAt(0);
  return nullptr;
}

MenuFlyoutViewManager::MenuFlyoutViewManager(const std::shared_ptr<IReactInstance>& reactInstance)
  : Super(reactInstance)
{
}

const char* MenuFlyoutViewManager::GetName() const
{
  return "RCTMenuFlyout";
}

XamlView MenuFlyoutViewManager::CreateViewCore(int64_t tag)
{
  return ViewPanel::Create().as<XamlView>();
}

facebook::react::ShadowNode* MenuFlyoutViewManager::createShadow() const
{
  return new MenuFlyoutShadowNode();
}

folly::dynamic MenuFlyoutViewManager::GetNativeProps() const
{
  auto props = Super::GetNativeProps();

  props.update(folly::dynamic::object
    ("horizontalOffset", "number")
    ("isLightDismissEnabled", "boolean")
    ("isOpen", "boolean")
    ("attachAsContextFlyout", "boolean")
    ("placement", "number")
    ("target", "number")
    ("verticalOffset", "number")
  );

  return props;
}

folly::dynamic MenuFlyoutViewManager::GetExportedCustomDirectEventTypeConstants() const
{
  auto directEvents = Super::GetExportedCustomDirectEventTypeConstants();
  directEvents["topDismiss"] = folly::dynamic::object("registrationName", "onDismiss");

  return directEvents;
}

void MenuFlyoutViewManager::SetLayoutProps(ShadowNodeBase& nodeToUpdate, XamlView viewToUpdate, float left, float top, float width, float height)
{
}

}}
