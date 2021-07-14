// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include <DynamicReader.h>
#include <DynamicWriter.h>
#include <IReactContext.h>
#include <IReactRootView.h>
#include <Modules/PaperUIManagerModule.h>
#include <Modules\NativeUIManager.h>
#include <Views/ViewManager.h>
#include <XamlUtils.h>
#include "ShadowNodeBase.h"
#include "Unicode.h"
#include "XamlUIService.h"

namespace Microsoft::ReactNative {

struct ViewAtIndex final {
  int64_t tag{};
  int64_t index{};
  static bool ViewAtIndexCompare(
      const std::shared_ptr<ViewAtIndex> &x,
      const std::shared_ptr<ViewAtIndex> &y) noexcept {
    return (x->index < y->index);
  }
};

std::vector<int64_t> PopulateIntVecFromDynamicArray(React::JSValueArray &arr) {
  std::vector<int64_t> vec(arr.empty() ? 0 : arr.size());

  if (arr.empty())
    return vec;

  auto i = 0;
  for (auto &val : arr) {
    vec[i] = val.AsInt64();
    ++i;
  }
  return vec;
}

winrt::Microsoft::ReactNative::ReactPropertyId<
    winrt::Microsoft::ReactNative::ReactNonAbiValue<std::unique_ptr<UIManagerSettings>>>
UIManagerSettingsProperty() noexcept {
  static winrt::Microsoft::ReactNative::ReactPropertyId<
      winrt::Microsoft::ReactNative::ReactNonAbiValue<std::unique_ptr<UIManagerSettings>>>
      prop{L"ReactNative.UIManager", L"Settings"};
  return prop;
}

UIManagerSettings::UIManagerSettings(
    const std::shared_ptr<facebook::react::MessageQueueThread> batchingUIMessageQueue,
    std::vector<std::unique_ptr<IViewManager>> &&viewManagers)
    : batchingUIMessageQueue(std::move(batchingUIMessageQueue)), viewManagers(std::move(viewManagers)) {}

void UIManager::SetSettings(
    winrt::Microsoft::ReactNative::IReactPropertyBag const &properties,
    std::unique_ptr<UIManagerSettings> &&settings) noexcept {
  properties.Set(
      UIManagerSettingsProperty().Handle(),
      winrt::Microsoft::ReactNative::ReactNonAbiValue<std::unique_ptr<UIManagerSettings>>{
          std::in_place, std::move(settings)});
}

winrt::Microsoft::ReactNative::ReactPropertyId<
    winrt::Microsoft::ReactNative::ReactNonAbiValue<std::weak_ptr<NativeUIManager>>>
NativeUIManagerProperty() noexcept {
  static winrt::Microsoft::ReactNative::ReactPropertyId<
      winrt::Microsoft::ReactNative::ReactNonAbiValue<std::weak_ptr<NativeUIManager>>>
      prop{L"ReactNative.UIManager", L"NativeUIManager"};
  return prop;
}

std::weak_ptr<NativeUIManager> GetNativeUIManager(const Mso::React::IReactContext &context) {
  auto v = winrt::Microsoft::ReactNative::ReactPropertyBag(context.Properties()).Get(NativeUIManagerProperty());
  return v ? v.Value() : std::weak_ptr<NativeUIManager>{};
}

class UIManagerModule : public std::enable_shared_from_this<UIManagerModule>, public INativeUIManagerHost {
 public:
  UIManagerModule() {}
  ~UIManagerModule() {
    m_nodeRegistry.removeAllRootViews([this](int64_t rootViewTag) { removeRootView(rootViewTag); });
    m_nodeRegistry.ForAllNodes(
        [this](int64_t tag, shadow_ptr const &shadowNode) noexcept { DropView(tag, false, true); });
    m_nativeUIManager->setHost(nullptr);
  }

