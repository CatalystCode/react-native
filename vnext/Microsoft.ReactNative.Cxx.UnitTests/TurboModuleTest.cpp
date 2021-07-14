// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"

#include <sstream>
#include "Point.h"
#include "future/futureWait.h"

namespace ReactNativeTests {
REACT_MODULE(MyTurboModule)
struct MyTurboModule {
  REACT_INIT(Initialize)
  void Initialize(React::ReactContext const &context) noexcept {
    IsInitialized = true;
    TestCheck(context);

    // Event and Function fields are initialized before REACT_INIT method call.
    TestCheck(this->OnIntEvent != nullptr);
    TestCheck(this->JSIntFunction != nullptr);
  }

  REACT_METHOD(Add)
  int Add(int x, int y) noexcept {
    return x + y;
  }

  REACT_METHOD(Negate)
  int Negate(int x) noexcept {
    return -x;
  }

  REACT_METHOD(SayHello)
  std::string SayHello() noexcept {
    return "Hello";
  }

  REACT_METHOD(StaticAdd)
  static int StaticAdd(int x, int y) noexcept {
    return x + y;
  }

  REACT_METHOD(StaticNegate)
  static int StaticNegate(int x) noexcept {
    return -x;
  }

  REACT_METHOD(StaticSayHello)
  static std::string StaticSayHello() noexcept {
    return "Hello";
  }

  REACT_METHOD(SayHello0)
  void SayHello0() noexcept {
    Message = "Hello_0";
  }

  REACT_METHOD(PrintPoint)
  void PrintPoint(Point pt) noexcept {
    std::stringstream ss;
    ss << "Point: (" << pt.X << ", " << pt.Y << ")";
    Message = ss.str();
  }

  REACT_METHOD(PrintLine)
  void PrintLine(Point start, Point end) noexcept {
    std::stringstream ss;
    ss << "Line: (" << start.X << ", " << start.Y << ")-(" << end.X << ", " << end.Y << ")";
    Message = ss.str();
  }

  REACT_METHOD(StaticSayHello0)
  static void StaticSayHello0() noexcept {
    StaticMessage = "Hello_0";
  }

  REACT_METHOD(StaticPrintPoint)
  static void StaticPrintPoint(Point pt) noexcept {
    std::stringstream ss;
    ss << "Static Point: (" << pt.X << ", " << pt.Y << ")";
    StaticMessage = ss.str();
  }

  REACT_METHOD(StaticPrintLine)
  static void StaticPrintLine(Point start, Point end) noexcept {
    std::stringstream ss;
    ss << "Static Line: (" << start.X << ", " << start.Y << ")-(" << end.X << ", " << end.Y << ")";
    StaticMessage = ss.str();
  }

  REACT_METHOD(AddCallback)
  void AddCallback(int x, int y, std::function<void(int)> const &resolve) noexcept {
    resolve(x + y);
  }

  REACT_METHOD(NegateCallback)
  void NegateCallback(int x, std::function<void(int)> const &resolve) noexcept {
    resolve(-x);
  }

  REACT_METHOD(NegateAsyncCallback)
  fire_and_forget NegateAsyncCallback(int x, std::function<void(int)> resolve) noexcept {
    co_await winrt::resume_background();
    resolve(-x);
  }

