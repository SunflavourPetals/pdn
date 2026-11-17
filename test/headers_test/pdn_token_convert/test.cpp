// Tests for pdn/include/pdn/pdn_token_convert.h.
#include <pdn/pdn_token_convert.h>

#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace type = pdn::type;
namespace un = pdn::unicode;

using u8s = un::u8string;
using u16s = un::u16string;
using u8sv = un::u8string_view;
using token8 = pdn::token<char8_t>;
using token16 = pdn::token<char16_t>;

static int fails = 0;

#define CHECK(...)                                                                      \
	do                                                                                  \
	{                                                                                   \
		if (!(__VA_ARGS__))                                                             \
		{                                                                               \
			++fails;                                                                    \
			::std::printf("FAIL line %d\n", __LINE__);                                  \
		}                                                                               \
	} while (0)

static auto string_token8(u8sv text) -> token8
{
	return token8{ pdn::source_position{ 2, 3 }, pdn::pdn_token_code::literal_string,
	               pdn::make_proxy<type::string<char8_t>>(text) };
}

static void test_same_char_type_is_identity()
{
	auto t = string_token8(u8sv{ u8"hi" });
	auto u = pdn::detail::token_convert<char8_t>(t);

	CHECK(u.position == t.position);
	CHECK(u.code == t.code);
	const auto& s = ::std::get<pdn::proxy<type::string<char8_t>>>(u.value);
	CHECK(*s == u8s{ u8"hi" });

	// a default token converts to an equal default token
	auto d = pdn::detail::token_convert<char8_t>(token8{});
	CHECK(d.position == pdn::source_position{ 1, 1 });
	CHECK(d.code == pdn::pdn_token_code::invalid);
	CHECK(::std::holds_alternative<::std::monostate>(d.value));
}

static void test_string_payload_is_reencoded()
{
	auto u16 = pdn::detail::token_convert<char16_t>(string_token8(u8sv{ u8"hi" }));
	CHECK(u16.position == pdn::source_position{ 2, 3 });
	CHECK(u16.code == pdn::pdn_token_code::literal_string);
	const auto& s = ::std::get<pdn::proxy<type::string<char16_t>>>(u16.value);
	CHECK(*s == u16s{ u"hi" });
	CHECK(s->size() == 2u);

	// CJK: three UTF-8 units become a single UTF-16 unit
	auto u16_cjk = pdn::detail::token_convert<char16_t>(string_token8(u8sv{ u8"\u4E2D" }));
	const auto& c = ::std::get<pdn::proxy<type::string<char16_t>>>(u16_cjk.value);
	CHECK(c->size() == 1u);
	CHECK((*c)[0] == u'\u4E2D');

	// emoji outside the BMP: four UTF-8 units become a surrogate pair
	auto u16_emoji = pdn::detail::token_convert<char16_t>(string_token8(u8sv{ u8"\U0001F600" }));
	const auto& e = ::std::get<pdn::proxy<type::string<char16_t>>>(u16_emoji.value);
	CHECK(e->size() == 2u);

	// and back to UTF-8
	auto round_trip = pdn::detail::token_convert<char8_t>(::std::move(u16_emoji));
	const auto& r = ::std::get<pdn::proxy<type::string<char8_t>>>(round_trip.value);
	CHECK(*r == u8s{ u8"\U0001F600" });
}

static void test_scalar_and_character_payloads()
{
	token8 integer{ pdn::source_position{ 4, 5 }, pdn::pdn_token_code::literal_integer, type::i32{ -7 } };
	auto converted_integer = pdn::detail::token_convert<char16_t>(::std::move(integer));
	CHECK(converted_integer.position == pdn::source_position{ 4, 5 });
	CHECK(converted_integer.code == pdn::pdn_token_code::literal_integer);
	CHECK(::std::get<type::i32>(converted_integer.value) == -7);

	token8 character{ pdn::source_position{ 6, 7 }, pdn::pdn_token_code::literal_character,
	                  type::character<char8_t>{ reinterpret_cast<const char8_t*>(u8"x"), 1 } };
	auto converted_character = pdn::detail::token_convert<char16_t>(::std::move(character));
	const auto& ch = ::std::get<type::character<char16_t>>(converted_character.value);
	CHECK(ch.size() == 1u);
	CHECK(ch.data()[0] == u'x');
}

static void test_at_identifier_payload_is_kept()
{
	token8 at_iden{ pdn::source_position{ 8, 9 }, pdn::pdn_token_code::at_identifier,
	                pdn::detail::at_iden_string_proxy{ u8sv{ u8"i8_max" } } };
	auto converted = pdn::detail::token_convert<char16_t>(::std::move(at_iden));
	const auto& iden = ::std::get<pdn::detail::at_iden_string_proxy>(converted.value);
	CHECK(iden.get_id() == u8s{ u8"i8_max" });        // at identifiers always stay UTF-8
}

static void test_empty_string_payload()
{
	auto t = string_token8(u8sv{ u8"" });
	auto u16 = pdn::detail::token_convert<char16_t>(::std::move(t));
	const auto& s = ::std::get<pdn::proxy<type::string<char16_t>>>(u16.value);
	CHECK(s->empty());
}

static void test_types()
{
	static_assert(::std::same_as<decltype(pdn::detail::token_convert<char16_t>(::std::declval<token8>())), token16>);
	static_assert(::std::same_as<decltype(pdn::detail::token_convert<char8_t>(::std::declval<token8>())), token8>);
}

int main()
{
	test_same_char_type_is_identity();
	test_string_payload_is_reencoded();
	test_scalar_and_character_payloads();
	test_at_identifier_payload_is_kept();
	test_empty_string_payload();
	test_types();

	::std::printf(fails == 0 ? "all token_convert tests passed\n" : "token_convert tests FAILED: %d\n", fails);
	return fails == 0 ? 0 : 1;
}
