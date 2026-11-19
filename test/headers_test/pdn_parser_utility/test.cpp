#include <cstddef>
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <pdn/pdn_parser_utility.h>

namespace
{
    int fails = 0;

    void check(bool ok, const char* what)
    {
        if (!ok)
        {
            ++fails;
            std::printf("FAILED: %s\n", what);
        }
    }

    using namespace pdn;
    using pdn::unicode::u16char_t;
    using pdn::unicode::u32char_t;
    using pdn::unicode::u8char_t;
    using pdn::parser_utility::default_entity_value;
    using pdn::parser_utility::is_expr_first;
    using pdn::parser_utility::is_list_element_first;
    using pdn::parser_utility::is_negative_sign;
    using pdn::parser_utility::is_unary_operator;
    using pdn::parser_utility::to_raw_error_token;
    using pdn::parser_utility::unary_record;

    // The token stream the parser reads: dereference to a token of the requested character type and
    // advance. Comparing with the end of the stream is the caller's business, so a stream with a
    // different character type or with a non-token element does not model the concept.
    struct token_stream
    {
        token<u8char_t>* pos{};
        auto operator*() const noexcept -> token<u8char_t>& { return *pos; }
        auto operator++() noexcept -> token_stream& { ++pos; return *this; }
    };
    static_assert(concepts::token_iterator<token_stream, u8char_t>);
    static_assert(!concepts::token_iterator<token_stream, u16char_t>);
    static_assert(concepts::token_iterator<std::vector<token<u8char_t>>::const_iterator, u8char_t>);

    struct int_stream
    {
        int* pos{};
        auto operator*() const noexcept -> int& { return *pos; }
        auto operator++() noexcept -> int_stream& { ++pos; return *this; }
    };
    static_assert(!concepts::token_iterator<int_stream, u8char_t>);

    // A code unit iterator belongs to exactly one encoding.
    static_assert(concepts::utf8_code_unit_iterator<const u8char_t*>);
    static_assert(concepts::utf16_code_unit_iterator<const u16char_t*>);
    static_assert(concepts::utf32_code_unit_iterator<const u32char_t*>);
    static_assert(!concepts::utf8_code_unit_iterator<const u16char_t*>);
    static_assert(!concepts::utf16_code_unit_iterator<const u8char_t*>);
    static_assert(!concepts::utf32_code_unit_iterator<const u16char_t*>);
    static_assert(concepts::utf_code_unit_iterator<const u8char_t*>);
    static_assert(concepts::utf_code_unit_iterator<const u16char_t*>);
    static_assert(concepts::utf_code_unit_iterator<const u32char_t*>);
    static_assert(!concepts::utf_code_unit_iterator<const char*>);

    // A string view is accepted for each UTF encoding, but not for a C array (its terminating null
    // is not text) and not for a string of another character type.
    static_assert(concepts::utf_code_unit_string_view<std::u8string_view>);
    static_assert(concepts::utf_code_unit_string_view<std::u16string_view>);
    static_assert(concepts::utf_code_unit_string_view<std::u32string_view>);
    static_assert(concepts::utf_code_unit_string_view<std::u8string>);
    static_assert(!concepts::utf_code_unit_string_view<std::string>);
    static_assert(!concepts::utf_code_unit_string_view<u8char_t[4]>);
    static_assert(!concepts::utf_code_unit_string_view<int>);

    // Every token code that can begin an expression, and a sample that cannot.
    constexpr pdn_token_code expr_first[] =
    {
        pdn_token_code::at_identifier,
        pdn_token_code::minus,
        pdn_token_code::plus,
        pdn_token_code::literal_boolean,
        pdn_token_code::literal_character,
        pdn_token_code::literal_string,
        pdn_token_code::literal_floating_point,
        pdn_token_code::literal_integer,
        pdn_token_code::left_brackets,
        pdn_token_code::left_curly_brackets,
    };
    constexpr pdn_token_code not_expr_first[] =
    {
        pdn_token_code::identifier,
        pdn_token_code::colon,
        pdn_token_code::semicolon,
        pdn_token_code::comma,
        pdn_token_code::eof,
        pdn_token_code::right_brackets,
        pdn_token_code::right_curly_brackets,
        pdn_token_code::question_mark,
    };

    void test_predicates()
    {
        for (const auto c : expr_first)
        {
            check(is_expr_first(c), "an expression first token is recognized");
            check(is_list_element_first(c), "an expression first token also starts a list element");
        }
        for (const auto c : not_expr_first)
        {
            check(!is_expr_first(c), "a non expression first token is rejected");
            if (c != pdn_token_code::identifier)
            {
                check(!is_list_element_first(c), "a non expression first token is rejected in a list");
            }
        }

        // A list element may also be a named entity definition.
        check(is_list_element_first(pdn_token_code::identifier), "identifier starts a list element");
        check(!is_expr_first(pdn_token_code::identifier), "identifier does not start an expression");

        check(is_unary_operator(pdn_token_code::minus), "minus is a unary operator");
        check(is_unary_operator(pdn_token_code::plus), "plus is a unary operator");
        check(!is_unary_operator(pdn_token_code::literal_integer), "a literal is not a unary operator");
        check(is_negative_sign(pdn_token_code::minus), "minus is the negative sign");
        check(!is_negative_sign(pdn_token_code::plus), "plus is not the negative sign");
    }