  void Initialize(winrt::Microsoft::ReactNative::ReactContext const &reactContext) noexcept {
    m_context = reactContext;

    m_nativeUIManager = std::make_shared<NativeUIManager>(reactContext);

    m_context.Properties().Set(NativeUIManagerProperty(), std::weak_ptr<NativeUIManager>(m_nativeUIManager));

    auto settings = m_context.Properties().Get(UIManagerSettingsProperty());
    m_viewManagers = std::move((*settings)->viewManagers);
    m_nativeUIManager->setHost(this);
  }

  React::JSValueObject getConstantsForViewManager(std::string &&viewManagerName) noexcept {
    const IViewManager *vm = GetViewManager(viewManagerName);

    auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
    writer.WriteObjectBegin();

    if (vm != nullptr)
      vm->GetConstants(writer);
    writer.WriteObjectEnd();

    auto val = TakeJSValue(writer);
    return std::move(val.AsObject().Copy()); // Lame why do we need to copy?
  }

  void ConstantsViaConstantsProvider(winrt::Microsoft::ReactNative::ReactConstantProvider &constants) noexcept {
    auto writer = constants.Writer();
    for (auto &&vm : m_viewManagers) {
      writer.WritePropertyName(vm->GetName());
      writer.WriteObjectBegin();
      vm->GetConstants(writer);
      writer.WriteObjectEnd();
    }
    constants.Add(L"AccessibilityEventTypes", accessibilityEventTypes);
  }

  void createView(int64_t reactTag, std::string viewName, int64_t rootTag, React::JSValueObject &&props) noexcept {
    m_nativeUIManager->ensureInBatch();
    auto viewManager = GetViewManager(viewName);
    auto node = viewManager->createShadow();
    node->m_className = std::move(viewName);
    node->m_tag = reactTag;
    node->m_viewManager = viewManager;

    node->createView(props);

    m_nativeUIManager->CreateView(*node, props);

    m_nodeRegistry.addNode(shadow_ptr(node), reactTag);

    node->updateProperties(props);
  }

  void updateView(int64_t reactTag, std::string viewName, React::JSValueObject &&props) noexcept {
    m_nativeUIManager->ensureInBatch();
    ShadowNode *pShadowNode = FindShadowNodeForTag(reactTag);
    // Guard against setNative calls to removed views
    if (pShadowNode == nullptr)
      return;

    if (!pShadowNode->m_zombie)
      pShadowNode->updateProperties(props);

    m_nativeUIManager->UpdateView(*pShadowNode, props);
  }

  void focus(int64_t reactTag) noexcept {
    m_nativeUIManager->focus(reactTag);
  }

  void blur(int64_t reactTag) noexcept {
    m_nativeUIManager->blur(reactTag);
  }

  void findSubviewIn(
      int64_t reactTag,
      React::JSValueArray &&point,
      std::function<void(double nativeViewTag, double left, double top, double width, double height)>
          &&callback) noexcept {
    if (auto node = m_nodeRegistry->findNode(reactTag)) {
      float x = static_cast<float>(point[0]);
      float y = static_cast<float>(point[1]);
      m_nativeUIManager->findSubviewIn(*node, x, y, std::move(callback));
    }
  }

  void dispatchViewManagerCommand(
      int64_t reactTag,
      winrt::Microsoft::ReactNative::JSValue &&commandID,
      React::JSValueArray &&commandArgs) noexcept {
    m_nativeUIManager->ensureInBatch();
    if (auto node = m_nodeRegistry.findNode(reactTag)) {
      if (!node->m_zombie)
        node->dispatchCommand(commandID.AsString(), std::move(commandArgs));
    }
  }

  void measure(
      int64_t reactTag,
      std::function<void(double left, double top, double width, double height, double pageX, double pageY)>
          &&callback) noexcept {
    if (auto node = m_nodeRegistry.findNode(reactTag)) {
      int64_t rootTag = reactTag;
      while (true) {
        auto currNode = m_nodeRegistry.getNode(rootTag);
        if (currNode.m_parent == -1)
          break;
        rootTag = currNode.m_parent;
      }
      auto &rootNode = m_nodeRegistry.getNode(rootTag);

      m_nativeUIManager->measure(*node, rootNode, std::move(callback));
    }
  }

