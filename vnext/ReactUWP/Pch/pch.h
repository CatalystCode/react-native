// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <SDKDDKVer.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef WINRT_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#endif

#ifndef NOGDI
#define NOGDI
#endif

#include <combaseapi.h>
#include <guiddef.h>
#include <intrin.h>
#include <unknwn.h>
#include <windows.h>

#include "CppWinRTIncludes.h"

#include <cxxreact/CxxModule.h>
#include <cxxreact/Instance.h>
#include <cxxreact/JSBigString.h>
#include <cxxreact/JSExecutor.h>
#include <fbsystrace.h>
#include <folly/Memory.h>
#include <folly/Optional.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <yoga/yoga.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
