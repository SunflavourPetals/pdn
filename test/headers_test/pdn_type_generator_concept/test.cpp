// Tests for pdn/include/pdn/pdn_type_generator_concept.h.
//
// What is pinned down here:
//   * the concept accepts the shapes the library uses: a static function taking const&,
//     and an object-based (even stateful, non-const) member function -- the generator is
//     passed by value, so it does not have to be callable on a const object;
//   * both value categories of the identifier must be accepted: an rvalue-only overload
//     fails the lvalue half, a mutable-lvalue overload fails the rvalue half;
//   * the result must be convertible to type_code: int and bool are rejected, a
//     const type_code& is accepted (that is what convertible_to allows today);
//   * the real implementation, pdn::default_type_generator, satisfies the concept.

#include <pdn/pdn_type_generator_concept.h>
#include <pdn/pdn_type_generator.h>

#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	using pdn::type_code;
	using u8c = pdn::unicode::u8char_t;
	using u16c = pdn::unicode::u16char_t;
	template <typename char_t>
	using iden_t = pdn::type::string<char_t>;

	// The shape of default_type_generator: one static function taking const&.
	struct static_generator
	{
		static auto generate_type(const iden_t<u8c>&) -> type_code { return type_code::i8; }
	};

	// An object-based generator with a non-const member function and state.
	struct stateful_generator
	{
		int calls = 0;
		auto generate_type(const iden_t<u8c>&) -> type_code
		{
			++calls;
			return type_code::boolean;
		}
	};

	// Accepts both an lvalue and an rvalue, with different answers.
	struct overload_pair
	{
		static auto generate_type(const iden_t<u8c>&) -> type_code { return type_code::i8; }
		static auto generate_type(iden_t<u8c>&&) -> type_code { return type_code::i16; }
	};

	// The lvalue half cannot bind, so the concept must be false.
	struct rvalue_only
	{
		static auto generate_type(iden_t<u8c>&&) -> type_code { return type_code::i8; }
	};

	// The rvalue half cannot bind an lvalue reference, so the concept must be false.
	struct lvalue_ref_only
	{
		static auto generate_type(iden_t<u8c>&) -> type_code { return type_code::i8; }
	};

	// Wrong character type for the checked char_t.
	struct wrong_char
	{
		static auto generate_type(const iden_t<u16c>&) -> type_code { return type_code::i8; }
	};

	struct returns_int
	{
		static auto generate_type(const iden_t<u8c>&) -> int { return 1; }
	};

	struct returns_bool
	{
		static auto generate_type(const iden_t<u8c>&) -> bool { return true; }
	};

	// convertible_to accepts a reference to the right type; same_as would not.
	struct returns_ref
	{
		static constexpr type_code code = type_code::u32;
		static auto generate_type(const iden_t<u8c>&) -> const type_code& { return code; }
	};

	struct no_member {};

	static_assert(pdn::concepts::type_generator<static_generator, u8c>);
	static_assert(pdn::concepts::type_generator<stateful_generator, u8c>);
	static_assert(pdn::concepts::type_generator<overload_pair, u8c>);
	static_assert(pdn::concepts::type_generator<returns_ref, u8c>);

	static_assert(!pdn::concepts::type_generator<rvalue_only, u8c>);
	static_assert(!pdn::concepts::type_generator<lvalue_ref_only, u8c>);
	static_assert(!pdn::concepts::type_generator<wrong_char, u8c>);
	static_assert(!pdn::concepts::type_generator<returns_int, u8c>);
	static_assert(!pdn::concepts::type_generator<returns_bool, u8c>);
	static_assert(!pdn::concepts::type_generator<no_member, u8c>);
	static_assert(!pdn::concepts::type_generator<int, u8c>);

	// The library's own generator satisfies it, for every character type.
	static_assert(pdn::concepts::type_generator<pdn::default_type_generator<u8c>, u8c>);
	static_assert(pdn::concepts::type_generator<pdn::default_type_generator<u16c>, u16c>);

	// Calling through the constraint is possible for a stateful, non-const generator.
	template <typename gen_t>
	auto probe(gen_t& gen, const iden_t<u8c>& iden) -> type_code
		requires pdn::concepts::type_generator<gen_t, u8c>
	{
		return gen.generate_type(iden);
	}

	int failures = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	void test_stateful_generator()
	{
		stateful_generator gen{};
		check(gen.calls == 0, "stateful generator starts unused");
		check(probe(gen, iden_t<u8c>{ u8"anything" }) == type_code::boolean, "stateful generator answers");
		check(gen.calls == 1, "the constrained call went through the non-const member");

		check(static_generator::generate_type(iden_t<u8c>{ u8"x" }) == type_code::i8, "static generator answers");

		// The rvalue overload is the one that must be picked for a prvalue.
		check(overload_pair::generate_type(iden_t<u8c>{ u8"prvalue" }) == type_code::i16, "prvalue selects the && overload");
		const iden_t<u8c> iden{ u8"lvalue" };
		check(overload_pair::generate_type(iden) == type_code::i8, "lvalue selects the const& overload");
	}

	void test_real_generator()
	{
		// Smoke check only: type_generator_std.h is reviewed separately.
		auto gen = pdn::default_type_generator<u8c>{};
		check(gen.generate_type(iden_t<u8c>{ u8"i8" }) == type_code::i8, "the default generator knows i8");
		check(gen.generate_type(iden_t<u8c>{ u8"no-such-type" }) == type_code::unknown, "an unknown name answers unknown");
	}
}

int main()
{
	test_stateful_generator();
	test_real_generator();

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}

	std::printf("all checks passed\n");
	return 0;
}
