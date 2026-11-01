#ifndef PDN_Header_pdn_stoppable_concept
#define PDN_Header_pdn_stoppable_concept

#include <utility>
#include <concepts>
#include <type_traits>

#include "pdn_stop_reason.h"

namespace pdn::concepts
{
	template <typename type>
	concept stoppable = requires(type stop)
	{
		{ stop.stop_requested() } -> ::std::convertible_to<bool>;
	};

	template <typename type>
	concept stop_reasonable = stoppable<type> && requires(type stop)
	{
		{ stop.stop_reason() } -> ::std::convertible_to<stop_reason_code>;
	};

	template <typename type>
	concept const_stoppable = requires(const type stop)
	{
		{ stop.stop_requested() } -> ::std::convertible_to<bool>;
	};

	template <typename type>
	concept const_stop_reasonable = const_stoppable<type> && requires(const type stop)
	{
		{ stop.stop_reason() } -> ::std::convertible_to<stop_reason_code>;
	};

	template <typename type>
	concept nonconst_stoppable = stoppable<type> && !const_stoppable<type>;

	template <typename type>
	concept nonconst_stop_reasonable = stop_reasonable<type> && !const_stop_reasonable<type>;
}

namespace pdn::detail
{
	template <typename type>
	struct noexcept_stop_requested_const : ::std::false_type {};

	template <concepts::const_stoppable type>
	struct noexcept_stop_requested_const<type> : ::std::conditional_t<
		noexcept(::std::declval<const type&>().stop_requested()),
		::std::true_type,
		::std::false_type> {};

	template <typename type>
	constexpr bool noexcept_stop_requested_const_v = noexcept_stop_requested_const<type>::value;

	template <typename type>
	struct noexcept_stop_requested_nonconst : ::std::false_type {};

	template <concepts::stoppable type>
	struct noexcept_stop_requested_nonconst<type> : ::std::conditional_t<
		noexcept(::std::declval<type&>().stop_requested()),
		::std::true_type,
		::std::false_type> {};

	template <typename type>
	constexpr bool noexcept_stop_requested_nonconst_v = noexcept_stop_requested_nonconst<type>::value;

	template <typename type>
	struct noexcept_stop_reasonable_const : ::std::false_type {};

	template <concepts::const_stop_reasonable type>
	struct noexcept_stop_reasonable_const<type> : ::std::conditional_t<
		noexcept(::std::declval<const type&>().stop_reason()),
		::std::true_type,
		::std::false_type> {};

	template <typename type>
	constexpr bool noexcept_stop_reasonable_const_v = noexcept_stop_reasonable_const<type>::value;

	template <typename type>
	struct noexcept_stop_reasonable_nonconst : ::std::false_type {};

	template <concepts::stop_reasonable type>
	struct noexcept_stop_reasonable_nonconst<type> : ::std::conditional_t<
		noexcept(::std::declval<type&>().stop_reason()),
		::std::true_type,
		::std::false_type> {};

	template <typename type>
	constexpr bool noexcept_stop_reasonable_nonconst_v = noexcept_stop_reasonable_nonconst<type>::value;
}

#endif
