// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <Views/ControlViewManager.h>

namespace react { namespace uwp {

class CalendarViewViewManager : public ControlViewManager
{
  using Super = ControlViewManager;
public:
  CalendarViewViewManager(const std::shared_ptr<IReactInstance>& reactInstance);

  const char* GetName() const override;
  folly::dynamic GetNativeProps() const override;

  facebook::react::ShadowNode* createShadow() const;

  void UpdateProperties(ShadowNodeBase* nodeToUpdate, folly::dynamic reactDiffMap) override;

protected:
  XamlView CreateViewCore(int64_t tag) override;

  friend class CalendarViewShadowNode;
};

} }
