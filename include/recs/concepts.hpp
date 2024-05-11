#pragma once

#include <concepts>
#include <type_traits>

namespace recs
{

template <typename T, typename U>
concept SameAs = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename Access, typename T>
concept Contains = (Access::template contains<T>());

} // namespace recs
