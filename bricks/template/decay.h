/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>
          (c) 2019 Maxim Zhurovich <zhurovich@gmail.com>

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

#ifndef BRICKS_TEMPLATE_DECAY_H
#define BRICKS_TEMPLATE_DECAY_H

#include "../../port.h"

#include <string>
#include <type_traits>

#include "mapreduce.h"

namespace current {

template <typename T>
struct decay_impl {
  using type = std::decay_t<T>;
};

template <typename T>
using decay_impl_t = typename decay_impl<T>::type;

template <typename... TS>
struct decay_impl<std::tuple<TS...>> {
  using type = typename metaprogramming::map<decay_impl_t, std::tuple<TS...>>;
};

template <typename... TS>
struct decay_impl<const std::tuple<TS...>> {
  using type = typename metaprogramming::map<decay_impl_t, std::tuple<TS...>>;
};

template <typename... TS>
struct decay_impl<std::tuple<TS...>&> {
  using type = typename metaprogramming::map<decay_impl_t, std::tuple<TS...>>;
};

template <typename... TS>
struct decay_impl<const std::tuple<TS...>&> {
  using type = typename metaprogramming::map<decay_impl_t, std::tuple<TS...>>;
};

template <typename... TS>
struct decay_impl<std::tuple<TS...>&&> {
  using type = typename metaprogramming::map<decay_impl_t, std::tuple<TS...>>;
};

template <typename T>
using decay_t = typename decay_impl<T>::type;

}  // namespace current

#endif  // BRICKS_TEMPLATE_DECAY_H
