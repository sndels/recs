#pragma once

#include <concepts>
#include <type_traits>

namespace recs
{

template <typename T, typename U>
concept SameAs = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <typename Access, typename T>
concept Contains = (Access::template contains<T>());

// Things would probably get really messy really fast if non-POD component types
// were allowed.
template <typename T>
concept ValidComponent =
    (std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

} // namespace recs
