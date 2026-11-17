#include <pdn/pdn_type_character.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

#include <pdn/spdn.h>

namespace
{
	using u8c  = pdn::type::character<char8_t>;
	using u16c = pdn::type::character<char16_t>;
	using u32c = pdn::type::character<char32_t>;

	int fails = 0;
}
#define CHECK(cond) do { if (!(cond)) { std::printf("FAILED: %s (line %d)\n", #cond, __LINE__); ++fails; } } while (0)

namespace
{
	constexpr u8c  k_zhong(u8"\u4E2D", 3);
	constexpr u8c  k_a(u8"a", 1);
	constexpr u16c k_pair(u"\U0001F600", 2);
	constexpr u32c k_cp(U"\u4E2D", 1);

	// one scalar value uses at most 4/2/1 code units in utf-8/utf-16/utf-32
	static_assert(u8c::max_size() == 4);
	static_assert(u16c::max_size() == 2);
	static_assert(u32c::max_size() == 1);

	// member aliases
	static_assert(::std::same_as<u8c::value_type, char8_t>);
	static_assert(::std::same_as<u16c::value_type, char16_t>);
	static_assert(::std::same_as<u32c::value_type, char32_t>);
	static_assert(::std::same_as<u8c::const_pointer, const char8_t*>);
	static_assert(::std::same_as<u8c::pointer, char8_t*>);
	static_assert(::std::same_as<u8c::small_size_type, ::std::uint32_t>);
	static_assert(sizeof(u8c) == 8);
	static_assert(!::std::is_trivially_copyable_v<u8c>); // copy ctor is user-provided

	// the default character is one NUL code unit, not an empty buffer
	static_assert(u8c{}.size() == 1);
	static_assert(u8c{}.data()[0] == u8'\0');
	static_assert(u16c{} == u16c(u"\0", 1));
	static_assert(u32c{} == u32c(U"\0", 1));

	// the code units are stored unchanged
	static_assert(k_zhong.size() == 3);
	static_assert(k_zhong.data()[0] == 0xE4);
	static_assert(k_zhong.data()[1] == 0xB8);
	static_assert(k_zhong.data()[2] == 0xAD);
	static_assert(k_zhong.to_string_view() == u8"\u4E2D");
	static_assert(k_pair.size() == 2);
	static_assert(k_pair == u16c(u"\U0001F600", 2));
	static_assert(k_cp.size() == 1);
	static_assert(k_cp.data()[0] == char32_t(0x4E2D));

	// equality compares the stored code units
	static_assert(k_a == u8c(u8"a", 1));
	static_assert(!(k_a == k_zhong));
	static_assert(u8c(u8"a", 1) != u8c(u8"a\0", 2));
	static_assert(k_a.to_string_view() == u8"a");

	void test_truncation()
	{
		// count is truncated to max_size() without a diagnostic
		const char8_t five[] = { 0xF8, 0x88, 0x80, 0x80, 0x80 };
		u8c c(five, 5);
		CHECK(c.size() == 4);
		CHECK(c.data()[3] == 0x80);

		const char16_t three[] = { 0xD83D, 0xDE00, 0x0041 };
		u16c s(three, 3);
		CHECK(s.size() == 2);
		CHECK(s.data()[1] == 0xDE00);

		const char32_t two[] = { 0x41, 0x42 };
		u32c cp(two, 2);
		CHECK(cp.size() == 1);
		CHECK(cp.data()[0] == 0x41);
	}

	void test_empty_and_unvalidated()
	{
		// a zero count produces a character holding no scalar value at all
		u8c empty(u8"abc", 0);
		CHECK(empty.size() == 0);
		CHECK(empty.to_string_view().empty());
		CHECK(!(empty == u8c{})); // differs from the default (one NUL)

		// invalid sequences are stored as-is: the type does not validate scalars
		const char16_t lone_lead[]  = { 0xD800 };
		const char16_t lone_trail[] = { 0xDC00 };
		u16c lead(lone_lead, 1);
		u16c trail(lone_trail, 1);
		CHECK(lead.size() == 1);
		CHECK(lead.data()[0] == 0xD800);
		CHECK(trail.size() == 1);
		CHECK(!(lead == trail));

		const char8_t overlong[] = { 0xC0, 0x80 };
		u8c ov(overlong, 2);
		CHECK(ov.size() == 2);
	}

