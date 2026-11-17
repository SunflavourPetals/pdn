#include <pdn/spdn.h>
#include <pdn/pdn_token.h>
#include <pdn/pdn_token_value_variant.h>
#include <pdn/pdn_token_value_variant_convert.h>

#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace
{
	using u8_t = char8_t;
	using u16_t = char16_t;
	using u32_t = char32_t;

	using v8 = pdn::token_value_variant<u8_t>;
	using v16 = pdn::token_value_variant<u16_t>;
	using v32 = pdn::token_value_variant<u32_t>;

	int fails = 0;
}
#define CHECK(cond) do { if (!(cond)) { std::printf("FAILED: %s (line %d)\n", #cond, __LINE__); ++fails; } } while (0)

namespace
{
	template <typename variant_t, typename t, ::std::size_t... i>
	constexpr bool has_alternative_impl(::std::index_sequence<i...>)
	{
		return (::std::is_same_v<::std::variant_alternative_t<i, variant_t>, t> || ...);
	}

	template <typename variant_t, typename t>
	constexpr bool has_alternative()
	{
		return has_alternative_impl<variant_t, t>(::std::make_index_sequence<::std::variant_size_v<variant_t>>{});
	}

	// 15 alternatives: monostate, the 12 scalar types, the string proxy and the at-identifier payload
	static_assert(::std::variant_size_v<v8> == 15);
	static_assert(::std::same_as<::std::variant_alternative_t<0, v8>, ::std::monostate>);

	static_assert(has_alternative<v8, pdn::type::i8>());
	static_assert(has_alternative<v8, pdn::type::i16>());
	static_assert(has_alternative<v8, pdn::type::i32>());
	static_assert(has_alternative<v8, pdn::type::i64>());
	static_assert(has_alternative<v8, pdn::type::u8>());
	static_assert(has_alternative<v8, pdn::type::u16>());
	static_assert(has_alternative<v8, pdn::type::u32>());
	static_assert(has_alternative<v8, pdn::type::u64>());
	static_assert(has_alternative<v8, pdn::type::f32>());
	static_assert(has_alternative<v8, pdn::type::f64>());
	static_assert(has_alternative<v8, pdn::type::boolean>());
	static_assert(has_alternative<v8, pdn::type::character<u8_t>>());
	static_assert(has_alternative<v8, pdn::proxy<pdn::type::string<u8_t>>>());
	static_assert(has_alternative<v8, pdn::detail::at_iden_string_proxy>());

	// the scalar alternatives do not depend on char_t, the two payloads that hold text do
	static_assert(::std::same_as<::std::variant_alternative_t<12, v16>, pdn::type::character<u16_t>>);
	static_assert(::std::same_as<::std::variant_alternative_t<13, v16>, pdn::proxy<pdn::type::string<u16_t>>>);
	static_assert(::std::same_as<::std::variant_alternative_t<13, v32>, pdn::proxy<pdn::type::string<u32_t>>>);

	// the at-identifier text is UTF-8 whatever the source character type is
	static_assert(::std::same_as<::std::variant_alternative_t<14, v8>,
	                             ::std::variant_alternative_t<14, v16>>);
	static_assert(::std::same_as<::std::variant_alternative_t<14, v8>,
	                             ::std::variant_alternative_t<14, v32>>);
}

namespace
{
	void test_default_and_scalars()
	{
		v8 v{};
		CHECK(v.index() == 0);
		CHECK(::std::holds_alternative<::std::monostate>(v));

		v = pdn::type::i8{ -5 };
		CHECK(v.index() == 1);
		CHECK(::std::get<pdn::type::i8>(v) == -5);

		v = pdn::type::u64{ 42 };
		CHECK(::std::get<pdn::type::u64>(v) == 42);

		v = pdn::type::f32{ 1.5f };
		CHECK(::std::get<pdn::type::f32>(v) == 1.5f);

		v = pdn::type::boolean{ true };
		CHECK(::std::get<pdn::type::boolean>(v));

		// a value-less token starts as monostate
		pdn::token<u8_t> tk{};
		CHECK(tk.value.index() == 0);
	}

