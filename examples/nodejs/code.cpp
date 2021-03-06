#include <chrono>

// This is the `current/intergrations/nodejs/javascript.hpp`, with the path to it set in `binding.gyp`,
// so that the node-based build and test, which can be run with just `make` from this directory, does the job.
#include "javascript.hpp"

Napi::Object Init(Napi::Env env, Napi::Object unwrapped_exports) {
  using namespace current::javascript;

  JSEnvScope scope(env);
  JSObject exports(unwrapped_exports);

  // Some immediate values.
  exports["valueInt"] = 42;
  exports["valueDouble"] = 3.14;

  exports["valueString"] = "The Answer";

  exports["valueTrue"] = true;
  exports["valueFalse"] = false;

  exports["valueNull"] = nullptr;
  exports["valueUndefined"] = Undefined();

  exports["valueObjectOne"] = JSObject().Add("foo", "Foo").Add("bar", 42);
  JSObject valueObjectTwo;
  valueObjectTwo["foo"] = "Two";
  valueObjectTwo["bar"] = true;
  exports["valueObjectTwo"] = valueObjectTwo;
  JSObject valueObjectNested;
  valueObjectNested["a"]["aa"] = 0;
  valueObjectNested["a"]["ab"] = 1;
  valueObjectNested["b"]["ba"] = 2;
  valueObjectNested["b"]["bb"] = 3;
  exports["valueObjectNested"] = valueObjectNested;

  // The simple case: Just return the sum.
  exports["cppSyncSum"] = [](int a, int b) { return a + b; };

  // Another simple case: Invoke the callback with the sum as the only argument.
  exports["cppSyncCallbackSum"] = [](int a, int b, JSFunctionReference f) { f(a + b); };

  // The asynchronous callback must be called from within the right place, where it's legal to call into JavaScript.
  exports["cppAsyncCallbackSum"] = [](int a, int b, JSFunction f) {
    JSAsync([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }, [f, a, b]() { f(a + b); });
  };

  // The future can also be only set from the right place.
  exports["cppFutureSum"] = [](int a, int b) {
    JSPromise promise;
    JSAsync([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); },
            [promise, a, b]() { promise = a + b; });
    return promise;
  };

  // Check that `return nullptr;` returns `null` into JavaScript, and returning nothing returns `undefined`.
  exports["cppReturnsNull"] = []() { return nullptr; };
  exports["cppReturnsUndefined"] = []() {};
  exports["cppReturnsUndefinedII"] = []() { return Undefined(); };

  // NOTE(dkorolev): The arguments here can be `JSFunction` or `JSFunctionReference`,
  // as they are only called synchronously, from the "main thread".
  // In such a scenario, `JSFunctionReference` is preferred, as it has a lower overhead.
  exports["cppSyncCallbacksABA"] = [](JSFunctionReference f, JSFunctionReference g) {
    f(1);
    g(2);
    f(":three");
    // This weird way of returning from a synchronous function is unnecessary, but it is used here deliberately,
    // for the JavaScript tests for `cppSyncCallbacksABA` and `cppAsyncCallbacksABA` to be identical.
    JSPromise promise;
    promise = nullptr;
    return promise;
  };

  // Note: Unlike in `cppSyncCallbacksABA`, these functions should be of type `JSFunction`, not `JSFunctionReference`.
  exports["cppAsyncCallbacksABA"] = [](JSFunction f, JSFunction g) {
    JSPromise promise;
    JSAsync([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); },
            [f, g, promise]() {
              f("-test");
              g(":here:", nullptr, ":", 3.14, ":", true);
              f("-passed");
              promise = nullptr;
            });
    return promise;
  };

  // A simple function that returns an object.
  exports["cppReturnsObject"] = []() {
    JSObject object;
    object["_null"] = nullptr;
    object.Add("supports", "also").Add("dot_notation", true);
    object["_undefined"] = Undefined();
    return object;
  };

  // A simple function that modifies an object.
  exports["cppModifiesObject"] = [](JSObject obj) {
    const int a = obj["a"];
    const JSAny b = obj["b"];
    obj["sum"] = a + b.As<int>();
    obj["sum_as_string"] = std::to_string(a + static_cast<int>(b));
    return obj;
  };

  exports["cppAcceptsAny"] = [](std::string type, JSAny any) -> std::string {
    if (type == "i") {
      return "TwiceInt: " + std::to_string(any.As<int>() * 2);
    } else if (type == "s") {
      const std::string s = any;
      return "TwiceString: " + s + s;
    } else {
      return "Error.";
    }
  };

  exports["cppAcceptsAnyII"] = [](JSAny any) -> std::string {
    if (any.IsNumber()) {
      return "TwiceIntII: " + std::to_string(any.As<int>() * 2);
    } else if (any.IsString()) {
      const std::string s = any;
      return "TwiceStringII: " + s + s;
    } else {
      return "ErrorII.";
    }
  };

  exports["cppReturnsAny"] = [](std::string t) -> JSAny {
    if (t == "i") {
      return 42;
    } else if (t == "s") {
      return "42";
    } else if (t == "o") {
      return JSObject();
    } else {
      return nullptr;
    }
  };

  // A "native" lambda can be "returned", and the magic behind the scenes will work its way.
  exports["cppWrapsFunction"] = [](int x, JSFunctionReturning<std::string> f) {
    // This test case exposes a function, but only to be called within the call scope of the outer function.
    return f([x](int y) { return "Outer " + std::to_string(x) + ", inner " + std::to_string(y) + '.'; });
  };
  exports["cppWrapsFunctionWithState"] = [](int base) {
    // And this test case returns a function that keeps its own state across calls.
    return [shared_number = std::make_shared<int>(base)]() { return (*shared_number)++; };
  };

  // A C++ code that calls back into JavaScript and analyzes the results of those calls.
  exports["cppGetsResultsOfJsFunctions"] = [](JSFunctionReferenceReturning<std::string> a,
                                              JSFunctionReferenceReturning<std::string> b) { return a() + b(); };

  // A C++ code that calls back into JavaScript and analyzes the results of those calls.
  exports["cppGetsResultsOfJsFunctionsAsync"] = [](JSFunctionReturning<std::string> a,
                                                   JSFunctionReturning<std::string> b) {
    JSPromise promise;
    JSAsync([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); },
            [a, b, promise]() { promise = a() + b(); });
    return promise;
  };

  // Calling back JavaScript code multiple times, at arbitrary times.
  exports["cppMultipleAsyncCallsAtArbitraryTimes"] = [](JSFunctionReturning<bool> f_cond,
                                                        JSFunctionReturning<void> f_print) {
    const auto is_odd_prime = [](int p) {
      for (int d = 3; d * d <= p; d += 2) {
        if ((p % d) == 0) {
          return false;
        }
      }
      return true;
    };

    JSPromise promise;

    JSAsyncEventLoop([f_cond, f_print, promise, is_odd_prime](JSAsyncEventLoopRunner run_in_event_loop) {
      // Demo that `run_in_event_loop` can take a function returning a `bool`.
      if (!run_in_event_loop([f_cond, f_print, promise]() {
            if (!f_cond(1) || !f_cond(2)) {
              promise = 0;
              return false;
            } else {
              return true;
            }
          })) {
        return;
      }

      run_in_event_loop([f_print]() { f_print(2); });

      int odd_p = 3;
      int total_primes_found = 1;

      while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(odd_p));  // Sleep progressively longer.
        const bool is_odd_p_prime = is_odd_prime(odd_p);
        if (!run_in_event_loop([f_cond, f_print, is_odd_p_prime, &odd_p, &total_primes_found, promise]() {
              if (!f_cond(odd_p)) {
                promise = total_primes_found;
                return false;
              }
              if (is_odd_p_prime) {
                ++total_primes_found;
                f_print(odd_p);
              }
              if (!f_cond(++odd_p)) {
                promise = total_primes_found;
                return false;
              }
              ++odd_p;
              return true;
            })) {
          return;
        }
      }
    });

    return promise;
  };

  return exports;
}

NODE_API_MODULE(example, Init)
