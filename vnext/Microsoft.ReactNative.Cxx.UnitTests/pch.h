// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define NOGDI
#define NOMINMAX

#include <unknwn.h>

#undef GetCurrentTime

#include <CppWinRTIncludes.h>
#include <winrt/Microsoft.ReactNative.h>

#include <future/future.h>
#include <future/futureWait.h>
#include <gtest/gtest.h>
#include <motifCpp/gTestAdapter.h>
#include <motifCpp/testCheck.h>

#include <mutex>
#include <stack>

#ifndef CXXUNITTESTS
#define CXXUNITTESTS
#endif // CXXUNITTESTS
