// Review target: pdn/include/pdn/pdn_remove_cvref_same_as_concept.h
// This TU includes only the header under review (plus the standard library), so it also
// checks that the header is self-contained.
// History: this concept is the merge of decayed_same_as (one-sided normalization) and
// remove_cvref_same_as from parser_utility (two-sided normalization).

#include <pdn/pdn_remove_cvref_same_as_concept.h>

#include <cstdio>
#include <optional>

namespace
{
	using pdn::detail::remove_cvref_same_as;

	// ---- 1. Both sides strip cv/ref ----
	static_assert(remove_cvref_same_as<int, int>);
	static_assert(remove_cvref_same_as<const int, int>);
	static_assert(remove_cvref_same_as<int&, int>);
	static_assert(remove_cvref_same_as<const int&, volatile int&&>);
	static_assert(remove_cvref_same_as<int, const int>);
	static_assert(remove_cvref_same_as<int&&, const int&>);

	// ---- 2. Different types are not satisfied ----
	static_assert(!remove_cvref_same_as<int, long>);
	static_assert(!remove_cvref_same_as<int*, const int*>); // only top-level cv/ref is stripped, not the pointee's const

	// ---- 3. The name matches the implementation: remove_cvref is not decay ----
	static_assert(remove_cvref_same_as<int(&)[3], int(&)[3]>);
	static_assert(!remove_cvref_same_as<int(&)[3], int*>);
	static_assert(remove_cvref_same_as<void (&)(), void()>);
	static_assert(!remove_cvref_same_as<void (&)(), void (*)()>);

	// ---- 4. Usable as a return-type-requirement ----
	struct value_generator
	{
		auto generate_constant(int id) const -> ::std::optional<int> { return id; }
	};

	struct const_ref_generator
	{
		auto generate_constant(int) const -> const ::std::optional<int> & { return value; }
		::std::optional<int> value;
	};

	struct long_generator
	{
		auto generate_constant(int id) const -> ::std::optional<long> { return id; }
	};

	template <typename gen_t>
	concept can_generate_optional_int = requires (const gen_t & gen, int id)
	{
		{ gen.generate_constant(id) } -> remove_cvref_same_as<::std::optional<int>>;
	};

	// A reference on the right-hand side works too (with the old one-sided implementation
	// such a constraint was always false).
	template <typename gen_t>
	concept returns_ref_to_optional_int = requires (const gen_t & gen, int id)
	{
		{ gen.generate_constant(id) } -> remove_cvref_same_as<const ::std::optional<int> &>;
	};

	static_assert(can_generate_optional_int<value_generator>);
	static_assert(can_generate_optional_int<const_ref_generator>);
	static_assert(!can_generate_optional_int<long_generator>);
	static_assert(returns_ref_to_optional_int<value_generator>);
	static_assert(returns_ref_to_optional_int<const_ref_generator>);
	static_assert(!returns_ref_to_optional_int<long_generator>);
}

int main()
{
	::std::puts("pdn_remove_cvref_same_as_concept: all static assertions passed");
	return 0;
}