	void test_copy_semantics()
	{
		u8c a = k_zhong;
		CHECK(a == k_zhong);
		u8c b{};
		b = a;
		CHECK(b == k_zhong);
		a = u8c{};
		CHECK(!(a == k_zhong));
		CHECK(b == k_zhong); // deep copy: no aliasing between the two objects
	}

	void test_accessors_through_entity()
	{
		auto src = pdn::parse(::std::u8string_view{ u8"n '\\u0000';\na 'a';\nz '\\u4E2D';\ni 5;\n" }, pdn::utf8_tag);
		CHECK(src);
		if (!src) return;
		auto& obj = *src;

		// character -> boolean: NUL is false, any other character is true
		CHECK(pdn::as_bool(obj[u8"n"]) == false);
		CHECK(pdn::as_bool(obj[u8"a"]) == true);

		// character does not convert to numbers (reference: only integers, floats
		// and boolean take part in the conversion rules)
		CHECK(pdn::as_int(obj[u8"a"]) == 0);
		CHECK(pdn::as_fp(obj[u8"a"]) == 0.0);

		// as_char returns the character itself, or a default (one NUL) otherwise
		CHECK(pdn::as_char(obj[u8"a"]) == k_a);
		CHECK(pdn::as_char(obj[u8"z"]).to_string_view() == u8"\u4E2D");
		auto fallback = pdn::as_char(obj[u8"i"]);
		CHECK(fallback.size() == 1);
		CHECK(fallback == u8c{});

		// empty character (only reachable from user code) still counts as true
		u8c empty(u8"abc", 0);
		CHECK(pdn::detail::as_bool<char8_t>(empty) == true);
	}

	void test_serialization()
	{
		using s8 = pdn::type::string<char8_t>;
		auto fmt = [](auto val) {
			s8 out;
			pdn::detail::to_pdn_format(out, val);
			return ::std::string(out.begin(), out.end());
		};
		CHECK(fmt(k_a) == "'a'");
		CHECK(fmt(k_zhong) == "'\xE4\xB8\xAD'");
		CHECK(fmt(u8c{}) == "'\\0'");

		// anything that does not decode to a scalar value becomes replacement char
		const char16_t lone[] = { 0xD800 };
		CHECK(fmt(u16c(lone, 1)) == "'\xEF\xBF\xBD'");
		CHECK(fmt(u8c(u8"abc", 0)) == "'\xEF\xBF\xBD'");
		const char8_t overlong[] = { 0xC0, 0x80 };
		CHECK(fmt(u8c(overlong, 2)) == "'\xEF\xBF\xBD'");
	}

	void test_round_trip()
	{
		auto src = pdn::parse(::std::u8string_view{ u8"c1 'a';\nc2 '\\u4E2D';\nc3 '\\u0000';\n" }, pdn::utf8_tag);
		CHECK(src);
		if (!src) return;
		auto& obj = *src;
		CHECK(pdn::as_object(obj).size() == 3);

		auto text = pdn::make_u8serializer().serialize(pdn::as_object(obj));
		auto again = pdn::parse(::std::u8string_view{ text }, pdn::utf8_tag);
		CHECK(again);
		if (again)
		{
			CHECK(pdn::as_bool((*again)[u8"c1"]) == true);
			CHECK(pdn::as_char((*again)[u8"c2"]).to_string_view() == u8"\u4E2D");
			CHECK(pdn::as_bool((*again)[u8"c3"]) == false);
		}
	}
}

int main()
{
	test_truncation();
	test_empty_and_unvalidated();
	test_copy_semantics();
	test_accessors_through_entity();
	test_serialization();
	test_round_trip();
	if (fails != 0)
	{
		std::printf("%d check(s) failed\n", fails);
		return 1;
	}
	return 0;
}
