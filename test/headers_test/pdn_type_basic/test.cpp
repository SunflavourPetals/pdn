// Tests the integer alias table, the auto_int/auto_uint resolution and the type concepts
// declared by pdn_type_basic.h.
#include <pdn/pdn_type_basic.h>

#include <cstdio>
#include <type_traits>

namespace
{
	namespace t = pdn::type;
	namespace d = pdn::type::detail;

	using pdn::unicode::u8char_t;

	struct not_an_int { };

	// Each PDN integer selects itself.
	static_assert(std::is_same_v<d::alias_sint_t<t::i8>,  t::i8>);
	static_assert(std::is_same_v<d::alias_sint_t<t::i32>, t::i32>);
	static_assert(std::is_same_v<d::alias_sint_t<t::i64>, t::i64>);
	static_assert(std::is_same_v<d::alias_uint_t<t::u8>,  t::u8>);
	static_assert(std::is_same_v<d::alias_uint_t<t::u64>, t::u64>);
	static_assert(d::sint_has_alias_v<t::i16> && d::uint_has_alias_v<t::u16>);

	// Unknown types map to void, which is the signal the simu_* aliases fall back on.
	static_assert(std::is_same_v<d::alias_sint_t<not_an_int>, void>);
	static_assert(std::is_same_v<d::alias_uint_t<not_an_int>, void>);
	static_assert(!d::sint_has_alias_v<not_an_int> && !d::uint_has_alias_v<not_an_int>);
	static_assert(std::is_same_v<d::simu_int_t<not_an_int>,  t::i32>);
	static_assert(std::is_same_v<d::simu_uint_t<not_an_int>, t::u32>);

	// auto_int mirrors int: either int is exactly a PDN integer, or it falls back to i32.
	static_assert(std::is_same_v<t::auto_int,  d::simu_int_t<int>>);
	static_assert(std::is_same_v<t::auto_uint, d::simu_uint_t<unsigned int>>);
	static_assert(std::is_same_v<t::auto_int,  t::i32> || std::is_same_v<t::auto_int, int>);
	static_assert(std::is_same_v<t::auto_uint, t::u32> || std::is_same_v<t::auto_uint, unsigned int>);

	// pdn_sint / pdn_uint
	static_assert(t::concepts::pdn_sint<t::i8> && t::concepts::pdn_sint<t::i16>
		&& t::concepts::pdn_sint<t::i32> && t::concepts::pdn_sint<t::i64>);
	static_assert(t::concepts::pdn_uint<t::u8> && t::concepts::pdn_uint<t::u16>
		&& t::concepts::pdn_uint<t::u32> && t::concepts::pdn_uint<t::u64>);
	static_assert(!t::concepts::pdn_sint<t::u32> && !t::concepts::pdn_uint<t::i32>);
	static_assert(!t::concepts::pdn_sint<t::f32> && !t::concepts::pdn_sint<t::boolean>);
	static_assert(!t::concepts::pdn_sint<void>);

	// Exact types only: cv-qualified and reference types are not matched.
	static_assert(!t::concepts::pdn_sint<const t::i8>);
	static_assert(!t::concepts::pdn_sint<t::i8&>);
	static_assert(!t::concepts::pdn_integral<const t::u64>);

	// pdn_integral / pdn_fp / pdn_bool
	static_assert(t::concepts::pdn_integral<t::i8> && t::concepts::pdn_integral<t::u64>);
	static_assert(!t::concepts::pdn_integral<t::f32> && !t::concepts::pdn_integral<t::boolean>);
	static_assert(t::concepts::pdn_fp<t::f32> && t::concepts::pdn_fp<t::f64>);
	static_assert(!t::concepts::pdn_fp<long double> && !t::concepts::pdn_fp<t::i32>);
	static_assert(t::concepts::pdn_bool<t::boolean>);
	static_assert(!t::concepts::pdn_bool<int> && !t::concepts::pdn_bool<t::character<u8char_t>>);

	// basic_type = the four "value" kinds that an entity can hold directly.
	static_assert(t::concepts::basic_type<t::i8, u8char_t>);
	static_assert(t::concepts::basic_type<t::f64, u8char_t>);
	static_assert(t::concepts::basic_type<t::boolean, u8char_t>);
	static_assert(t::concepts::basic_type<t::character<u8char_t>, u8char_t>);
	static_assert(!t::concepts::basic_type<t::character<char16_t>, u8char_t>);
	static_assert(!t::concepts::basic_type<void, u8char_t>);
	static_assert(!t::concepts::basic_type<t::character<u8char_t>&, u8char_t>);

	int failures = 0;

	void check(const bool condition, const char* what)
	{
		if (!condition)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	void test_alias_widths()
	{
		check(sizeof(t::auto_int) == sizeof(int), "auto_int has the width of int");
		check(sizeof(t::i8) == 1 && sizeof(t::i64) == 8, "PDN integers keep their widths");
		check(static_cast<long long>(t::i64(-1)) == -1, "i64 is signed");
		check(t::u64(0) < t::u64(1), "u64 is unsigned");
	}
}

int main()
{
	test_alias_widths();
	if (failures == 0)
	{
		std::printf("pdn_type_basic: all checks passed\n");
	}
	return failures;
}
