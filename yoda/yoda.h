/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>
          (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// TODO(dkorolev) + TODO(mzhurovich): These comments should be made up to date one day.

// Yoda is a simple to use, machine-learning-friendly key-value storage atop a Sherlock stream.
//
// The C++ usecase is to instantiate `Yoda<MyEntryType> API("stream_name")`.
// The API supports CRUD-like operations, presently `Get` and `Add`.
//
// The API offers three ways to invoke itself:
//
//   1) Asynchronous, using std::{promise/future}, that utilize the the async/await paradigm.
//      For instance, `AsyncAdd(entry)` and `AsyncGet(key)`, which both return a future.
//
//   2) Asynchronous, using std::function as callbacks. Best for HTTP endpoints and other thread-based usecases.
//      For instance, `AsyncAdd(entry, on_success, on_failure)` and `AsyncGet(key, on_success, on_failure)`,
//      which both return `void` immediately and invoke the callback as the result is ready, from a separate
//      thread.
//
//   3) Synchronous, blocking calls.
//      For instance, `Get(key)` and `Add(entry)`.
//
// Exceptions strategy:
//
//   `Async*()` versions of the callers that take callbacks and return immediately never throw.
//
//   For synchronous and promise-based implementations:
//
//   1) `Get()` and `AsyncGet()` throw the `KeyNotFoundException` if the key has not been found.
//
//      This behavior can be overriden by inheriting the entry type from `yoda::Nullable`, and adding:
//
//        constexpr static bool allow_nonthrowing_get = true;
//
//      to the definition of the entry class.
//
//   2) `Add()` and `AsyncGet()` throw `KeyAlreadyExistsException` if an entry with this key already exists.
//
//      This behavior can be overriden by inheriting the entry type from `yoda::AllowOverwriteOnAdd` or adding:
//
//        constexpr static bool allow_overwrite_on_add = true;
//
//      to the definition of the entry class.
//
// Policies:
//
//   Yoda's behavior can be customized further by providing a POLICY as a second template parameter to
//   yoda::API.
//   This struct should contain all the members described in the "Exceptions" section above.
//   Please refer to `struct DefaultPolicy` below for more details.

#ifndef SHERLOCK_YODA_YODA_H
#define SHERLOCK_YODA_YODA_H

#include <atomic>
#include <future>
#include <string>
#include <tuple>

#include "types.h"
#include "metaprogramming.h"
#include "policy.h"
#include "exceptions.h"

#include "api/key_entry/key_entry.h"
#include "api/matrix/matrix_entry.h"

namespace yoda {

// `yoda::APIWrapper` requires two template parameters:
// 1) The base type for the stream entries -- to serialize and deserialize polymorphic records.
// 2) The list of specific entries to expose through the Yoda API.
template <typename ENTRY_BASE_TYPE, typename ENTRIES_TYPELIST>
struct APIWrapper : apicalls::APICallsWrapper<
                        YodaTypes<ENTRY_BASE_TYPE, ENTRIES_TYPELIST>,
                        CombinedYodaImpls<YodaTypes<ENTRY_BASE_TYPE, ENTRIES_TYPELIST>, ENTRIES_TYPELIST>> {
 private:
  static_assert(bricks::metaprogramming::is_std_tuple<ENTRIES_TYPELIST>::value, "");
  typedef YodaTypes<ENTRY_BASE_TYPE, ENTRIES_TYPELIST> YT;

 public:
  APIWrapper() = delete;
  APIWrapper(const std::string& stream_name)
      : apicalls::APICallsWrapper<
            YT,
            CombinedYodaImpls<YodaTypes<ENTRY_BASE_TYPE, ENTRIES_TYPELIST>, ENTRIES_TYPELIST>>(mq_),
        stream_(sherlock::Stream<std::unique_ptr<typename YT::T_ENTRY_BASE_TYPE>>(stream_name)),
        mq_listener_(container_, stream_),
        mq_(mq_listener_),
        stream_listener_(mq_),
        sherlock_listener_scope_(stream_.Subscribe(stream_listener_)) {}

  typename YT::T_STREAM_TYPE& UnsafeStream() { return stream_; }

  template <typename F>
  typename YT::template T_STREAM_LISTENER_TYPE<F> Subscribe(F& listener) {
    return std::move(stream_.Subscribe(listener));
  }

  // For testing purposes.
  bool CaughtUp() const { return stream_listener_.caught_up_; }
  size_t EntriesSeen() const { return stream_listener_.entries_seen_; }

 private:
  typename YT::T_STREAM_TYPE stream_;
  YodaContainer<YT> container_;
  typename YT::T_MQ_LISTENER mq_listener_;
  typename YT::T_MQ mq_;
  typename YT::T_SHERLOCK_LISTENER stream_listener_;
  typename YT::T_SHERLOCK_LISTENER_SCOPE_TYPE sherlock_listener_scope_;
};

// `yoda::API` suports both a typelist and an `std::tuple<>` with parameter definition.
template <typename ENTRY_BASE_TYPE, typename... SUPPORTED_TYPES>
struct APIWrapperSelector {
  typedef APIWrapper<ENTRY_BASE_TYPE, std::tuple<SUPPORTED_TYPES...>> type;
};

template <typename ENTRY_BASE_TYPE, typename SUPPORTED_TYPES>
struct APIWrapperSelector<ENTRY_BASE_TYPE, std::tuple<SUPPORTED_TYPES>> {
  typedef APIWrapper<ENTRY_BASE_TYPE, std::tuple<SUPPORTED_TYPES>> type;
};

template <typename ENTRY_BASE_TYPE, typename... SUPPORTED_TYPES>
using API = typename APIWrapperSelector<ENTRY_BASE_TYPE, SUPPORTED_TYPES...>::type;

}  // namespace yoda

#endif  // SHERLOCK_YODA_YODA_H
