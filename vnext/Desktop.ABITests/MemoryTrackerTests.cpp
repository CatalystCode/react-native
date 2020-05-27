// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include <winrt/base.h>
#include <winrt/facebook.react.h>

#include "ActivationFactory.h"
#include "SimpleMessageQueue.h"

using namespace winrt::facebook::react;

namespace ABITests {

// We turn clang format off here because it does not work with some of the
// test macros.
// clang-format off

TEST_CLASS(MemoryTrackerTests) {
 public:
  MemoryTrackerTests() noexcept {
    winrt_activation_handler = WINRT_RoGetActivationFactory;
  }

  TEST_METHOD(Handler_AddedAndRemoved){
    init_apartment(winrt::apartment_type::single_threaded);
    IMessageQueue callbackMessageQueue = ::winrt::make<SimpleMessageQueue>();
    MemoryTracker tracker{callbackMessageQueue};

    uint32_t registrationToken = tracker.AddThresholdHandler(
        /* threshold */ 100,
        /* minCallbackIntervalInMilliseconds */ 100,
        [](uint64_t currentUsage) {});
    TestCheck(tracker.RemoveThresholdHandler(registrationToken));
  }


  TEST_METHOD(CurrentMemoryUsage_ReturnsInitialValue) {
    init_apartment(winrt::apartment_type::single_threaded);
    IMessageQueue callbackMessageQueue = ::winrt::make<SimpleMessageQueue>();
    MemoryTracker tracker{callbackMessageQueue};

    tracker.Initialize(1000);
    TestCheckEqual(
        1000ull, tracker.CurrentMemoryUsage(), "CurrentMemoryUsage");
  }

  TEST_METHOD(PeakMemoryUsage_ReturnsInitialValue) {
    init_apartment(winrt::apartment_type::single_threaded);
    IMessageQueue callbackMessageQueue = ::winrt::make<SimpleMessageQueue>();
    MemoryTracker tracker{callbackMessageQueue};

    tracker.Initialize(1000);
    TestCheckEqual(1000ull, tracker.PeakMemoryUsage(), "PeakMemoryUsage");
  }

 
  TEST_METHOD(ThresholdHandler_Called) {
    init_apartment(winrt::apartment_type::single_threaded);

    IMessageQueue callbackMessageQueue = ::winrt::make<SimpleMessageQueue>();
    MemoryTracker tracker{callbackMessageQueue};

    tracker.Initialize(500);

    std::vector<uint64_t> actualCallbacks;

    uint32_t registrationToken = tracker.AddThresholdHandler(
         1000,
         100,
        [&actualCallbacks](uint64_t currentUsage) {
          actualCallbacks.push_back(currentUsage);
        });

    tracker.OnAllocation(1000);

    // allow the callback to get dispatched
    TestCheck(callbackMessageQueue.as<SimpleMessageQueue>()->DispatchOne());
    TestCheck(callbackMessageQueue.as<SimpleMessageQueue>()->IsEmpty());

    TestCheckEqual(
        1500ull, tracker.CurrentMemoryUsage(), "CurrentMemoryUsage");
    TestCheckEqual(1500ull, tracker.PeakMemoryUsage(), "PeakMemoryUsage");

    TestCheckEqual(static_cast<size_t>(1), actualCallbacks.size());
    TestCheckEqual(1500ull, actualCallbacks[0]);

    TestCheck(tracker.RemoveThresholdHandler(registrationToken));
  }

};

// clange-format on

} // namespace ABITests
