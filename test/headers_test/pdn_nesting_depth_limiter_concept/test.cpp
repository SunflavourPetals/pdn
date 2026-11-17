// Tests for pdn/include/pdn/pdn_nesting_depth_limiter_concept.h.
//
// The concept guards the optional function-package member the parser calls to cap {} and []
// nesting. It is intentionally narrow: callable on a const object, returning exactly std::size_t.
// A package that returns another integer type is treated as "not provided" (the parser keeps its
// default 256), which is why the negative cases below matter as much as the positive ones.

#include <pdn/pdn_nesting_depth_limiter_concept.h>

#include <cstddef>

namespace
{
	// Accepts a const object with the exact return type. Neither static nor noexcept nor constexpr
	// are required by the concept.
	struct plain
	{
		constexpr auto nesting_depth_limit() const -> ::std::size_t { return 42; }
	};

	struct with_noexcept
	{
		constexpr auto nesting_depth_limit() const noexcept -> ::std::size_t { return 1024; }
	};

	struct static_member
	{
		static auto nesting_depth_limit() -> ::std::size_t { return 7; }
	};

	// Return type is not std::size_t: the member is ignored on purpose.
	struct returns_int
	{
		auto nesting_depth_limit() const -> int { return 42; }
	};

	struct returns_long_long
	{
		auto nesting_depth_limit() const -> long long { return 42; }
	};

	struct returns_reference
	{
		auto nesting_depth_limit() const -> ::std::size_t& { static ::std::size_t v = 42; return v; }
	};

	// Callable only on a non-const object.
	struct non_const_only
	{
		auto nesting_depth_limit() -> ::std::size_t { return 42; }
	};

	// Wrong shape: takes an argument, or is a data member rather than a function.
	struct takes_argument
	{
		auto nesting_depth_limit(int) const -> ::std::size_t { return 42; }
	};

	struct data_member
	{
		::std::size_t nesting_depth_limit = 42;
	};

	struct nothing
	{
	};

	static_assert(pdn::concepts::nesting_depth_limiter<plain>);
	static_assert(pdn::concepts::nesting_depth_limiter<with_noexcept>);
	static_assert(pdn::concepts::nesting_depth_limiter<static_member>);

	static_assert(!pdn::concepts::nesting_depth_limiter<returns_int>);
	static_assert(!pdn::concepts::nesting_depth_limiter<returns_long_long>);
	static_assert(!pdn::concepts::nesting_depth_limiter<returns_reference>);
	static_assert(!pdn::concepts::nesting_depth_limiter<non_const_only>);
	static_assert(!pdn::concepts::nesting_depth_limiter<takes_argument>);
	static_assert(!pdn::concepts::nesting_depth_limiter<data_member>);
	static_assert(!pdn::concepts::nesting_depth_limiter<nothing>);

	// References are stripped before the const is applied, so a reference argument means the same
	// as the value type: T&, const T& and T&& cannot smuggle in a non-const nesting_depth_limit().
	static_assert(pdn::concepts::nesting_depth_limiter<plain&>);
	static_assert(pdn::concepts::nesting_depth_limiter<const plain&>);
	static_assert(pdn::concepts::nesting_depth_limiter<plain&&>);
	static_assert(!pdn::concepts::nesting_depth_limiter<non_const_only&>);
	static_assert(!pdn::concepts::nesting_depth_limiter<non_const_only&&>);

	// The parser's overload pair, reduced to its essence: use the package's limit when the concept
	// holds, otherwise the built-in default. This is the behaviour the concept exists to select.
	constexpr ::std::size_t default_limit = 256;

	template <typename package_t>
	constexpr auto limit_of(const package_t& package) noexcept -> ::std::size_t
	{
		if constexpr (pdn::concepts::nesting_depth_limiter<package_t>)
		{
			return package.nesting_depth_limit();
		}
		else
		{
			return default_limit;
		}
	}

	static_assert(limit_of(plain{}) == 42);
	static_assert(limit_of(with_noexcept{}) == 1024);
	static_assert(limit_of(returns_int{}) == default_limit);
	static_assert(limit_of(nothing{}) == default_limit);
}

int main()
{
	return limit_of(plain{}) == 42 ? 0 : 1;
}
