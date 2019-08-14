// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include "UIMessageQueueThread.h"

#include <winrt/Windows.UI.Core.h>

namespace react {
namespace uwp {

UIMessageQueueThread::UIMessageQueueThread(
    winrt::Windows::UI::Core::CoreDispatcher dispatcher)
    : m_uiDispatcher(dispatcher) {}

UIMessageQueueThread::~UIMessageQueueThread() {}

void UIMessageQueueThread::runOnQueue(std::function<void()> &&func) {
  ensureQueue();
  m_queue->push_back(func);

//#define TRACK_UI_CALLS
#ifdef TRACK_UI_CALLS
        char buffer[1024];
        static uint32_t cCalls = 0;
        _snprintf_s(
            buffer,
            _countof(buffer),
            _TRUNCATE,
            "UIMessageQueueThread Calls: %u\r\n",
            ++cCalls);
        OutputDebugStringA(buffer);
#endif

        func();
      });
}

void UIMessageQueueThread::ensureQueue(){
  if (m_queue == nullptr) {
    m_queue = std::make_unique<WorkItemQueue>();
  }
}

void UIMessageQueueThread::onBatchComplete() {
  std::shared_ptr<WorkItemQueue> queue = m_queue;
  m_queue = nullptr;
  if (queue) {
    m_uiDispatcher.RunAsync(
      winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
      [queue]() {
        for (auto func : *queue) {
          func();
        }
      });
  }
}

void UIMessageQueueThread::runOnQueueSync(std::function<void()> &&func) {
  // Not supported
  // TODO: crash
}

void UIMessageQueueThread::quitSynchronous() {
  // Not supported
  // TODO: crash
}

} // namespace uwp
} // namespace react

