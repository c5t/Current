#pragma once

#include "javascript_cpp2js.hpp"

namespace current {
namespace javascript {
namespace impl {

template <class, bool>
struct JSFunctionCallerImpl;

// NOTE(dkorolev): The `operator()`-s are `const`, largely because this allows capturing JS functions into lambdas
// by values and calling them right away, without declaring the lambdas `mutable`.

template <class T>
class JSFunctionReferenceReturning final {
 private:
  Napi::FunctionReference function_;

 public:
  JSFunctionReferenceReturning(Napi::FunctionReference f) : function_(std::move(f)) {}
  JSFunctionReferenceReturning(Napi::Function f) : function_(Napi::Weak(f)) {}
  JSFunctionReferenceReturning(Napi::Value f) : function_(Napi::Weak(f.As<Napi::Function>())) {}

  template <typename... ARGS, class U = T>
  typename std::enable_if<std::is_same<U, void>::value, void>::type operator()(ARGS&&... args) const {
    function_.Call({CPP2JS(std::forward<ARGS>(args))...});
  }

  template <typename... ARGS, class U = T>
  typename std::enable_if<!std::is_same<U, void>::value, T>::type operator()(ARGS&&... args) const {
    return JSFunctionCallerImpl<T, true>::DoItForJSFunctionReferenceReturning(function_, std::forward<ARGS>(args)...);
  }
};

template <class T>
class JSFunctionReturning final {
 private:
  struct AsyncContextHolderImpl final {
    Napi::AsyncContext async_context_;
    Napi::FunctionReference function_reference_;
    explicit AsyncContextHolderImpl(const Napi::Function& f)
        : async_context_(JSEnv(), "CurrentJSBinding"), function_reference_(Napi::Persistent(f)) {}
  };
  std::shared_ptr<AsyncContextHolderImpl> impl_;

  const Napi::FunctionReference& GetFunctionReference() const { return impl_->function_reference_; }
  const Napi::AsyncContext& GetNapiAsyncContext() const { return impl_->async_context_; }

 public:
  explicit JSFunctionReturning(const Napi::Function& f) : impl_(std::make_shared<AsyncContextHolderImpl>(f)) {}

  template <typename... ARGS, class U = T>
  typename std::enable_if<std::is_same<U, void>::value, void>::type operator()(ARGS&&... args) const {
    GetFunctionReference().MakeCallback(JSEnv().Global(), {CPP2JS(std::forward<ARGS>(args))...}, GetNapiAsyncContext());
  }

  template <typename... ARGS, class U = T>
  typename std::enable_if<!std::is_same<U, void>::value, T>::type operator()(ARGS&&... args) const {
    return JSFunctionCallerImpl<T, true>::DoItForJSFunctionReturning(
        GetFunctionReference(), GetNapiAsyncContext(), std::forward<ARGS>(args)...);
  }
};

using JSFunctionReference = JSFunctionReferenceReturning<void>;
using JSFunction = JSFunctionReturning<void>;

}  // namespace impl
}  // namespace javascript
}  // namespace current
