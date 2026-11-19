// Tests the type aliases and the object container alias declared by pdn_type_config.h.
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifdef PDN_USER_OBJECT
#include <pdn/pdn_ordered_map.h>
#endif
#include <pdn/pdn_type_config.h>

namespace
{
	namespace t = pdn::type::config;

	using namespace std::literals;
	using pdn::unicode::u16string_view;
	using pdn::unicode::u32string_view;
	using pdn::unicode::u8string_view;

	static_assert(std::is_same_v<t::i8,  std::int8_t>);
	static_assert(std::is_same_v<t::i16, std::int16_t>);
	static_assert(std::is_same_v<t::i32, std::int32_t>);
	static_assert(std::is_same_v<t::i64, std::int64_t>);
	static_assert(std::is_same_v<t::u8,  std::uint8_t>);
	static_assert(std::is_same_v<t::u16, std::uint16_t>);
	static_assert(std::is_same_v<t::u32, std::uint32_t>);
	static_assert(std::is_same_v<t::u64, std::uint64_t>);
	static_assert(sizeof(t::i8) == 1 && sizeof(t::i16) == 2 && sizeof(t::i32) == 4 && sizeof(t::i64) == 8);
	static_assert(std::is_same_v<t::f32, float>);
	static_assert(std::is_same_v<t::f64, double>);
	static_assert(std::is_same_v<t::boolean, bool>);

	static_assert(std::is_same_v<t::character<char8_t>,  pdn::type::detail::character<char8_t>>);
	static_assert(std::is_same_v<t::string<char8_t>,     std::u8string>);
	static_assert(std::is_same_v<t::string<char16_t>,    std::u16string>);
	static_assert(std::is_same_v<t::list<int>,           std::vector<int>>);

	// key_hasher is transparent and hashes text through a view, not through a key object.
	static_assert(std::is_same_v<t::key_hasher::is_transparent, void>);
	static_assert(std::is_invocable_r_v<std::size_t, const t::key_hasher&, u8string_view>);
	static_assert(std::is_invocable_r_v<std::size_t, const t::key_hasher&, u16string_view>);
	static_assert(std::is_invocable_r_v<std::size_t, const t::key_hasher&, u32string_view>);

	using u8map = t::object<std::u8string, int>;

#ifdef PDN_USER_OBJECT
	static_assert(std::is_same_v<u8map, pdn::ordered_map<std::u8string, int>>);
#else
	static_assert(std::is_same_v<u8map, std::unordered_map<std::u8string, int, t::key_hasher, std::equal_to<>>>);
#endif

	int failures = 0;

	void check(const bool condition, const char* what)
	{
		if (!condition)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	void test_key_hasher()
	{
		const t::key_hasher hasher{};
		check(hasher(u8"alpha"sv) == hasher(std::u8string{ u8"alpha" }),
			"a view and the equal string hash the same");
		check(hasher(u8"alpha"sv) != hasher(u8"beta"sv), "different text hashes differently");
	}

	void test_heterogeneous_lookup()
	{
		u8map m;
		m[std::u8string{ u8"alpha" }] = 1;
		m[std::u8string{ u8"beta" }]  = 2;

		const u8string_view key = u8"alpha";
		check(m.find(key) != m.end(), "find by view");
		check(m.count(key) == 1, "count by view");
		check(m.contains(key), "contains by view");
		check(m.find(u8"beta"sv)->second == 2, "find the second element by view");
		check(!m.contains(u8"gamma"sv), "absent key is not found");
		check(m.size() == 2, "size is unchanged by lookups");
	}

	void test_utf16_and_utf32_keys()
	{
		t::object<std::u16string, int> m16;
		m16[std::u16string{ u"zhong" }] = 7;
		check(m16.find(u"zhong"sv) != m16.end(), "utf-16 key found by view");
		check(m16.find(u"nope"sv) == m16.end(), "absent utf-16 key");

		t::object<std::u32string, int> m32;
		m32[std::u32string{ U"zhong" }] = 8;
		check(m32.find(U"zhong"sv) != m32.end(), "utf-32 key found by view");
	}

	void test_iteration()
	{
		u8map m;
		m[std::u8string{ u8"beta" }]  = 1;
		m[std::u8string{ u8"alpha" }] = 2;
		m[std::u8string{ u8"mid" }]   = 3;
		int seen = 0;
		for (const auto& [key, value] : m)
		{
			(void)key;
			(void)value;
			++seen;
		}
		check(seen == 3, "iteration visits every element");
#ifdef PDN_USER_OBJECT
		const int expected[] = { 1, 2, 3 };
		int index = 0;
		for (const auto& [key, value] : m)
		{
			(void)key;
			check(index < 3 && value == expected[index], "ordered map keeps insertion order");
			++index;
		}
#endif
	}
}

int main()
{
	test_key_hasher();
	test_heterogeneous_lookup();
	test_utf16_and_utf32_keys();
	test_iteration();
	if (failures == 0)
	{
		std::printf("pdn_type_config: all checks passed\n");
	}
	return failures;
}