  void measureInWindow(
      int64_t reactTag,
      std::function<void(double x, double y, double width, double height)> &&callback) noexcept {
    if (auto node = m_nodeRegistry.findNode(reactTag))
      m_nativeUIManager->measureInWindow(*node, std::move(callback));
  }

  void viewIsDescendantOf(
      int64_t reactTag,
      int64_t ancestorReactTag,
      std::function<void(React::JSValue const &)> const &callback) noexcept {
    assert(false);
    // TODO
  }

  void measureLayout(
      int64_t reactTag,
      int64_t ancestorReactTag,
      std::function<void(React::JSValue const &)> &&errorCallback,
      std::function<void(double left, double top, double width, double height)> &&callback) noexcept {
    auto node = m_nodeRegistry.findNode(reactTag);
    auto ancestorNode = m_nodeRegistry.findNode(ancestorReactTag);
    if (node && ancestorNode) {
      m_nativeUIManager->measureLayout(*node, *ancestorNode, std::move(errorCallback), std::move(callback));
    }
  }

  void measureLayoutRelativeToParent(
      int64_t reactTag,
      std::function<void(React::JSValue const &)> &&errorCallback,
      std::function<void(React::JSValue const &)> &&callback) noexcept {
    assert(false);
    // TODO
  }

  void setJSResponder(int64_t reactTag, bool blockNativeResponder) noexcept {
    // TODO
  }

  void clearJSResponder() noexcept {
    // TODO
  }

  void configureNextLayoutAnimation(
      React::JSValueObject &&config,
      std::function<void()> &&callback,
      std::function<void(React::JSValue const &)> &&errorCallback) noexcept {
    m_nativeUIManager->configureNextLayoutAnimation(std::move(config), std::move(callback), std::move(errorCallback));
  }

  void removeSubviewsFromContainerWithID(int64_t containerID) noexcept {
    m_nativeUIManager->ensureInBatch();
    if (auto containerNode = m_nodeRegistry.findNode(containerID)) {

      std::vector<int64_t> indicesToRemove(containerNode->m_children.size());
      for (size_t i = 0; i < containerNode->m_children.size(); i++)
        indicesToRemove[static_cast<size_t>(i)] = static_cast<int64_t>(i);
      std::vector<int64_t> emptyVec;
      manageChildren(*containerID, emptyVec, emptyVec, emptyVec, emptyVec, indicesToRemove);
    }
  }

  void replaceExistingNonRootView(int64_t reactTag, int64_t newReactTag) noexcept {
    m_nativeUIManager->ensureInBatch();
    std::vector<int64_t> indicesToRemove(1);
    std::vector<int64_t> indicesToAdd(1);
    std::vector<int64_t> tagToAdd(1);
    tagToAdd[0] = newReactTag;

    if (auto node = m_nodeRegistry.findNode(reactTag)) {
      CHECK(node->m_parent != -1) << "oldTag must have a parent";
      if (auto parent = m_nodeRegistry.findNode(node->m_parent)) {
        auto it = find(parent->m_children.begin(), parent->m_children.end(), reactTag);
        CHECK(it != parent->m_children.end());
        indicesToAdd[0] = indicesToRemove[0] = it - parent->m_children.begin();

        std::vector<int64_t> emptyVec;
        manageChildren(parent->m_tag, emptyVec, emptyVec, tagToAdd, indicesToAdd, indicesToRemove);
      }
    }
  }

  void removeRootView(int64_t rootViewTag) noexcept {
    m_nativeUIManager->ensureInBatch();
    if (auto node = m_nodeRegistry.findRoot(rootViewTag)) {
      m_nativeUIManager->removeRootView(*node);
      DropView(rootViewTag, true);
      m_nodeRegistry.removeRootView(rootViewTag);
      m_nativeUIManager->destroyRootShadowNode(node);
    }
  }

