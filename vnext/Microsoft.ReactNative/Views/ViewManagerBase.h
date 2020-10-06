// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <React.h>
#include <Shared/ReactWindowsAPI.h>
#include <Shared/ViewManager.h>
#include <XamlView.h>
#include <folly/dynamic.h>
#include <yoga/yoga.h>

namespace facebook {
namespace react {
struct ShadowNode;
}
} // namespace facebook

namespace react::uwp {

class ExpressionAnimationStore;
struct IReactInstance;
struct ShadowNodeBase;

struct YogaContext {
  YogaContext(const XamlView &view_) : view(view_) {}

  XamlView view;
};

REACTWINDOWS_EXPORT YGSize DefaultYogaSelfMeasureFunc(
    YGNodeRef node,
    float width,
    YGMeasureMode widthMode,
    float height,
    YGMeasureMode heightMode);

#pragma warning(push)
#pragma warning(disable : 4275) // base is not DLL exported
#pragma warning(disable : 4251) // member is not DLL exported
class REACTWINDOWS_EXPORT ViewManagerBase : public facebook::react::IViewManager {
 public:
  ViewManagerBase(const Mso::React::IReactContext &context);
  virtual ~ViewManagerBase() {}

  virtual XamlView CreateView(int64_t tag);

  folly::dynamic GetExportedViewConstants() const override;
  folly::dynamic GetCommands() const override;
  folly::dynamic GetNativeProps() const override;

  facebook::react::ShadowNode *createShadow() const override;
  void destroyShadow(facebook::react::ShadowNode *node) const override;

  folly::dynamic GetConstants() const override;
  folly::dynamic GetExportedCustomBubblingEventTypeConstants() const override;
  folly::dynamic GetExportedCustomDirectEventTypeConstants() const override;

  virtual void AddView(const XamlView &parent, const XamlView &child, int64_t index);
  virtual void RemoveAllChildren(const XamlView &parent);
  virtual void RemoveChildAt(const XamlView &parent, int64_t index);
  virtual void ReplaceChild(const XamlView &parent, const XamlView &oldChild, const XamlView &newChild);

  virtual void UpdateProperties(ShadowNodeBase *nodeToUpdate, const folly::dynamic &reactDiffMap);

  virtual void
  DispatchCommand(const XamlView &viewToUpdate, const std::string &commandId, const folly::dynamic &commandArgs);

  // Yoga Layout
  virtual void SetLayoutProps(
      ShadowNodeBase &nodeToUpdate,
      const XamlView &viewToUpdate,
      float left,
      float top,
      float width,
      float height);
  virtual YGMeasureFunc GetYogaCustomMeasureFunc() const;
  virtual bool RequiresYogaNode() const;
  bool IsNativeControlWithSelfLayout() const;

  const Mso::React::IReactContext &GetReactContext() const {
    return *m_context;
  }
  std::shared_ptr<ExpressionAnimationStore> GetExpressionAnimationStore() noexcept;
  void DispatchEvent(int64_t viewTag, std::string &&eventName, folly::dynamic &&eventData) const noexcept;

  virtual void TransferProperties(const XamlView &oldView, const XamlView &newView);

 protected:
  virtual XamlView CreateViewCore(int64_t tag) = 0;
  virtual void OnViewCreated(XamlView view) {}
  virtual bool
  UpdateProperty(ShadowNodeBase *nodeToUpdate, const std::string &propertyName, const folly::dynamic &propertyValue);
  virtual void NotifyUnimplementedProperty(
      ShadowNodeBase *nodeToUpdate,
      const std::string &propertyName,
      const folly::dynamic &value);
  virtual void OnPropertiesUpdated(ShadowNodeBase *node) {}

 protected:
  Mso::CntPtr<const Mso::React::IReactContext> m_context;
};
#pragma warning(pop)

} // namespace react::uwp
