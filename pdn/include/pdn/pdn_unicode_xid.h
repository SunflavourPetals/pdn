#ifndef PDN_Header_pdn_unicode_xid
#define PDN_Header_pdn_unicode_xid

// Unicode 18.0.0  XID_Start / XID_Continue, from DerivedCoreProperties.txt.

#include <cstddef>

#include "pdn_unicode_xid_data_start.h"
#include "pdn_unicode_xid_data_continue.h"

namespace pdn::unicode::detail
{
	// Binary search in a sorted list of disjoint closed ranges.
	// Invariant: all ranges before `first` end before cp, all ranges from `last`
	// on start after cp.
	template <::std::size_t N>
	[[nodiscard]] constexpr bool in_ranges(const char32_t(&lo)[N], const char32_t(&hi)[N], char32_t cp) noexcept
	{
		auto first = ::std::size_t{};
		auto last = N;
		while (first < last)
		{
			const auto mid = first + (last - first) / 2;
			if (cp < lo[mid])
			{
				last = mid;
			}
			else if (cp > hi[mid])
			{
				first = mid + 1;
			}
			else
			{
				return true;
			}
		}
		return false;
	}

	// Sanity check on the generated data: sorted, disjoint, non-inverted.
	template <::std::size_t N>
	constexpr bool is_sorted_disjoint(const char32_t(&lo)[N], const char32_t(&hi)[N]) noexcept
	{
		for (auto i = ::std::size_t{}; i < N; ++i)
		{
			if (lo[i] > hi[i])
			{
				return false;
			}
			if (i > 0 && lo[i] <= hi[i - 1])
			{
				return false;
			}
		}
		return true;
	}

}

namespace pdn::unicode
{
	/// True if `cp` has the XID_Start property (Unicode 18.0.0).
	[[nodiscard]] constexpr bool is_xid_start(char32_t cp) noexcept
	{
		return detail::in_ranges(detail::xid_start_lo, detail::xid_start_hi, cp);
	}

	/// True if `cp` has the XID_Continue property (Unicode 18.0.0).
	/// Note: XID_Start is a subset of XID_Continue, but this is a separate lookup.
	[[nodiscard]] constexpr bool is_xid_continue(char32_t cp) noexcept
	{
		return detail::in_ranges(detail::xid_continue_lo, detail::xid_continue_hi, cp);
	}
}

#endif