  void setChildren(int64_t containerTag, React::JSValueArray &&reactTags) noexcept {
    m_nativeUIManager->ensureInBatch();
    if (auto parent = m_nodeRegistry.findNode(containerTag)) {
      int64_t index = 0;
      for (auto &&childTag : reactTags) {
        auto tag = childTag.AsInt64();
        auto childNode = m_nodeRegistry.findNode(tag);
        childNode->m_parent = parent->m_tag;
        parent->m_children.push_back(tag);
        if (!parent->m_zombie)
          parent->AddView(*childNode, index);

        m_nativeUIManager->AddView(*parent, *childNode, index);
        ++index;
      }
    }
  }

  void manageChildren(
      int64_t containerTag,
      React::JSValueArray &&moveFromIndices,
      React::JSValueArray &&moveToIndices,
      React::JSValueArray &&addChildReactTags,
      React::JSValueArray &&addAtIndices,
      React::JSValueArray &&removeAtIndices) noexcept {
    auto vecMoveFrom = PopulateIntVecFromDynamicArray(moveFromIndices);
    auto vecMoveTo = PopulateIntVecFromDynamicArray(moveToIndices);
    auto vecAddChildTags = PopulateIntVecFromDynamicArray(addChildReactTags);
    auto vecAddAtIndices = PopulateIntVecFromDynamicArray(addAtIndices);
    auto vecRemoveFrom = PopulateIntVecFromDynamicArray(removeAtIndices);
    manageChildren(containerTag, vecMoveFrom, vecMoveTo, vecAddChildTags, vecAddAtIndices, vecRemoveFrom);
  }

  void setLayoutAnimationEnabledExperimental(bool enabled) noexcept {
    assert(false);
    // TODO
  }

  void sendAccessibilityEvent(int64_t reactTag, double eventType) noexcept {
    auto type = int(eventType);
    if (type == accessibilityEventTypes["typeViewFocused"]) {
      focus(reactTag);
    } else {
      assert(false);
    }
  }

  void showPopupMenu(
      int64_t reactTag,
      React::JSValueArray &&items,
      std::function<void(React::JSValue const &)> const &error,
      std::function<void(React::JSValue const &)> const &success) noexcept {
    assert(false);
    // TODO
  }

  void dismissPopupMenu() noexcept {
    assert(false);
    // TODO
  }

 private:
  std::unordered_set<int64_t> &GetAllRootTags() {
    return m_nodeRegistry.getAllRoots();
  }

  ShadowNode *FindShadowNodeForTag(int64_t tag) {
    return m_nodeRegistry.findNode(tag);
  }

  ShadowNode *FindParentRootShadowNode(int64_t tag) {
    return m_nodeRegistry.getParentRootShadowNode(tag);
  }

  ShadowNode &GetShadowNodeForTag(int64_t tag) {
    return m_nodeRegistry.getNode(tag);
  }

  void onBatchComplete() {
    m_nativeUIManager->onBatchComplete();
  }