    void test_default_entity_value()
    {
        const auto v = [](pdn::type_code c) { return default_entity_value<u8char_t>(c); };

        check(v(type_code::i8).type_test<type::i8>() && v(type_code::i8).as_int() == 0, "i8 zero");
        check(v(type_code::i16).type_test<type::i16>() && v(type_code::i16).as_int() == 0, "i16 zero");
        check(v(type_code::i32).type_test<type::i32>() && v(type_code::i32).as_int() == 0, "i32 zero");
        check(v(type_code::i64).type_test<type::i64>() && v(type_code::i64).as_int() == 0, "i64 zero");
        check(v(type_code::u8).type_test<type::u8>() && v(type_code::u8).as_uint() == 0, "u8 zero");
        check(v(type_code::u16).type_test<type::u16>() && v(type_code::u16).as_uint() == 0, "u16 zero");
        check(v(type_code::u32).type_test<type::u32>() && v(type_code::u32).as_uint() == 0, "u32 zero");
        check(v(type_code::u64).type_test<type::u64>() && v(type_code::u64).as_uint() == 0, "u64 zero");
        check(v(type_code::f32).type_test<type::f32>() && v(type_code::f32).as_fp() == 0.0, "f32 zero");
        check(v(type_code::f64).type_test<type::f64>() && v(type_code::f64).as_fp() == 0.0, "f64 zero");
        check(v(type_code::boolean).type_test<type::boolean>() && !v(type_code::boolean).as_bool(), "boolean false");

        // A default-constructed character holds exactly one code unit, U+0000, so it is a scalar
        // value that the serializer can write out (an empty character could not round-trip).
        const auto character = v(type_code::character);
        check(character.type_test<type::character<u8char_t>>(), "character type");
        check(character.as_char().size() == 1, "character holds one code unit");
        check(character.as_char().data()[0] == u8char_t{ 0 }, "the character is U+0000");

        // string, list and object are stored behind an engaged proxy that holds an empty value.
        const auto string = v(type_code::string);
        check(string.type_test<proxy<type::string<u8char_t>>>(), "string proxy type");
        check(string.as_string().empty(), "string is empty");
        const auto list = v(type_code::list);
        check(list.type_test<proxy<type::list<u8char_t>>>(), "list proxy type");
        check(list.as_list().empty(), "list is empty");
        const auto object = v(type_code::object);
        check(object.type_test<proxy<type::object<u8char_t>>>(), "object proxy type");
        check(object.as_object().empty(), "object is empty");

        // No usable type specifier: the value falls back to auto_int (i32 on this platform), the
        // first entity alternative, which is what a value-initialized entity holds.
        check(v(type_code::unknown).type_test<type::auto_int>() && v(type_code::unknown).as_int() == 0, "unknown is auto_int zero");
    }

    void test_to_raw_error_token()
    {
        // Same character type: code, position and payload are kept.
        const auto t8 = token<u8char_t>
        {
            .position = { 5, 7 },
            .code     = pdn_token_code::literal_integer,
            .value    = type::i32{ 42 },
        };
        const auto r8 = to_raw_error_token(t8);
        check(r8.value.code == pdn_token_code::literal_integer, "raw token keeps the code");
        check(r8.value.position.line == 5 && r8.value.position.column == 7, "raw token keeps the position");
        check(::std::get<type::i32>(r8.value.value) == 42, "raw token keeps the payload");

        // Another character type: the payload is re-expressed in the message character type.
        const auto t16 = token<u16char_t>
        {
            .code  = pdn_token_code::literal_character,
            .value = type::character<u16char_t>{ u"a", 1 },
        };
        const auto r16 = to_raw_error_token(t16);
        const auto c = ::std::get<type::character<error_msg_char>>(r16.value.value);
        check(r16.value.code == pdn_token_code::literal_character, "converted raw token keeps the code");
        check(c.to_string_view() == ::std::u8string_view{ u8"a" }, "converted raw token holds UTF-8 text");
    }

    void test_unary_record()
    {
        const unary_record none{ false };
        check(!none.has_sign, "no sign seen");
        check(none.negative_sign_count == 0, "no minus counted");
        check(!none.is_last_sign_negative, "no sign position recorded");

        unary_record rec{ true };
        check(rec.has_sign, "a sign was seen");
        rec.last_positive_sign_pos = { 2, 3 };
        rec.last_negative_sign_pos = { 2, 5 };
        rec.negative_sign_count = 3;
        rec.is_last_sign_negative = true;
        check(rec.last_positive_sign_pos.column == 3, "the last plus is kept");
        check(rec.last_negative_sign_pos.column == 5, "the last minus is kept");
        check(rec.negative_sign_count == 3, "minus signs are counted");
        check(rec.is_last_sign_negative, "the last sign is a minus");

        // The default value of source_position is the beginning of the text.
        check(source_position{}.line == 1 && source_position{}.column == 1, "sign positions default to the start");
    }
}

int main()
{
    test_predicates();
    test_default_entity_value();
    test_to_raw_error_token();
    test_unary_record();

    if (fails == 0)
    {
        std::printf("all checks passed\n");
        return 0;
    }
    std::printf("%d check(s) failed\n", fails);
    return 1;
}