  REACT_METHOD(NegateDispatchQueueCallback)
  void NegateDispatchQueueCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve]() noexcept { resolve(-x); });
  }

  REACT_METHOD(NegateFutureCallback)
  void NegateFutureCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::PostFuture([x, resolve]() noexcept { resolve(-x); });
  }

  REACT_METHOD(SayHelloCallback)
  void SayHelloCallback(std::function<void(const std::string &)> const &resolve) noexcept {
    resolve("Hello_2");
  }

  REACT_METHOD(StaticAddCallback)
  static void StaticAddCallback(int x, int y, std::function<void(int)> const &resolve) noexcept {
    resolve(x + y);
  }

  REACT_METHOD(StaticNegateCallback)
  static void StaticNegateCallback(int x, std::function<void(int)> const &resolve) noexcept {
    resolve(-x);
  }

  REACT_METHOD(StaticNegateAsyncCallback)
  static fire_and_forget StaticNegateAsyncCallback(int x, std::function<void(int)> resolve) noexcept {
    co_await winrt::resume_background();
    resolve(-x);
  }

  REACT_METHOD(StaticNegateDispatchQueueCallback)
  static void StaticNegateDispatchQueueCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve]() noexcept { resolve(-x); });
  }

  REACT_METHOD(StaticNegateFutureCallback)
  static void StaticNegateFutureCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::PostFuture([x, resolve]() noexcept { resolve(-x); });
  }

  REACT_METHOD(StaticSayHelloCallback)
  static void StaticSayHelloCallback(std::function<void(const std::string &)> const &resolve) noexcept {
    resolve("Static Hello_2");
  }

  REACT_METHOD(CallbackZeroArgs)
  void CallbackZeroArgs(std::function<void()> const &resolve) noexcept {
    resolve();
  }

  REACT_METHOD(CallbackTwoArgs)
  void CallbackTwoArgs(std::function<void(int, int)> const &resolve) noexcept {
    resolve(1, 2);
  }

  REACT_METHOD(CallbackThreeArgs)
  void CallbackThreeArgs(std::function<void(int, int, std::string const &)> const &resolve) noexcept {
    resolve(1, 2, "Hello");
  }

  REACT_METHOD(DivideCallbacks)
  void DivideCallbacks(
      int x,
      int y,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    if (y != 0) {
      resolve(x / y);
    } else {
      reject("Division by 0");
    }
  }

  REACT_METHOD(NegateCallbacks)
  void NegateCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(NegateAsyncCallbacks)
  fire_and_forget NegateAsyncCallbacks(
      int x,
      std::function<void(int)> resolve,
      std::function<void(std::string const &)> reject) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(NegateDispatchQueueCallbacks)
  void NegateDispatchQueueCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(NegateFutureCallbacks)
  void NegateFutureCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::PostFuture([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(ResolveSayHelloCallbacks)
  void ResolveSayHelloCallbacks(
      std::function<void(std::string const &)> const &resolve,
      std::function<void(std::string const &)> const & /*reject*/) noexcept {
    resolve("Hello_3");
  }

  REACT_METHOD(RejectSayHelloCallbacks)
  void RejectSayHelloCallbacks(
      std::function<void(std::string const &)> const & /*resolve*/,
      std::function<void(std::string const &)> const &reject) noexcept {
    reject("Goodbye");
  }

  REACT_METHOD(StaticDivideCallbacks)
  static void StaticDivideCallbacks(
      int x,
      int y,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    if (y != 0) {
      resolve(x / y);
    } else {
      reject("Division by 0");
    }
  }

  REACT_METHOD(StaticNegateCallbacks)
  static void StaticNegateCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(StaticNegateAsyncCallbacks)
  static fire_and_forget StaticNegateAsyncCallbacks(
      int x,
      std::function<void(int)> resolve,
      std::function<void(std::string const &)> reject) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(StaticNegateDispatchQueueCallbacks)
  static void StaticNegateDispatchQueueCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(StaticNegateFutureCallbacks)
  static void StaticNegateFutureCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::PostFuture([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(StaticResolveSayHelloCallbacks)
  static void StaticResolveSayHelloCallbacks(
      std::function<void(std::string const &)> const &resolve,
      std::function<void(std::string const &)> const & /*reject*/) noexcept {
    resolve("Hello_3");
  }

  REACT_METHOD(StaticRejectSayHelloCallbacks)
  static void StaticRejectSayHelloCallbacks(
      std::function<void(std::string const &)> const & /*resolve*/,
      std::function<void(std::string const &)> const &reject) noexcept {
    reject("Goodbye");
  }

  REACT_METHOD(TwoCallbacksZeroArgs1)
  void TwoCallbacksZeroArgs1(std::function<void()> const &resolve, std::function<void()> const & /*reject*/) noexcept {
    resolve();
  }

  REACT_METHOD(TwoCallbacksZeroArgs2)
  void TwoCallbacksZeroArgs2(std::function<void()> const & /*resolve*/, std::function<void()> const &reject) noexcept {
    reject();
  }

  REACT_METHOD(TwoCallbacksTwoArgs1)
  void TwoCallbacksTwoArgs1(
      std::function<void(int, int)> const &resolve,
      std::function<void(int, int)> const & /*reject*/) noexcept {
    resolve(1, 2);
  }

  REACT_METHOD(TwoCallbacksTwoArgs2)
  void TwoCallbacksTwoArgs2(
      std::function<void(int, int)> const & /*resolve*/,
      std::function<void(int, int)> const &reject) noexcept {
    reject(1, 2);
  }

  REACT_METHOD(TwoCallbacksThreeArgs1)
  void TwoCallbacksThreeArgs1(
      std::function<void(int, int, std::string const &)> const &resolve,
      std::function<void(int, int, std::string const &)> const & /*reject*/) noexcept {
    resolve(1, 2, "Hello");
  }

  REACT_METHOD(TwoCallbacksThreeArgs2)
  void TwoCallbacksThreeArgs2(
      std::function<void(int, int, std::string const &)> const & /*resolve*/,
      std::function<void(int, int, std::string const &)> const &reject) noexcept {
    reject(1, 2, "Hello");
  }

  REACT_METHOD(DividePromise)
  void DividePromise(int x, int y, React::ReactPromise<int> const &result) noexcept {
    if (y != 0) {
      result.Resolve(x / y);
    } else {
      React::ReactError error{};
      error.Message = "Division by 0";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegatePromise)
  void NegatePromise(int x, React::ReactPromise<int> const &result) noexcept {
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegateAsyncPromise)
  fire_and_forget NegateAsyncPromise(int x, React::ReactPromise<int> result) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegateDispatchQueuePromise)
  void NegateDispatchQueuePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  REACT_METHOD(NegateFuturePromise)
  void NegateFuturePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::PostFuture([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  // Each macro has second optional parameter: JS name.
  REACT_METHOD(VoidPromise, L"voidPromise")
  void VoidPromise(int x, React::ReactPromise<void> const &result) noexcept {
    if (x % 2 == 0) {
      result.Resolve();
    } else {
      result.Reject("Odd unexpected");
    }
  }

  REACT_METHOD(ResolveSayHelloPromise)
  void ResolveSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    result.Resolve("Hello_4");
  }

  REACT_METHOD(RejectSayHelloPromise)
  void RejectSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    React::ReactError error{};
    error.Message = "Promise rejected";
    result.Reject(std::move(error));
  }

  REACT_METHOD(StaticDividePromise)
  static void StaticDividePromise(int x, int y, React::ReactPromise<int> const &result) noexcept {
    if (y != 0) {
      result.Resolve(x / y);
    } else {
      React::ReactError error{};
      error.Message = "Division by 0";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegatePromise)
  static void StaticNegatePromise(int x, React::ReactPromise<int> const &result) noexcept {
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegateAsyncPromise)
  static fire_and_forget StaticNegateAsyncPromise(int x, React::ReactPromise<int> result) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegateDispatchQueuePromise)
  static void StaticNegateDispatchQueuePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  REACT_METHOD(StaticNegateFuturePromise)
  static void StaticNegateFuturePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::PostFuture([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  // Each macro has second optional parameter: JS name.
  REACT_METHOD(StaticVoidPromise, L"staticVoidPromise")
  void StaticVoidPromise(int x, React::ReactPromise<void> const &result) noexcept {
    if (x % 2 == 0) {
      result.Resolve();
    } else {
      result.Reject("Odd unexpected");
    }
  }

  REACT_METHOD(StaticResolveSayHelloPromise)
  static void StaticResolveSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    result.Resolve("Hello_4");
  }

  REACT_METHOD(StaticRejectSayHelloPromise)
  static void StaticRejectSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    React::ReactError error{};
    error.Message = "Promise rejected";
    result.Reject(std::move(error));
  }

  REACT_SYNC_METHOD(AddSync)
  int AddSync(int x, int y) noexcept {
    return x + y;
  }

  REACT_SYNC_METHOD(NegateSync)
  int NegateSync(int x) noexcept {
    return -x;
  }

  REACT_SYNC_METHOD(SayHelloSync)
  std::string SayHelloSync() noexcept {
    return "Hello";
  }

  REACT_SYNC_METHOD(StaticAddSync)
  static int StaticAddSync(int x, int y) noexcept {
    return x + y;
  }

  REACT_SYNC_METHOD(StaticNegateSync)
  static int StaticNegateSync(int x) noexcept {
    return -x;
  }

  REACT_SYNC_METHOD(StaticSayHelloSync)
  static std::string StaticSayHelloSync() noexcept {
    return "Hello";
  }

  REACT_CONSTANT(Constant1)
  const std::string Constant1{"MyConstant1"};

  REACT_CONSTANT(Constant2, L"const2")
  const std::string Constant2{"MyConstant2"};

  REACT_CONSTANT(Constant3, L"const3")
  static constexpr Point Constant3{/*X =*/2, /*Y =*/3};

  REACT_CONSTANT(Constant4)
  static constexpr Point Constant4{/*X =*/3, /*Y =*/4};

  REACT_CONSTANT_PROVIDER(Constant5)
  void Constant5(React::ReactConstantProvider &provider) noexcept {
    provider.Add(L"const51", Point{/*X =*/12, /*Y =*/14});
    provider.Add(L"const52", "MyConstant52");
  }

  REACT_CONSTANT_PROVIDER(Constant6)
  static void Constant6(React::ReactConstantProvider &provider) noexcept {
    provider.Add(L"const61", Point{/*X =*/15, /*Y =*/17});
    provider.Add(L"const62", "MyConstant62");
  }

  // Allows to emit native module events
  REACT_EVENT(OnIntEvent)
  std::function<void(int)> OnIntEvent;

  // An event without arguments
  REACT_EVENT(OnNoArgEvent)
  std::function<void()> OnNoArgEvent;

  // An event with two arguments
  REACT_EVENT(OnTwoArgsEvent)
  std::function<void(Point const &, Point const &)> OnTwoArgsEvent;

  // Specify event name different from the field name.
  REACT_EVENT(OnPointEvent, L"onPointEvent")
  std::function<void(Point const &)> OnPointEvent;

  // By default we use the event emitter name from REACT_MODULE which is by default 'RCTDeviceEventEmitter'.
  // Here we specify event emitter name local for this event.
  REACT_EVENT(OnStringEvent, L"onStringEvent", L"MyEventEmitter")
  std::function<void(char const *)> OnStringEvent;

  // Use React::JSValue which is an immutable JSON-like data representation.
  REACT_EVENT(OnJSValueEvent)
  std::function<void(const React::JSValue &)> OnJSValueEvent;

  // Allows to call JS functions.
  REACT_FUNCTION(JSIntFunction)
  std::function<void(int)> JSIntFunction;

  // Specify JS function name different from the field name.
  REACT_FUNCTION(JSPointFunction, L"pointFunc")
  std::function<void(Point const &)> JSPointFunction;

  // Use two arguments. Specify JS function name different from the field name.
  REACT_FUNCTION(JSLineFunction, L"lineFunc")
  std::function<void(Point const &, Point const &)> JSLineFunction;

  // Use no arguments.
  REACT_FUNCTION(JSNoArgFunction)
  std::function<void()> JSNoArgFunction;

  // By default we use the module name from REACT_MODULE which is by default the struct name.
  // Here we specify module name local for this function.
  REACT_FUNCTION(JSStringFunction, L"stringFunc", L"MyModule")
  std::function<void(char const *)> JSStringFunction;

  // Use React::JSValue which is an immutable JSON-like data representation.
  REACT_FUNCTION(JSValueFunction)
  std::function<void(const React::JSValue &)> JSValueFunction;

 public: // Used to report some test messages
  bool IsInitialized{false};
  std::string Message;
  static std::string StaticMessage;
};

/*static*/ std::string MyTurboModule::StaticMessage;

// The TurboModule spec is going to be generated from the Flow spec file.
// It verifies that:
// - module methods names are unique;
// - method names are matching to the module spec method names;
// - method signatures match the spec method signatures.
struct MyTurboModuleSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(int, int, Callback<int>) noexcept>{0, L"Add"},
      Method<void(int, Callback<int>) noexcept>{1, L"Negate"},
      Method<void(Callback<std::string>) noexcept>{2, L"SayHello"},
      Method<void(int, int, Callback<int>) noexcept>{3, L"StaticAdd"},
      Method<void(int, Callback<int>) noexcept>{4, L"StaticNegate"},
      Method<void(Callback<std::string>) noexcept>{5, L"StaticSayHello"},
      Method<void() noexcept>{6, L"SayHello0"},
      Method<void(Point) noexcept>{7, L"PrintPoint"},
      Method<void(Point, Point) noexcept>{8, L"PrintLine"},
      Method<void() noexcept>{9, L"StaticSayHello0"},
      Method<void(Point) noexcept>{10, L"StaticPrintPoint"},
      Method<void(Point, Point) noexcept>{11, L"StaticPrintLine"},
      Method<void(int, int, Callback<int>) noexcept>{12, L"AddCallback"},
      Method<void(int, Callback<int>) noexcept>{13, L"NegateCallback"},
      Method<void(int, Callback<int>) noexcept>{14, L"NegateAsyncCallback"},
      Method<void(int, Callback<int>) noexcept>{15, L"NegateDispatchQueueCallback"},
      Method<void(int, Callback<int>) noexcept>{16, L"NegateFutureCallback"},
      Method<void(Callback<std::string>) noexcept>{17, L"SayHelloCallback"},
      Method<void(int, int, Callback<int>) noexcept>{18, L"StaticAddCallback"},
      Method<void(int, Callback<int>) noexcept>{19, L"StaticNegateCallback"},
      Method<void(int, Callback<int>) noexcept>{20, L"StaticNegateAsyncCallback"},
      Method<void(int, Callback<int>) noexcept>{21, L"StaticNegateDispatchQueueCallback"},
      Method<void(int, Callback<int>) noexcept>{22, L"StaticNegateFutureCallback"},
      Method<void(Callback<std::string>) noexcept>{23, L"StaticSayHelloCallback"},
      Method<void(Callback<>) noexcept>{24, L"CallbackZeroArgs"},
      Method<void(Callback<int, int>) noexcept>{25, L"CallbackTwoArgs"},
      Method<void(Callback<int, int, std::string>) noexcept>{26, L"CallbackThreeArgs"},
      Method<void(int, int, Callback<int>, Callback<std::string>) noexcept>{27, L"DivideCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{28, L"NegateCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{29, L"NegateAsyncCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{30, L"NegateDispatchQueueCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{31, L"NegateFutureCallbacks"},
      Method<void(Callback<std::string>, Callback<std::string>) noexcept>{32, L"ResolveSayHelloCallbacks"},
      Method<void(Callback<std::string>, Callback<std::string>) noexcept>{33, L"RejectSayHelloCallbacks"},
      Method<void(int, int, Callback<int>, Callback<std::string>) noexcept>{34, L"StaticDivideCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{35, L"StaticNegateCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{36, L"StaticNegateAsyncCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{37, L"StaticNegateDispatchQueueCallbacks"},
      Method<void(int, Callback<int>, Callback<std::string>) noexcept>{38, L"StaticNegateFutureCallbacks"},
      Method<void(Callback<std::string>, Callback<std::string>) noexcept>{39, L"StaticResolveSayHelloCallbacks"},
      Method<void(Callback<std::string>, Callback<std::string>) noexcept>{40, L"StaticRejectSayHelloCallbacks"},
      Method<void(Callback<>, Callback<>) noexcept>{41, L"TwoCallbacksZeroArgs1"},
      Method<void(Callback<>, Callback<>) noexcept>{42, L"TwoCallbacksZeroArgs2"},
      Method<void(Callback<int, int>, Callback<int, int>) noexcept>{43, L"TwoCallbacksTwoArgs1"},
      Method<void(Callback<int, int>, Callback<int, int>) noexcept>{44, L"TwoCallbacksTwoArgs2"},
      Method<void(Callback<int, int, std::string>, Callback<int, int, std::string>) noexcept>{
          45,
          L"TwoCallbacksThreeArgs1"},
      Method<void(Callback<int, int, std::string>, Callback<int, int, std::string>) noexcept>{
          46,
          L"TwoCallbacksThreeArgs2"},
      Method<void(int, int, Promise<int>) noexcept>{47, L"DividePromise"},
      Method<void(int, Promise<int>) noexcept>{48, L"NegatePromise"},
      Method<void(int, Promise<int>) noexcept>{49, L"NegateAsyncPromise"},
      Method<void(int, Promise<int>) noexcept>{50, L"NegateDispatchQueuePromise"},
      Method<void(int, Promise<int>) noexcept>{51, L"NegateFuturePromise"},
      Method<void(int, Promise<void>) noexcept>{52, L"voidPromise"},
      Method<void(Promise<std::string>) noexcept>{53, L"ResolveSayHelloPromise"},
      Method<void(Promise<std::string>) noexcept>{54, L"RejectSayHelloPromise"},
      Method<void(int, int, Promise<int>) noexcept>{55, L"StaticDividePromise"},
      Method<void(int, Promise<int>) noexcept>{56, L"StaticNegatePromise"},
      Method<void(int, Promise<int>) noexcept>{57, L"StaticNegateAsyncPromise"},
      Method<void(int, Promise<int>) noexcept>{58, L"StaticNegateDispatchQueuePromise"},
      Method<void(int, Promise<int>) noexcept>{59, L"StaticNegateFuturePromise"},
      Method<void(int, Promise<void>) noexcept>{60, L"staticVoidPromise"},
      Method<void(Promise<std::string>) noexcept>{61, L"StaticResolveSayHelloPromise"},
      Method<void(Promise<std::string>) noexcept>{62, L"StaticRejectSayHelloPromise"},
      SyncMethod<int(int, int) noexcept>{63, L"AddSync"},
      SyncMethod<int(int) noexcept>{64, L"NegateSync"},
      SyncMethod<std::string() noexcept>{65, L"SayHelloSync"},
      SyncMethod<int(int, int) noexcept>{66, L"StaticAddSync"},
      SyncMethod<int(int) noexcept>{67, L"StaticNegateSync"},
      SyncMethod<std::string() noexcept>{68, L"StaticSayHelloSync"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, MyTurboModuleSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
        0,
        "Add",
        "    REACT_METHOD(Add) int Add(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) void Add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) winrt::fire_and_forget Add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) static int Add(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) static void Add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) static React::Coroutine Add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        1,
        "Negate",
        "    REACT_METHOD(Negate) int Negate(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) void Negate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) winrt::fire_and_forget Negate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) static int Negate(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) static void Negate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) static winrt::fire_and_forget Negate(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        2,
        "SayHello",
        "    REACT_METHOD(SayHello) std::string SayHello() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) void SayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) winrt::fire_and_forget SayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) static std::string SayHello() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) static void SayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) static winrt::fire_and_forget SayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        3,
        "StaticAdd",
        "    REACT_METHOD(StaticAdd) int StaticAdd(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) void StaticAdd(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) winrt::fire_and_forget StaticAdd(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) static int StaticAdd(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) static void StaticAdd(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) static winrt::fire_and_forget StaticAdd(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        4,
        "StaticNegate",
        "    REACT_METHOD(StaticNegate) int StaticNegate(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) void StaticNegate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) winrt::fire_and_forget StaticNegate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) static int StaticNegate(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) static void StaticNegate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) static winrt::fire_and_forget StaticNegate(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        5,
        "StaticSayHello",
        "    REACT_METHOD(StaticSayHello) std::string StaticSayHello() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) void StaticSayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) winrt::fire_and_forget StaticSayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) static std::string StaticSayHello() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) static void StaticSayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) static winrt::fire_and_forget StaticSayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        6,
        "SayHello0",
        "    REACT_METHOD(SayHello0) void SayHello0() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello0) winrt::fire_and_forget SayHello0() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello0) static void SayHello0() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello0) static winrt::fire_and_forget SayHello0() noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        7,
        "PrintPoint",
        "    REACT_METHOD(PrintPoint) void PrintPoint(Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(PrintPoint) winrt::fire_and_forget PrintPoint(Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(PrintPoint) static void PrintPoint(Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(PrintPoint) static winrt::fire_and_forget PrintPoint(Point) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        8,
        "PrintLine",
        "    REACT_METHOD(PrintPoint) void PrintLine(Point, Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(PrintPoint) winrt::fire_and_forget PrintLine(Point, Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(PrintPoint) static void PrintLine(Point, Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(PrintPoint) static winrt::fire_and_forget PrintLine(Point, Point) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        9,
        "StaticSayHello0",
        "    REACT_METHOD(StaticSayHello0) void StaticSayHello0() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello0) winrt::fire_and_forget StaticSayHello0() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello0) static void StaticSayHello0() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello0) static winrt::fire_and_forget StaticSayHello0() noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        10,
        "StaticPrintPoint",
        "    REACT_METHOD(StaticPrintPoint) void StaticPrintPoint(Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticPrintPoint) winrt::fire_and_forget StaticPrintPoint(Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticPrintPoint) static void StaticPrintPoint(Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticPrintPoint) static winrt::fire_and_forget StaticPrintPoint(Point) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        11,
        "StaticPrintLine",
        "    REACT_METHOD(StaticPrintPoint) void StaticPrintLine(Point, Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticPrintPoint) winrt::fire_and_forget StaticPrintLine(Point, Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticPrintPoint) static void StaticPrintLine(Point, Point) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticPrintPoint) static winrt::fire_and_forget StaticPrintLine(Point, Point) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        12,
        "AddCallback",
        "    REACT_METHOD(AddCallback) int AddCallback(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(AddCallback) void AddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(AddCallback) winrt::fire_and_forget AddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(AddCallback) static int AddCallback(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(AddCallback) static void AddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(AddCallback) static winrt::fire_and_forget AddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        13,
        "NegateCallback",
        "    REACT_METHOD(NegateCallback) int NegateCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallback) void NegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallback) winrt::fire_and_forget NegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallback) static int NegateCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallback) static void NegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallback) static winrt::fire_and_forget NegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        14,
        "NegateAsyncCallback",
        "    REACT_METHOD(NegateAsyncCallback) int NegateAsyncCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallback) void NegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallback) winrt::fire_and_forget NegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallback) static int NegateAsyncCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallback) static void NegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallback) static winrt::fire_and_forget NegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        15,
        "NegateDispatchQueueCallback",
        "    REACT_METHOD(NegateDispatchQueueCallback) int NegateDispatchQueueCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallback) void NegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallback) winrt::fire_and_forget NegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallback) static int NegateDispatchQueueCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallback) static void NegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallback) static winrt::fire_and_forget NegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        16,
        "NegateFutureCallback",
        "    REACT_METHOD(NegateFutureCallback) int NegateFutureCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallback) void NegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallback) winrt::fire_and_forget NegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallback) static int NegateFutureCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallback) static void NegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallback) static winrt::fire_and_forget NegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        17,
        "SayHelloCallback",
        "    REACT_METHOD(SayHelloCallback) std::string SayHelloCallback() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHelloCallback) void SayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHelloCallback) winrt::fire_and_forget SayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHelloCallback) static std::string SayHelloCallback() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHelloCallback) static void SayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHelloCallback) static winrt::fire_and_forget SayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        18,
        "StaticAddCallback",
        "    REACT_METHOD(StaticAddCallback) int StaticAddCallback(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAddCallback) void StaticAddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAddCallback) winrt::fire_and_forget StaticAddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAddCallback) static int StaticAddCallback(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAddCallback) static void StaticAddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAddCallback) static winrt::fire_and_forget StaticAddCallback(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        19,
        "StaticNegateCallback",
        "    REACT_METHOD(StaticNegateCallback) int StaticNegateCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallback) void StaticNegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallback) winrt::fire_and_forget StaticNegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallback) static int StaticNegateCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallback) static void StaticNegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallback) static winrt::fire_and_forget StaticNegateCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        20,
        "StaticNegateAsyncCallback",
        "    REACT_METHOD(StaticNegateAsyncCallback) int StaticNegateAsyncCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallback) void StaticNegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallback) winrt::fire_and_forget StaticNegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallback) static int StaticNegateAsyncCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallback) static void StaticNegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallback) static winrt::fire_and_forget StaticNegateAsyncCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        21,
        "StaticNegateDispatchQueueCallback",
        "    REACT_METHOD(StaticNegateDispatchQueueCallback) int StaticNegateDispatchQueueCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallback) void StaticNegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallback) winrt::fire_and_forget StaticNegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallback) static int StaticNegateDispatchQueueCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallback) static void StaticNegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallback) static winrt::fire_and_forget StaticNegateDispatchQueueCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        22,
        "StaticNegateFutureCallback",
        "    REACT_METHOD(StaticNegateFutureCallback) int StaticNegateFutureCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallback) void StaticNegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallback) winrt::fire_and_forget StaticNegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallback) static int StaticNegateFutureCallback(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallback) static void StaticNegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallback) static winrt::fire_and_forget StaticNegateFutureCallback(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        23,
        "StaticSayHelloCallback",
        "    REACT_METHOD(StaticSayHelloCallback) std::string StaticSayHelloCallback() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHelloCallback) void StaticSayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHelloCallback) winrt::fire_and_forget StaticSayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHelloCallback) static std::string StaticSayHelloCallback() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHelloCallback) static void StaticSayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHelloCallback) static winrt::fire_and_forget StaticSayHelloCallback(ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        24,
        "CallbackZeroArgs",
        "    REACT_METHOD(CallbackZeroArgs) void CallbackZeroArgs(ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackZeroArgs) winrt::fire_and_forget CallbackZeroArgs(ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackZeroArgs) static void CallbackZeroArgs(ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackZeroArgs) static winrt::fire_and_forget CallbackZeroArgs(ReactCallback<>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        25,
        "CallbackTwoArgs",
        "    REACT_METHOD(CallbackTwoArgs) void CallbackTwoArgs(ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackTwoArgs) winrt::fire_and_forget CallbackTwoArgs(ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackTwoArgs) static void CallbackTwoArgs(ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackTwoArgs) static winrt::fire_and_forget CallbackTwoArgs(ReactCallback<int, int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        26,
        "CallbackThreeArgs",
        "    REACT_METHOD(CallbackThreeArgs) void CallbackThreeArgs(ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackThreeArgs) winrt::fire_and_forget CallbackThreeArgs(ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackThreeArgs) static void CallbackThreeArgs(ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(CallbackThreeArgs) static winrt::fire_and_forget CallbackThreeArgs(ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        27,
        "DivideCallbacks",
        "    REACT_METHOD(DivideCallbacks) void DivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(DivideCallbacks) winrt::fire_and_forget DivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(DivideCallbacks) static void DivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(DivideCallbacks) static winrt::fire_and_forget DivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        28,
        "NegateCallbacks",
        "    REACT_METHOD(NegateCallbacks) void NegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallbacks) winrt::fire_and_forget NegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallbacks) static void NegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateCallbacks) static winrt::fire_and_forget NegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        29,
        "NegateAsyncCallbacks",
        "    REACT_METHOD(NegateAsyncCallbacks) void NegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallbacks) winrt::fire_and_forget NegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallbacks) static void NegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncCallbacks) static winrt::fire_and_forget NegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        30,
        "NegateDispatchQueueCallbacks",
        "    REACT_METHOD(NegateDispatchQueueCallbacks) void NegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallbacks) winrt::fire_and_forget NegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallbacks) static void NegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueueCallbacks) static winrt::fire_and_forget NegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        31,
        "NegateFutureCallbacks",
        "    REACT_METHOD(NegateFutureCallbacks) void NegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallbacks) winrt::fire_and_forget NegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallbacks) static void NegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFutureCallbacks) static winrt::fire_and_forget NegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        32,
        "ResolveSayHelloCallbacks",
        "    REACT_METHOD(ResolveSayHelloCallbacks) void ResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(ResolveSayHelloCallbacks) winrt::fire_and_forget ResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(ResolveSayHelloCallbacks) static void ResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(ResolveSayHelloCallbacks) static winrt::fire_and_forget ResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        33,
        "RejectSayHelloCallbacks",
        "    REACT_METHOD(RejectSayHelloCallbacks) void RejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(RejectSayHelloCallbacks) winrt::fire_and_forget RejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(RejectSayHelloCallbacks) static void RejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(RejectSayHelloCallbacks) static winrt::fire_and_forget RejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        34,
        "StaticDivideCallbacks",
        "    REACT_METHOD(StaticDivideCallbacks) void StaticDivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticDivideCallbacks) winrt::fire_and_forget StaticDivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticDivideCallbacks) static void StaticDivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticDivideCallbacks) static winrt::fire_and_forget StaticDivideCallbacks(int, int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        35,
        "StaticNegateCallbacks",
        "    REACT_METHOD(StaticNegateCallbacks) void StaticNegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallbacks) winrt::fire_and_forget StaticNegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallbacks) static void StaticNegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateCallbacks) static winrt::fire_and_forget StaticNegateCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        36,
        "StaticNegateAsyncCallbacks",
        "    REACT_METHOD(StaticNegateAsyncCallbacks) void StaticNegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallbacks) winrt::fire_and_forget StaticNegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallbacks) static void StaticNegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncCallbacks) static winrt::fire_and_forget StaticNegateAsyncCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        37,
        "StaticNegateDispatchQueueCallbacks",
        "    REACT_METHOD(StaticNegateDispatchQueueCallbacks) void StaticNegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallbacks) winrt::fire_and_forget StaticNegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallbacks) static void StaticNegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueueCallbacks) static winrt::fire_and_forget StaticNegateDispatchQueueCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        38,
        "StaticNegateFutureCallbacks",
        "    REACT_METHOD(StaticNegateFutureCallbacks) void StaticNegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallbacks) winrt::fire_and_forget StaticNegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallbacks) static void StaticNegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFutureCallbacks) static winrt::fire_and_forget StaticNegateFutureCallbacks(int, ReactCallback<int>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        39,
        "StaticResolveSayHelloCallbacks",
        "    REACT_METHOD(StaticResolveSayHelloCallbacks) void StaticResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticResolveSayHelloCallbacks) winrt::fire_and_forget StaticResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticResolveSayHelloCallbacks) static void StaticResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticResolveSayHelloCallbacks) static winrt::fire_and_forget StaticResolveSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        40,
        "StaticRejectSayHelloCallbacks",
        "    REACT_METHOD(StaticRejectSayHelloCallbacks) void StaticRejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticRejectSayHelloCallbacks) winrt::fire_and_forget StaticRejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticRejectSayHelloCallbacks) static void StaticRejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticRejectSayHelloCallbacks) static winrt::fire_and_forget StaticRejectSayHelloCallbacks(ReactCallback<std::string>, ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        41,
        "TwoCallbacksZeroArgs1",
        "    REACT_METHOD(TwoCallbacksZeroArgs1) void TwoCallbacksZeroArgs1(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksZeroArgs1) winrt::fire_and_forget TwoCallbacksZeroArgs1(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksZeroArgs1) static void TwoCallbacksZeroArgs1(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksZeroArgs1) static winrt::fire_and_forget TwoCallbacksZeroArgs1(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        42,
        "TwoCallbacksZeroArgs2",
        "    REACT_METHOD(TwoCallbacksZeroArgs2) void TwoCallbacksZeroArgs2(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksZeroArgs2) winrt::fire_and_forget TwoCallbacksZeroArgs2(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksZeroArgs2) static void TwoCallbacksZeroArgs2(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksZeroArgs2) static winrt::fire_and_forget TwoCallbacksZeroArgs2(ReactCallback<>, ReactCallback<>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        43,
        "TwoCallbacksTwoArgs1",
        "    REACT_METHOD(TwoCallbacksTwoArgs1) void TwoCallbacksTwoArgs1(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksTwoArgs1) winrt::fire_and_forget TwoCallbacksTwoArgs1(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksTwoArgs1) static void TwoCallbacksTwoArgs1(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksTwoArgs1) static winrt::fire_and_forget TwoCallbacksTwoArgs1(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        44,
        "TwoCallbacksTwoArgs2",
        "    REACT_METHOD(TwoCallbacksTwoArgs2) void TwoCallbacksTwoArgs2(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksTwoArgs2) winrt::fire_and_forget TwoCallbacksTwoArgs2(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksTwoArgs2) static void TwoCallbacksTwoArgs2(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksTwoArgs2) static winrt::fire_and_forget TwoCallbacksTwoArgs2(ReactCallback<int, int>, ReactCallback<int, int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        45,
        "TwoCallbacksThreeArgs1",
        "    REACT_METHOD(TwoCallbacksThreeArgs1) void TwoCallbacksThreeArgs1(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksThreeArgs1) winrt::fire_and_forget TwoCallbacksThreeArgs1(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksThreeArgs1) static void TwoCallbacksThreeArgs1(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksThreeArgs1) static winrt::fire_and_forget TwoCallbacksThreeArgs1(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        46,
        "TwoCallbacksThreeArgs2",
        "    REACT_METHOD(TwoCallbacksThreeArgs2) void TwoCallbacksThreeArgs2(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksThreeArgs2) winrt::fire_and_forget TwoCallbacksThreeArgs2(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksThreeArgs2) static void TwoCallbacksThreeArgs2(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(TwoCallbacksThreeArgs2) static winrt::fire_and_forget TwoCallbacksThreeArgs2(ReactCallback<int, int, std::string>, ReactCallback<int, int, std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        47,
        "DividePromise",
        "    REACT_METHOD(DividePromise) void DividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(DividePromise) winrt::fire_and_forget DividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(DividePromise) static void DividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(DividePromise) static winrt::fire_and_forget DividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        48,
        "NegatePromise",
        "    REACT_METHOD(NegatePromise) void NegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegatePromise) winrt::fire_and_forget NegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegatePromise) static void NegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegatePromise) static winrt::fire_and_forget NegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        49,
        "NegateAsyncPromise",
        "    REACT_METHOD(NegateAsyncPromise) void NegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncPromise) winrt::fire_and_forget NegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncPromise) static void NegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateAsyncPromise) static winrt::fire_and_forget NegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        50,
        "NegateDispatchQueuePromise",
        "    REACT_METHOD(NegateDispatchQueuePromise) void NegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueuePromise) winrt::fire_and_forget NegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueuePromise) static void NegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateDispatchQueuePromise) static winrt::fire_and_forget NegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        51,
        "NegateFuturePromise",
        "    REACT_METHOD(NegateFuturePromise) void NegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFuturePromise) winrt::fire_and_forget NegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFuturePromise) static void NegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateFuturePromise) static winrt::fire_and_forget NegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        52,
        "voidPromise",
        "    REACT_METHOD(voidPromise) void voidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(voidPromise) winrt::fire_and_forget voidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(voidPromise) static void voidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(voidPromise) static winrt::fire_and_forget voidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        53,
        "ResolveSayHelloPromise",
        "    REACT_METHOD(ResolveSayHelloPromise) void ResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(ResolveSayHelloPromise) winrt::fire_and_forget ResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(ResolveSayHelloPromise) static void ResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(ResolveSayHelloPromise) static winrt::fire_and_forget ResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        54,
        "RejectSayHelloPromise",
        "    REACT_METHOD(RejectSayHelloPromise) void RejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(RejectSayHelloPromise) winrt::fire_and_forget RejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(RejectSayHelloPromise) static void RejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(RejectSayHelloPromise) static winrt::fire_and_forget RejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        55,
        "StaticDividePromise",
        "    REACT_METHOD(StaticDividePromise) void StaticDividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticDividePromise) winrt::fire_and_forget StaticDividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticDividePromise) static void StaticDividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticDividePromise) static winrt::fire_and_forget StaticDividePromise(int, int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        56,
        "StaticNegatePromise",
        "    REACT_METHOD(StaticNegatePromise) void StaticNegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegatePromise) winrt::fire_and_forget StaticNegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegatePromise) static void StaticNegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegatePromise) static winrt::fire_and_forget StaticNegatePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        57,
        "StaticNegateAsyncPromise",
        "    REACT_METHOD(StaticNegateAsyncPromise) void StaticNegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncPromise) winrt::fire_and_forget StaticNegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncPromise) static void StaticNegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateAsyncPromise) static winrt::fire_and_forget StaticNegateAsyncPromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        58,
        "StaticNegateDispatchQueuePromise",
        "    REACT_METHOD(StaticNegateDispatchQueuePromise) void StaticNegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueuePromise) winrt::fire_and_forget StaticNegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueuePromise) static void StaticNegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateDispatchQueuePromise) static winrt::fire_and_forget StaticNegateDispatchQueuePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        59,
        "StaticNegateFuturePromise",
        "    REACT_METHOD(StaticNegateFuturePromise) void StaticNegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFuturePromise) winrt::fire_and_forget StaticNegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFuturePromise) static void StaticNegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateFuturePromise) static winrt::fire_and_forget StaticNegateFuturePromise(int, ReactPromise<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        60,
        "staticVoidPromise",
        "    REACT_METHOD(staticVoidPromise) void staticVoidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(staticVoidPromise) winrt::fire_and_forget staticVoidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(staticVoidPromise) static void staticVoidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(staticVoidPromise) static winrt::fire_and_forget staticVoidPromise(int, ReactPromise<void>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        61,
        "StaticResolveSayHelloPromise",
        "    REACT_METHOD(StaticResolveSayHelloPromise) void StaticResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticResolveSayHelloPromise) winrt::fire_and_forget StaticResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticResolveSayHelloPromise) static void StaticResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticResolveSayHelloPromise) static winrt::fire_and_forget StaticResolveSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        62,
        "StaticRejectSayHelloPromise",
        "    REACT_METHOD(StaticRejectSayHelloPromise) void StaticRejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticRejectSayHelloPromise) winrt::fire_and_forget StaticRejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticRejectSayHelloPromise) static void StaticRejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticRejectSayHelloPromise) static winrt::fire_and_forget StaticRejectSayHelloPromise(ReactPromise<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_SYNC_METHOD_SPEC_ERRORS(
        63,
        "AddSync",
        "    REACT_METHOD(AddSync) int AddSync(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(AddSync) static int AddSync(int, int) noexcept {/*implementation*/}\n");
    REACT_SHOW_SYNC_METHOD_SPEC_ERRORS(
        64,
        "NegateSync",
        "    REACT_METHOD(NegateSync) int NegateSync(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(NegateSync) static int NegateSync(int) noexcept {/*implementation*/}\n");
    REACT_SHOW_SYNC_METHOD_SPEC_ERRORS(
        65,
        "SayHelloSync",
        "    REACT_METHOD(SayHelloSync) std::string SayHelloSync() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHelloSync) static std::string SayHelloSync() noexcept {/*implementation*/}\n");
    REACT_SHOW_SYNC_METHOD_SPEC_ERRORS(
        66,
        "StaticAddSync",
        "    REACT_METHOD(StaticAddSync) int StaticAddSync(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAddSync) static int StaticAddSync(int, int) noexcept {/*implementation*/}\n");
    REACT_SHOW_SYNC_METHOD_SPEC_ERRORS(
        67,
        "StaticNegateSync",
        "    REACT_METHOD(StaticNegateSync) int StaticNegateSync(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegateSync) static int StaticNegateSync(int) noexcept {/*implementation*/}\n");
    REACT_SHOW_SYNC_METHOD_SPEC_ERRORS(
        68,
        "StaticSayHelloSync",
        "    REACT_METHOD(StaticSayHelloSync) std::string StaticSayHelloSync() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHelloSync) static std::string StaticSayHelloSync() noexcept {/*implementation*/}\n");
  }
};

TEST_CLASS (TurboModuleTest) {
  React::ReactModuleBuilderMock m_builderMock{};
  React::IReactModuleBuilder m_moduleBuilder;
  Windows::Foundation::IInspectable m_moduleObject{nullptr};
  MyTurboModule *m_module;

  TurboModuleTest() {
    m_moduleBuilder = winrt::make<React::ReactModuleBuilderImpl>(m_builderMock);
    auto provider = React::MakeTurboModuleProvider<MyTurboModule, MyTurboModuleSpec>();
    m_moduleObject = m_builderMock.CreateModule(provider, m_moduleBuilder);
    m_module = React::ReactNonAbiValue<MyTurboModule>::GetPtrUnsafe(m_moduleObject);
  }

  TEST_METHOD(TestMethodCall_Add) {
    m_builderMock.Call1(L"Add", std::function<void(int)>([](int result) noexcept { TestCheckEqual(8, result); }), 3, 5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_Negate) {
    m_builderMock.Call1(L"Negate", std::function<void(int)>([](int result) noexcept { TestCheck(result == -3); }), 3);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_SayHello) {
    m_builderMock.Call1(L"SayHello", std::function<void(const std::string &)>([](const std::string &result) noexcept {
                          TestCheck(result == "Hello");
                        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticAdd) {
    m_builderMock.Call1(
        L"StaticAdd", std::function<void(int)>([](int result) noexcept { TestCheck(result == 25); }), 20, 5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegate) {
    m_builderMock.Call1(
        L"StaticNegate", std::function<void(int)>([](int result) noexcept { TestCheck(result == -7); }), 7);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticSayHello) {
    m_builderMock.Call1(
        L"StaticSayHello", std::function<void(const std::string &)>([](const std::string &result) noexcept {
          TestCheck(result == "Hello");
        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_SayHello0) {
    m_builderMock.Call0(L"SayHello0");
    TestCheck(m_module->Message == "Hello_0");
  }

  TEST_METHOD(TestMethodCall_PrintPoint) {
    m_builderMock.Call0(L"PrintPoint", Point{/*X =*/3, /*Y =*/5});
    TestCheck(m_module->Message == "Point: (3, 5)");
  }

  TEST_METHOD(TestMethodCall_PrintLine) {
    m_builderMock.Call0(L"PrintLine", Point{/*X =*/3, /*Y =*/5}, Point{/*X =*/6, /*Y =*/8});
    TestCheck(m_module->Message == "Line: (3, 5)-(6, 8)");
  }

  TEST_METHOD(TestMethodCall_StaticSayHello0) {
    m_builderMock.Call0(L"StaticSayHello0");
    TestCheck(MyTurboModule::StaticMessage == "Hello_0");
  }

  TEST_METHOD(TestMethodCall_StaticPrintPoint) {
    m_builderMock.Call0(L"StaticPrintPoint", Point{/*X =*/13, /*Y =*/15});
    TestCheck(MyTurboModule::StaticMessage == "Static Point: (13, 15)");
  }

  TEST_METHOD(TestMethodCall_StaticPrintLine) {
    m_builderMock.Call0(L"StaticPrintLine", Point{/*X =*/13, /*Y =*/15}, Point{/*X =*/16, /*Y =*/18});
    TestCheck(MyTurboModule::StaticMessage == "Static Line: (13, 15)-(16, 18)");
  }

  TEST_METHOD(TestMethodCall_AddCallback) {
    m_builderMock.Call1(
        L"AddCallback", std::function<void(int)>([](int result) noexcept { TestCheck(result == -1); }), 7, -8);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateCallback) {
    m_builderMock.Call1(
        L"NegateCallback", std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }), 4);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateAsyncCallback) {
    Mso::FutureWait(m_builderMock.Call1(
        L"NegateAsyncCallback", std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }), 4));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateDispatchQueueCallback) {
    Mso::FutureWait(m_builderMock.Call1(
        L"NegateDispatchQueueCallback",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }),
        4));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateFutureCallback) {
    Mso::FutureWait(m_builderMock.Call1(
        L"NegateFutureCallback", std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }), 4));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_SayHelloCallback) {
    m_builderMock.Call1(
        L"SayHelloCallback", std::function<void(const std::string &)>([](const std::string &result) noexcept {
          TestCheck(result == "Hello_2");
        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticAddCallback) {
    m_builderMock.Call1(
        L"StaticAddCallback", std::function<void(int)>([](int result) noexcept { TestCheck(result == 60); }), 4, 56);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateCallback) {
    m_builderMock.Call1(
        L"StaticNegateCallback", std::function<void(int)>([](int result) noexcept { TestCheck(result == -33); }), 33);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateAsyncCallback) {
    Mso::FutureWait(m_builderMock.Call1(
        L"StaticNegateAsyncCallback",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }),
        4));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateDispatchQueueCallback) {
    Mso::FutureWait(m_builderMock.Call1(
        L"StaticNegateDispatchQueueCallback",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }),
        4));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateFutureCallback) {
    Mso::FutureWait(m_builderMock.Call1(
        L"StaticNegateFutureCallback",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -4); }),
        4));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticSayHelloCallback) {
    m_builderMock.Call1(
        L"StaticSayHelloCallback", std::function<void(const std::string &)>([](const std::string &result) noexcept {
          TestCheck(result == "Static Hello_2");
        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_CallbackZeroArgs) {
    m_builderMock.Call1(L"CallbackZeroArgs", std::function<void()>([]() noexcept {}));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_CallbackTwoArgs) {
    m_builderMock.Call1(L"CallbackTwoArgs", std::function<void(int, int)>([](int p1, int p2) noexcept {
                          TestCheckEqual(1, p1);
                          TestCheckEqual(2, p2);
                        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_CallbackThreeArgs) {
    m_builderMock.Call1(
        L"CallbackThreeArgs",
        std::function<void(int, int, std::string const &)>([](int p1, int p2, std::string const &p3) noexcept {
          TestCheckEqual(1, p1);
          TestCheckEqual(2, p2);
          TestCheckEqual("Hello", p3);
        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_DivideCallbacks) {
    m_builderMock.Call2(
        L"DivideCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Division by 0"); }),
        6,
        2);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_DivideCallbacksError) {
    m_builderMock.Call2(
        L"DivideCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Division by 0"); }),
        6,
        0);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateCallbacks) {
    m_builderMock.Call2(
        L"NegateCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateCallbacksError) {
    m_builderMock.Call2(
        L"NegateCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateAsyncCallbacks) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateAsyncCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateAsyncCallbacksError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateAsyncCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateDispatchQueueCallbacks) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateDispatchQueueCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateDispatchQueueCallbacksError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateDispatchQueueCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateFutureCallbacks) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateFutureCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateFutureCallbacksError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateFutureCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_ResolveSayHelloCallbacks) {
    m_builderMock.Call2(
        L"ResolveSayHelloCallbacks",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_3"); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Goodbye"); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_RejectSayHelloCallbacks) {
    m_builderMock.Call2(
        L"RejectSayHelloCallbacks",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_3"); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Goodbye"); }));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticDivideCallbacks) {
    m_builderMock.Call2(
        L"StaticDivideCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Division by 0"); }),
        6,
        2);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticDivideCallbacksError) {
    m_builderMock.Call2(
        L"StaticDivideCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Division by 0"); }),
        6,
        0);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateCallbacks) {
    m_builderMock.Call2(
        L"StaticNegateCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateCallbacksError) {
    m_builderMock.Call2(
        L"StaticNegateCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateAsyncCallbacks) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateAsyncCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateAsyncCallbacksError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateAsyncCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateDispatchQueueCallbacks) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateDispatchQueueCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateDispatchQueueCallbacksError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateDispatchQueueCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateFutureCallbacks) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateFutureCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateFutureCallbacksError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateFutureCallbacks",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticResolveSayHelloCallbacks) {
    m_builderMock.Call2(
        L"StaticResolveSayHelloCallbacks",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_3"); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Goodbye"); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticRejectSayHelloCallbacks) {
    m_builderMock.Call2(
        L"StaticRejectSayHelloCallbacks",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_3"); }),
        std::function<void(std::string const &)>(
            [](std::string const &error) noexcept { TestCheck(error == "Goodbye"); }));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_TwoCallbacksZeroArgs1) {
    m_builderMock.Call2(
        L"TwoCallbacksZeroArgs1", std::function<void()>([]() noexcept {}), std::function<void()>([]() noexcept {
          TestCheckFail();
        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_TwoCallbacksZeroArgs2) {
    m_builderMock.Call2(
        L"TwoCallbacksZeroArgs2",
        std::function<void()>([]() noexcept { TestCheckFail(); }),
        std::function<void()>([]() noexcept {}));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_TwoCallbacksTwoArgs1) {
    m_builderMock.Call2(
        L"TwoCallbacksTwoArgs1",
        std::function<void(int, int)>([](int p1, int p2) noexcept {
          TestCheckEqual(1, p1);
          TestCheckEqual(2, p2);
        }),
        std::function<void(int, int)>([](int /*p1*/, int /*p2*/) noexcept { TestCheckFail(); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_TwoCallbacksTwoArgs2) {
    m_builderMock.Call2(
        L"TwoCallbacksTwoArgs2",
        std::function<void(int, int)>([](int /*p1*/, int /*p2*/) noexcept { TestCheckFail(); }),
        std::function<void(int, int)>([](int p1, int p2) noexcept {
          TestCheckEqual(1, p1);
          TestCheckEqual(2, p2);
        }));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_TwoCallbacksThreeArgs1) {
    m_builderMock.Call2(
        L"TwoCallbacksThreeArgs1",
        std::function<void(int, int, const std::string &)>([](int p1, int p2, std::string const &p3) noexcept {
          TestCheckEqual(1, p1);
          TestCheckEqual(2, p2);
          TestCheckEqual("Hello", p3);
        }),
        std::function<void(int, int, const std::string &)>(
            [](int /*p1*/, int /*p2*/, std::string const & /*p3*/) noexcept { TestCheckFail(); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_TwoCallbacksThreeArgs2) {
    m_builderMock.Call2(
        L"TwoCallbacksThreeArgs2",
        std::function<void(int, int, const std::string &)>(
            [](int /*p1*/, int /*p2*/, std::string const & /*p3*/) noexcept { TestCheckFail(); }),
        std::function<void(int, int, const std::string &)>([](int p1, int p2, std::string const &p3) noexcept {
          TestCheckEqual(1, p1);
          TestCheckEqual(2, p2);
          TestCheckEqual("Hello", p3);
        }));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_DividePromise) {
    m_builderMock.Call2(
        L"DividePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Division by 0"); }),
        6,
        2);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_DividePromiseError) {
    m_builderMock.Call2(
        L"DividePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Division by 0"); }),
        6,
        0);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegatePromise) {
    m_builderMock.Call2(
        L"NegatePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegatePromiseError) {
    m_builderMock.Call2(
        L"NegatePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateAsyncPromise) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateAsyncPromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateAsyncPromiseError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateAsyncPromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateDispatchQueuePromise) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateDispatchQueuePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateDispatchQueuePromiseError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateDispatchQueuePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateFuturePromise) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateFuturePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_NegateFuturePromiseError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"NegateFuturePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_VoidPromise) {
    m_builderMock.Call2(
        L"voidPromise",
        std::function<void()>([]() noexcept {}),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Odd unexpected"); }),
        2);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_VoidError) {
    m_builderMock.Call2(
        L"voidPromise",
        std::function<void()>([]() noexcept {}),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Odd unexpected"); }),
        3);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_ResolveSayHelloPromise) {
    m_builderMock.Call2(
        L"ResolveSayHelloPromise",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_4"); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Promise rejected"); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_RejectSayHelloPromise) {
    m_builderMock.Call2(
        L"RejectSayHelloPromise",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_4"); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Promise rejected"); }));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticDividePromise) {
    m_builderMock.Call2(
        L"StaticDividePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Division by 0"); }),
        6,
        2);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticDividePromiseError) {
    m_builderMock.Call2(
        L"StaticDividePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == 3); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Division by 0"); }),
        6,
        0);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegatePromise) {
    m_builderMock.Call2(
        L"StaticNegatePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegatePromiseError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateAsyncPromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateAsyncPromise) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateAsyncPromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateAsyncPromiseError) {
    m_builderMock.Call2(
        L"StaticNegatePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateDispatchQueuePromise) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateDispatchQueuePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateDispatchQueuePromiseError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateDispatchQueuePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateFuturePromise) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateFuturePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        5));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegateFuturePromiseError) {
    Mso::FutureWait(m_builderMock.Call2(
        L"StaticNegateFuturePromise",
        std::function<void(int)>([](int result) noexcept { TestCheck(result == -5); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Already negative"); }),
        -5));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticVoidPromise) {
    m_builderMock.Call2(
        L"staticVoidPromise",
        std::function<void()>([]() noexcept {}),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Odd unexpected"); }),
        2);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticVoidPromiseError) {
    m_builderMock.Call2(
        L"staticVoidPromise",
        std::function<void()>([]() noexcept {}),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Odd unexpected"); }),
        3);
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticResolveSayHelloPromise) {
    m_builderMock.Call2(
        L"StaticResolveSayHelloPromise",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_4"); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Promise rejected"); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticRejectSayHelloPromise) {
    m_builderMock.Call2(
        L"StaticRejectSayHelloPromise",
        std::function<void(const std::string &)>(
            [](const std::string &result) noexcept { TestCheck(result == "Hello_4"); }),
        std::function<void(React::JSValue const &)>(
            [](React::JSValue const &error) noexcept { TestCheck(error["message"] == "Promise rejected"); }));
    TestCheck(m_builderMock.IsRejectCallbackCalled());
  }

  TEST_METHOD(TestMethodSyncCall_AddSync) {
    int result;
    m_builderMock.CallSync(L"AddSync", /*out*/ result, 3, 5);
    TestCheck(result == 8);
  }

  TEST_METHOD(TestMethodSyncCall_NegateSync) {
    int result;
    m_builderMock.CallSync(L"NegateSync", /*out*/ result, 7);
    TestCheck(result == -7);
  }

  TEST_METHOD(TestMethodSyncCall_SayHelloSync) {
    std::string result;
    m_builderMock.CallSync(L"SayHelloSync", /*out*/ result);
    TestCheck(result == "Hello");
  }

  TEST_METHOD(TestMethodSyncCall_StaticAddSync) {
    int result;
    m_builderMock.CallSync(L"StaticAddSync", /*out*/ result, 3, 5);
    TestCheck(result == 8);
  }

  TEST_METHOD(TestMethodSyncCall_StaticNegateSync) {
    int result;
    m_builderMock.CallSync(L"StaticNegateSync", /*out*/ result, 7);
    TestCheck(result == -7);
  }

  TEST_METHOD(TestMethodSyncCall_StaticSayHelloSync) {
    std::string result;
    m_builderMock.CallSync(L"StaticSayHelloSync", /*out*/ result);
    TestCheck(result == "Hello");
  }

  TEST_METHOD(TestConstants) {
    auto constants = m_builderMock.GetConstants();
    TestCheck(constants["Constant1"] == "MyConstant1");
    TestCheck(constants["const2"] == "MyConstant2");
    TestCheck(constants["const3"]["X"] == 2);
    TestCheck(constants["const3"]["Y"] == 3);
    TestCheck(constants["Constant4"]["X"] == 3);
    TestCheck(constants["Constant4"]["Y"] == 4);
    TestCheck(constants["const51"]["X"] == 12);
    TestCheck(constants["const51"]["Y"] == 14);
    TestCheck(constants["const52"] == "MyConstant52");
    TestCheck(constants["const61"]["X"] == 15);
    TestCheck(constants["const61"]["Y"] == 17);
    TestCheck(constants["const62"] == "MyConstant62");
  }

  TEST_METHOD(TestEvent_IntEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"OnIntEvent", [&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheck(args[0] == 42);
          eventRaised = true;
        });

    m_module->OnIntEvent(42);
    TestCheck(eventRaised);
  }

  TEST_METHOD(TestEvent_OnNoArgEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"OnNoArgEvent", [&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheckEqual(0u, args.size());
          eventRaised = true;
        });

    m_module->OnNoArgEvent();
    TestCheck(eventRaised);
  }

  TEST_METHOD(TestEvent_TwoArgsEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"OnTwoArgsEvent", [&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheckEqual(4, args[0]["X"]);
          TestCheckEqual(2, args[0]["Y"]);
          TestCheckEqual(12, args[1]["X"]);
          TestCheckEqual(18, args[1]["Y"]);
          eventRaised = true;
        });

    m_module->OnTwoArgsEvent(Point{/*X =*/4, /*Y =*/2}, Point{/*X =*/12, /*Y =*/18});
    TestCheck(eventRaised);
  }

  TEST_METHOD(TestEvent_JSNameEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"onPointEvent", [&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheck(args[0]["X"] == 4);
          TestCheck(args[0]["Y"] == 2);
          eventRaised = true;
        });

    m_module->OnPointEvent(Point{/*X =*/4, /*Y =*/2});
    TestCheck(eventRaised == true);
  }

  TEST_METHOD(TestEvent_JSEventEmitterEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"MyEventEmitter", L"onStringEvent", [&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheckEqual("Hello World!", args[0]);
          eventRaised = true;
        });

    m_module->OnStringEvent("Hello World!");
    TestCheck(eventRaised == true);
  }

  TEST_METHOD(TestEvent_JSValueObjectEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"OnJSValueEvent", ([&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheck(args[0]["X"] == 4);
          TestCheck(args[0]["Y"] == 2);
          eventRaised = true;
        }));

    m_module->OnJSValueEvent(React::JSValueObject{{"X", 4}, {"Y", 2}});
    TestCheck(eventRaised == true);
  }

  TEST_METHOD(TestEvent_JSValueArrayEventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"OnJSValueEvent", ([&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheck(args[0][0] == "X");
          TestCheck(args[0][1] == 4);
          TestCheck(args[0][2] == true);
          TestCheck(args[0][3]["Id"] == 42);
          eventRaised = true;
        }));

    m_module->OnJSValueEvent(React::JSValueArray{"X", 4, true, React::JSValueObject{{"Id", 42}}});
    TestCheck(eventRaised == true);
  }

  TEST_METHOD(TestEvent_JSValueArray1EventField) {
    bool eventRaised = false;
    m_builderMock.ExpectEvent(
        L"RCTDeviceEventEmitter", L"OnJSValueEvent", ([&eventRaised](React::JSValueArray const &args) noexcept {
          TestCheck(args[0][0] == 4);
          eventRaised = true;
        }));

    m_module->OnJSValueEvent(React::JSValueArray{4});
    TestCheck(eventRaised == true);
  }

  TEST_METHOD(TestFunction_JSIntFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyTurboModule", L"JSIntFunction", [&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheck(args[0] == 42);
          functionCalled = true;
        });

    m_module->JSIntFunction(42);
    TestCheck(functionCalled);
  }

  TEST_METHOD(TestFunction_JSNameFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyTurboModule", L"pointFunc", [&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheck(args[0]["X"] == 4);
          TestCheck(args[0]["Y"] == 2);
          functionCalled = true;
        });

    m_module->JSPointFunction(Point{/*X =*/4, /*Y =*/2});
    TestCheck(functionCalled == true);
  }

  TEST_METHOD(TestFunction_TwoArgFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyTurboModule", L"lineFunc", [&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheck(args[0]["X"] == 4);
          TestCheck(args[0]["Y"] == 2);
          TestCheck(args[1]["X"] == 12);
          TestCheck(args[1]["Y"] == 18);
          functionCalled = true;
        });

    m_module->JSLineFunction(Point{/*X =*/4, /*Y =*/2}, Point{/*X =*/12, /*Y =*/18});
    TestCheck(functionCalled == true);
  }

  TEST_METHOD(TestFunction_NoArgFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyTurboModule", L"JSNoArgFunction", [&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheckEqual(0u, args.size());
          functionCalled = true;
        });

    m_module->JSNoArgFunction();
    TestCheck(functionCalled);
  }

  TEST_METHOD(TestFunction_JSModuleNameFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyModule", L"stringFunc", [&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheck(args[0] == "Hello World!");
          functionCalled = true;
        });

    m_module->JSStringFunction("Hello World!");
    TestCheck(functionCalled == true);
  }

  TEST_METHOD(TestFunction_JSValueObjectFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyTurboModule", L"JSValueFunction", ([&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheck(args[0]["X"] == 4);
          TestCheck(args[0]["Y"] == 2);
          functionCalled = true;
        }));

    m_module->JSValueFunction(React::JSValueObject{{"X", 4}, {"Y", 2}});
    TestCheck(functionCalled == true);
  }

  TEST_METHOD(TestFunction_JSValueArrayFunctionField) {
    bool functionCalled = false;
    m_builderMock.ExpectFunction(
        L"MyTurboModule", L"JSValueFunction", ([&functionCalled](React::JSValueArray const &args) noexcept {
          TestCheck(args[0][0] == "X");
          TestCheck(args[0][1] == 4);
          TestCheck(args[0][2] == true);
          TestCheck(args[0][3]["Id"] == 42);
          functionCalled = true;
        }));

    m_module->JSValueFunction(React::JSValueArray{"X", 4, true, React::JSValueObject{{"Id", 42}}});
    TestCheck(functionCalled == true);
  }

  TEST_METHOD(TestInitialized) {
    TestCheck(m_module->IsInitialized);
  }
};

} // namespace ReactNativeTests
