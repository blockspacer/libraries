/*
    Copyright 2017 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#ifndef STLAB_MEMORY_HPP
#define STLAB_MEMORY_HPP

/**************************************************************************************************/

#include <memory>

/**************************************************************************************************/

namespace stlab {

/**************************************************************************************************/

inline namespace v1 {
/**************************************************************************************************/

template <typename T>
auto make_weak_ptr(const std::shared_ptr<T>& x) {
    return std::weak_ptr<T>(x);
}

/**************************************************************************************************/

} // namespace v1

/**************************************************************************************************/

} // namespace stlab

/**************************************************************************************************/

#endif