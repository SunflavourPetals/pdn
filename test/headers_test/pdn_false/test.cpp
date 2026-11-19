// Review target: pdn/include/pdn/pdn_false.h
// This TU includes only the header under review (plus the standard library), so it also
// checks that the header is self-contained.

#include <cstdio>
#include <type_traits>

#include <pdn/pdn_false.h>

namespace
{
	using pdn::detail::false_v;

	// ---- 1. Always false, regardless of the template argument ----
	static_assert(!false_v<int>);
	static_assert(!false_v<void>);
	static_assert(!false_v<int[]>);
	static_assert(!false_v<const volatile int&>);
	static_assert(!false_v<decltype(nullptr)>);
	static_assert(false_v<int> == false_v<void>); // the argument really does not matter

	// ---- 2. Usable in constant expressions and as a non-type template argument ----
	template <bool>
	struct flag
	{
		static constexpr bool value = true;
	};
	static_assert(flag<false_v<int>>::value);

	// ---- 3. Key property: inside a template definition it does not fail immediately ----
	// If false_v were not dependent, instantiating the primary template would already
	// fail. Only the explicit specialization for int is instantiated here, so a clean
	// build means the assertion was deferred to the invalid branch.
	template <typename t>
	struct only_int
	{
		static_assert(false_v<t>, "[test] only int is supported");
	};
	template <>
	struct only_int<int>
	{
		static constexpr int value = 42;
	};
	static_assert(only_int<int>::value == 42);

	// ---- 4. Discarded branch of if constexpr ----
	template <typename t>
	constexpr auto classify() -> int
	{
		if constexpr (::std::is_integral_v<t>)
		{
			return 1;
		}
		else
		{
			static_assert(false_v<t>, "[test] not integral");
			return 0;
		}
	}
	static_assert(classify<int>() == 1);

	// ---- 5. Rejecting some arguments in a partial specialization (same shape as pdn_proxy.h:88) ----
	template <typename t>
	struct no_array
	{
		static constexpr int value = 1;
	};
	template <typename t>
	struct no_array<t[]>
	{
		static_assert(false_v<t[]>, "[test] array is not allowed");
	};
	static_assert(no_array<int>::value == 1);
}

int main()
{
	::std::puts("pdn_false: all static assertions passed");
	return 0;
}