	void test_at_iden_payload()
	{
		pdn::detail::at_iden_string_proxy def{};
		CHECK(def.get_id().empty());

		// a string literal decays to the pointer constructor instead of being ambiguous
		// between the string_view and the string&& constructor
		pdn::detail::at_iden_string_proxy from_lit{ u8"@i32_max" };
		CHECK(from_lit.get_id() == u8"@i32_max");

		pdn::detail::at_iden_string_proxy from_view{ std::u8string_view{ u8"@true" } };
		CHECK(from_view.get_id() == u8"@true");

		std::u8string text{ u8"@pi" };
		const auto old_data = text.data();
		pdn::detail::at_iden_string_proxy from_owned{ ::std::move(text) };
		CHECK(from_owned.get_id() == u8"@pi");
		CHECK(from_owned.get_id().data() == old_data || from_owned.get_id() == u8"@pi");

		// the proxy owns the string, so copying the payload copies the text
		pdn::detail::at_iden_string_proxy copy{ from_owned };
		CHECK(copy.get_id() == from_owned.get_id());
		CHECK(copy.get_id().data() != from_owned.get_id().data());

		// the non-const accessor hands out the mutable text
		copy.get_id().push_back(u8'!');
		CHECK(copy.get_id() == u8"@pi!");
		CHECK(from_owned.get_id() == u8"@pi");
	}

	void test_variant_holds_at_iden()
	{
		v8 v = std::u8string_view{ u8"@name" };
		CHECK(v.index() == 14);
		CHECK(::std::get<pdn::detail::at_iden_string_proxy>(v).get_id() == u8"@name");
	}

	void test_convert()
	{
		// same character type: the payload is returned as is
		v8 same = pdn::detail::token_value_variant_convert<u8_t>(v8{ pdn::type::i8{ 3 } });
		CHECK(::std::get<pdn::type::i8>(same) == 3);

		// the string proxy is re-encoded for the target character type
		v8 src = pdn::make_proxy<pdn::type::string<u8_t>>(std::u8string_view{ u8"\u4E2D" });
		v16 conv_str = pdn::detail::token_value_variant_convert<u16_t>(::std::move(src));
		CHECK(conv_str.index() == 13);
		CHECK(::std::get<pdn::proxy<pdn::type::string<u16_t>>>(conv_str)->size() == 1);

		// the at-identifier text is UTF-8 by design, so it is not re-encoded
		v16 conv_at = pdn::detail::token_value_variant_convert<u16_t>(v8{ std::u8string_view{ u8"@\u03C0" } });
		CHECK(conv_at.index() == 14);
		CHECK(::std::get<pdn::detail::at_iden_string_proxy>(conv_at).get_id() == u8"@\u03C0");
	}

	void test_end_to_end()
	{
		auto r = pdn::parse(std::u8string_view{ u8"x: @i32_max\ny: @true\nz: @pi\n" }, pdn::utf8_tag);
		CHECK(r.success());
		if (!r.success())
		{
			return;
		}

		auto obj = pdn::as_object(r.value());
		CHECK(obj.size() == 3);
		CHECK(obj.at(u8"x").as_int() == ::std::numeric_limits<::std::int32_t>::max());
		CHECK(obj.at(u8"y").as_bool());
		CHECK(obj.at(u8"z").as_fp() > 3.14 && obj.at(u8"z").as_fp() < 3.15);

		// at-identifiers are resolved while parsing, so the value is what comes back out.
		// i32 is an auto type and prints as a plain number; booleans and pi keep their
		// canonical at-identifier spelling in the serializer's table.
		auto text = pdn::make_u8serializer().serialize(pdn::as_object(r.value()));
		CHECK(text.find(u8"x: 2147483647;") != std::u8string::npos);
		CHECK(text.find(u8"y: @true;") != std::u8string::npos);
		CHECK(text.find(u8"z: @pi;") != std::u8string::npos);

		// an unknown at-identifier is a syntax error, not a silent zero
		auto bad = pdn::parse(std::u8string_view{ u8"q: @no_such_constant" }, pdn::utf8_tag);
		CHECK(!bad.success());
	}
}

int main()
{
	test_default_and_scalars();
	test_at_iden_payload();
	test_variant_holds_at_iden();
	test_convert();
	test_end_to_end();

	if (fails != 0)
	{
		std::printf("%d check(s) failed\n", fails);
	}
	return fails == 0 ? 0 : 1;
}