  void manageChildren(
      int64_t viewTag,
      std::vector<int64_t> &moveFrom,
      std::vector<int64_t> &moveTo,
      std::vector<int64_t> &addChildTags,
      std::vector<int64_t> &addAtIndices,
      std::vector<int64_t> &removeFrom) {
    m_nativeUIManager->ensureInBatch();
    auto &shadowNodeToManage = m_nodeRegistry.getNode(viewTag);

    auto numToMove = moveFrom.size();
    auto numToAdd = addChildTags.size();
    auto numToRemove = removeFrom.size();
    std::vector<std::shared_ptr<ViewAtIndex>> viewsToAdd(numToMove + numToAdd);
    std::vector<std::shared_ptr<ViewAtIndex>> viewsToRemove(numToMove + numToRemove);
    std::vector<int64_t> tagsToDelete(numToRemove);

    if (numToMove > 0) {
      for (size_t i = 0; i < numToMove; ++i) {
        auto moveFromIndex = moveFrom[i];
        auto tagToMove =
            m_nodeRegistry
                .getNode(static_cast<size_t>(shadowNodeToManage.m_children[static_cast<size_t>(moveFromIndex)]))
                .m_tag;
        viewsToAdd[i] = std::make_shared<ViewAtIndex>(ViewAtIndex{tagToMove, moveTo[i]});
        viewsToRemove[i] = std::make_shared<ViewAtIndex>(ViewAtIndex{tagToMove, moveFromIndex});
      }
    }

    if (numToAdd > 0) {
      for (size_t i = 0; i < numToAdd; ++i) {
        viewsToAdd[numToMove + i] = std::make_shared<ViewAtIndex>(ViewAtIndex{addChildTags[i], addAtIndices[i]});
      }
    }

    if (numToRemove > 0) {
      for (size_t i = 0; i < numToRemove; ++i) {
        auto indexToRemove = removeFrom[i];
        auto tagToRemove =
            m_nodeRegistry
                .getNode(static_cast<size_t>(shadowNodeToManage.m_children[static_cast<size_t>(indexToRemove)]))
                .m_tag;
        viewsToRemove[i] = std::make_shared<ViewAtIndex>(ViewAtIndex{tagToRemove, indexToRemove});
        tagsToDelete[i] = tagToRemove;
      }
    }

    // NB: moveFrom and removeForm are both relative to the starting
    // state of the view's children.
    //
    // 1) Sort the views to add and indices to remove by index
    // 2) Iterate the indices being removed from high to low and remove
    //    them. Going high to low makes sure we remove the correct
    //    index when there are multiple to remove.
    // 3) Iterate the views being added by index low to high and add
    //    them. Like the view removal, iteration direction is important
    //    to preserve the correct index.

    std::sort(viewsToAdd.begin(), viewsToAdd.end(), ViewAtIndex::ViewAtIndexCompare);
    std::sort(viewsToRemove.begin(), viewsToRemove.end(), ViewAtIndex::ViewAtIndexCompare);

    // Apply changes to the ReactShadowNode hierarchy.
    for (auto i = static_cast<int>(viewsToRemove.size()) - 1; i >= 0; --i) {
      auto viewAtIndex = viewsToRemove[i];
      auto &shadowNodeToRemove = m_nodeRegistry.getNode(viewAtIndex->tag);

      shadowNodeToManage.m_children.erase(
          shadowNodeToManage.m_children.begin() + static_cast<size_t>(viewAtIndex->index));
      shadowNodeToManage.RemoveChildAt(viewAtIndex->index);
    }

    for (size_t i = 0; i < viewsToAdd.size(); ++i) {
      auto viewAtIndex = viewsToAdd[i];
      auto &shadowNodeToAdd = m_nodeRegistry.getNode(viewAtIndex->tag);
      shadowNodeToAdd.m_parent = shadowNodeToManage.m_tag;
      shadowNodeToManage.m_children.insert(
          shadowNodeToManage.m_children.begin() + static_cast<size_t>(viewAtIndex->index), shadowNodeToAdd.m_tag);
      if (!shadowNodeToManage.m_zombie)
        shadowNodeToManage.AddView(shadowNodeToAdd, viewAtIndex->index);

      m_nativeUIManager->AddView(shadowNodeToManage, shadowNodeToAdd, viewAtIndex->index);
    }

    for (auto tagToDelete : tagsToDelete)
      DropView(tagToDelete);
  }

  IViewManager *GetViewManager(const std::string &className) const {
    auto wclassName = Microsoft::Common::Unicode::Utf8ToUtf16(className);
    for (auto &&vm : m_viewManagers) {
      if (!std::wcscmp(vm->GetName(), wclassName.c_str()))
        return vm.get();
    }
    return nullptr;
  }

  void RegisterRootView(facebook::react::IReactRootView *rootView, int64_t rootViewTag, int64_t width, int64_t height) {
    static std::string rootClassName = "ROOT";
    auto viewManager = GetViewManager(rootClassName);

    auto root = m_nativeUIManager->createRootShadowNode(rootView);
    root->m_className = rootClassName;
    root->m_viewManager = viewManager;
    root->m_tag = rootViewTag;
    m_nodeRegistry.addRootView(shadow_ptr(root), rootViewTag);

    m_nativeUIManager->AddRootView(*root, rootView);
  }

  void zombieView(int64_t tag) {
    DropView(tag, false, true);
  }

  void DropView(int64_t tag, bool removeChildren = true, bool zombieView = false) {
    if (auto node = m_nodeRegistry.findNode(tag)) {
      node->onDropViewInstance();

      m_nativeUIManager->RemoveView(*node, removeChildren);

      if (zombieView)
        node->m_zombie = true;

      for (auto childTag : node->m_children)
        DropView(childTag, removeChildren, zombieView);

      if (removeChildren)
        node->removeAllChildren();

      if (!zombieView)
        m_nodeRegistry.removeNode(tag);
    }
  }

  winrt::Microsoft::ReactNative::ReactContext m_context;
  std::vector<std::unique_ptr<IViewManager>> m_viewManagers;
  ShadowNodeRegistry m_nodeRegistry;
  std::shared_ptr<NativeUIManager> m_nativeUIManager;
  const React::JSValueObject accessibilityEventTypes = React::JSValueObject{{"typeViewFocused", 8}};
};

UIManager::UIManager() : m_module(std::make_shared<UIManagerModule>()) {}

UIManager::~UIManager() {
  // To make sure that we destroy UI components in UI thread.
  if (!m_context.UIDispatcher().HasThreadAccess()) {
    m_context.UIDispatcher().Post([module = std::move(m_module)]() {});
  }
}

void UIManager::Initialize(winrt::Microsoft::ReactNative::ReactContext const &reactContext) noexcept {
  m_context = reactContext;

  auto settings = m_context.Properties().Get(UIManagerSettingsProperty());
  m_batchingUIMessageQueue = std::move((*settings)->batchingUIMessageQueue);

  m_module->Initialize(reactContext);
}

React::JSValueObject UIManager::getConstantsForViewManager(std::string viewManagerName) noexcept {
  return m_module->getConstantsForViewManager(std::move(viewManagerName));
}

React::JSValueObject UIManager::getViewManagerConfig(std::string viewManagerName) noexcept {
  return getConstantsForViewManager(std::move(viewManagerName));
}

void UIManager::ConstantsViaConstantsProvider(
    winrt::Microsoft::ReactNative::ReactConstantProvider &constants) noexcept {
  m_module->ConstantsViaConstantsProvider(constants);
}

React::JSValueArray UIManager::getDefaultEventTypes() noexcept {
  assert(false);
  // TODO
  return {};
}

React::JSValueObject UIManager::lazilyLoadView(std::string name) noexcept {
  assert(false);
  // TODO
  return {};
}

void UIManager::createView(
    double reactTag,
    std::string viewName,
    double rootTag,
    React::JSValueObject &&props) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         viewName = std::move(viewName),
                                                         rootTag,
                                                         props = std::move(props)]() mutable {
    if (auto module = m.lock()) {
      module->createView(static_cast<int64_t>(reactTag), viewName, static_cast<int64_t>(rootTag), std::move(props));
    }
  }));
}

void UIManager::updateView(double reactTag, std::string viewName, React::JSValueObject &&props) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         viewName = std::move(viewName),
                                                         props = std::move(props)]() mutable {
    if (auto module = m.lock()) {
      module->updateView(static_cast<int64_t>(reactTag), viewName, std::move(props));
    }
  }));
}

void UIManager::focus(double reactTag) noexcept {
  m_batchingUIMessageQueue->runOnQueue([m = std::weak_ptr<UIManagerModule>(m_module), reactTag]() {
    if (auto module = m.lock()) {
      module->focus(static_cast<int64_t>(reactTag));
    }
  });
}

void UIManager::blur(double reactTag) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), reactTag]() {
    if (auto module = m.lock()) {
      module->blur(static_cast<int64_t>(reactTag));
    }
  }));
}

void UIManager::findSubviewIn(
    double reactTag,
    React::JSValueArray &&point,
    std::function<void(double nativeViewTag, double left, double top, double width, double height)> const
        &callback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         point = std::move(point),
                                                         callback = std::move(callback)]() mutable {
    if (auto module = m.lock()) {
      module->findSubviewIn(static_cast<int64_t>(reactTag), std::move(point), std::move(callback));
    }
  }));
}

void UIManager::dispatchViewManagerCommand(
    double reactTag,
    winrt::Microsoft::ReactNative::JSValue &&commandID,
    React::JSValueArray &&commandArgs) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         commandID = std::move(commandID),
                                                         commandArgs = std::move(commandArgs)]() mutable {
    if (auto module = m.lock()) {
      module->dispatchViewManagerCommand(static_cast<int64_t>(reactTag), std::move(commandID), std::move(commandArgs));
    }
  }));
}

void UIManager::measure(
    double reactTag,
    std::function<void(double left, double top, double width, double height, double pageX, double pageY)> const
        &callback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor(
      [m = std::weak_ptr<UIManagerModule>(m_module), reactTag, callback = std::move(callback)]() mutable {
        if (auto module = m.lock()) {
          module->measure(static_cast<int64_t>(reactTag), std::move(callback));
        }
      }));
}

void UIManager::measureInWindow(
    double reactTag,
    std::function<void(double x, double y, double width, double height)> const &callback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor(
      [m = std::weak_ptr<UIManagerModule>(m_module), reactTag, callback = std::move(callback)]() mutable {
        if (auto module = m.lock()) {
          module->measureInWindow(static_cast<int64_t>(reactTag), std::move(callback));
        }
      }));
}

void UIManager::viewIsDescendantOf(
    double reactTag,
    double ancestorReactTag,
    std::function<void(React::JSValue const &)> const &callback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         ancestorReactTag,
                                                         callback = std::move(callback)]() mutable {
    if (auto module = m.lock()) {
      module->viewIsDescendantOf(
          static_cast<int64_t>(reactTag), static_cast<int64_t>(ancestorReactTag), std::move(callback));
    }
  }));
}

void UIManager::measureLayout(
    double reactTag,
    double ancestorReactTag,
    std::function<void(React::JSValue const &)> const &errorCallback,
    std::function<void(double left, double top, double width, double height)> const &callback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         ancestorReactTag,
                                                         errorCallback = std::move(errorCallback),
                                                         callback = std::move(callback)]() mutable {
    if (auto module = m.lock()) {
      module->measureLayout(
          static_cast<int64_t>(reactTag),
          static_cast<int64_t>(ancestorReactTag),
          std::move(errorCallback),
          std::move(callback));
    }
  }));
}

void UIManager::measureLayoutRelativeToParent(
    double reactTag,
    std::function<void(React::JSValue const &)> const &errorCallback,
    std::function<void(React::JSValue const &)> const &callback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         errorCallback = std::move(errorCallback),
                                                         callback = std::move(callback)]() mutable {
    if (auto module = m.lock()) {
      module->measureLayoutRelativeToParent(
          static_cast<int64_t>(reactTag), std::move(errorCallback), std::move(callback));
    }
  }));
}

void UIManager::setJSResponder(double reactTag, bool blockNativeResponder) noexcept {
  m_batchingUIMessageQueue->runOnQueue(
      Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), reactTag, blockNativeResponder]() mutable {
        if (auto module = m.lock()) {
          module->setJSResponder(static_cast<int64_t>(reactTag), blockNativeResponder);
        }
      }));
}

void UIManager::clearJSResponder() noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module)]() mutable {
    if (auto module = m.lock()) {
      module->clearJSResponder();
    }
  }));
}

void UIManager::configureNextLayoutAnimation(
    React::JSValueObject &&config,
    std::function<void()> const &callback,
    std::function<void(React::JSValue const &)> const &errorCallback) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         config = std::move(config),
                                                         callback = std::move(callback),
                                                         errorCallback = std::move(errorCallback)]() mutable {
    if (auto module = m.lock()) {
      module->configureNextLayoutAnimation(std::move(config), std::move(callback), std::move(errorCallback));
    }
  }));
}

void UIManager::removeSubviewsFromContainerWithID(double containerID) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), containerID]() {
    if (auto module = m.lock()) {
      module->removeSubviewsFromContainerWithID(static_cast<int64_t>(containerID));
    }
  }));
}

void UIManager::replaceExistingNonRootView(double reactTag, double newReactTag) noexcept {
  m_batchingUIMessageQueue->runOnQueue(
      Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), reactTag, newReactTag]() {
        if (auto module = m.lock()) {
          module->replaceExistingNonRootView(static_cast<int64_t>(reactTag), static_cast<int64_t>(newReactTag));
        }
      }));
}

void UIManager::removeRootView(double reactTag) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), reactTag]() {
    if (auto module = m.lock()) {
      module->removeRootView(static_cast<int64_t>(reactTag));
    }
  }));
}

void UIManager::setChildren(double containerTag, React::JSValueArray &&reactTags) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor(
      [m = std::weak_ptr<UIManagerModule>(m_module), containerTag, reactTags = std::move(reactTags)]() mutable {
        if (auto module = m.lock()) {
          module->setChildren(static_cast<int64_t>(containerTag), std::move(reactTags));
        }
      }));
}

void UIManager::manageChildren(
    double containerTag,
    React::JSValueArray &&moveFromIndices,
    React::JSValueArray &&moveToIndices,
    React::JSValueArray &&addChildReactTags,
    React::JSValueArray &&addAtIndices,
    React::JSValueArray &&removeAtIndices) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         containerTag,
                                                         moveFromIndices = std::move(moveFromIndices),
                                                         moveToIndices = std::move(moveToIndices),
                                                         addChildReactTags = std::move(addChildReactTags),
                                                         addAtIndices = std::move(addAtIndices),
                                                         removeAtIndices = std::move(removeAtIndices)]() mutable {
    if (auto module = m.lock()) {
      module->manageChildren(
          static_cast<int64_t>(containerTag),
          std::move(moveFromIndices),
          std::move(moveToIndices),
          std::move(addChildReactTags),
          std::move(addAtIndices),
          std::move(removeAtIndices));
    }
  }));
}

void UIManager::setLayoutAnimationEnabledExperimental(bool enabled) noexcept {
  m_batchingUIMessageQueue->runOnQueue(
      Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), enabled]() mutable {
        if (auto module = m.lock()) {
          module->setLayoutAnimationEnabledExperimental(enabled);
        }
      }));
}

void UIManager::sendAccessibilityEvent(double reactTag, double eventType) noexcept {
  m_batchingUIMessageQueue->runOnQueue(
      Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module), reactTag, eventType]() mutable {
        if (auto module = m.lock()) {
          module->sendAccessibilityEvent(static_cast<int64_t>(reactTag), eventType);
        }
      }));
}

void UIManager::showPopupMenu(
    double reactTag,
    React::JSValueArray &&items,
    std::function<void(React::JSValue const &)> const &error,
    std::function<void(React::JSValue const &)> const &success) noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module),
                                                         reactTag,
                                                         items = std::move(items),
                                                         error = std::move(error),
                                                         success = std::move(success)]() mutable {
    if (auto module = m.lock()) {
      module->showPopupMenu(static_cast<int64_t>(reactTag), std::move(items), std::move(error), std::move(success));
    }
  }));
}

void UIManager::dismissPopupMenu() noexcept {
  m_batchingUIMessageQueue->runOnQueue(Mso::VoidFunctor([m = std::weak_ptr<UIManagerModule>(m_module)]() mutable {
    if (auto module = m.lock()) {
      module->dismissPopupMenu();
    }
  }));
}

} // namespace Microsoft::ReactNative
