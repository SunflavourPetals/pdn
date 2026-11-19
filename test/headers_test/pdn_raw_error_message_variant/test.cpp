#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <utility>
#include <variant>

#include <pdn/pdn_raw_error_message_variant.h>

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

    using v = pdn::raw_error_message_variant;
    namespace rt = pdn::raw_error_message_type;

    template <typename type, std::size_t... index>
    constexpr bool has_alternative(std::index_sequence<index...>) noexcept
    {
        return (::std::same_as<::std::variant_alternative_t<index, v>, type> || ...);
    }

    template <typename type>
    constexpr bool has_alternative_v = has_alternative<type>(std::make_index_sequence<std::variant_size_v<v>>{});

    // monostate + the 15 payload types of pdn_raw_error_message_type.h.
    static_assert(std::variant_size_v<v> == 16);
    static_assert(::std::same_as<::std::variant_alternative_t<0, v>, ::std::monostate>);
    static_assert(has_alternative_v<rt::utf8_decode_error>);
    static_assert(has_alternative_v<rt::utf16_decode_error>);
    static_assert(has_alternative_v<rt::utf32_decode_error>);
    static_assert(has_alternative_v<rt::identifier>);
    static_assert(has_alternative_v<rt::error_token>);
    static_assert(has_alternative_v<rt::unary_operation>);
    static_assert(has_alternative_v<rt::casting_msg>);
    static_assert(has_alternative_v<rt::not_unicode_scalar_value>);
    static_assert(has_alternative_v<rt::error_string>);
    static_assert(has_alternative_v<rt::character_length_error>);
    static_assert(has_alternative_v<rt::number_end_with_separator>);
    static_assert(has_alternative_v<rt::from_chars_error>);
    static_assert(has_alternative_v<rt::escape_not_unicode_scalar_value>);
    static_assert(has_alternative_v<rt::missing_terminating_sequence>);
    static_assert(has_alternative_v<rt::delimiter_error>);

    static_assert(::std::is_default_constructible_v<v>);
    static_assert(::std::is_copy_constructible_v<v>);
    static_assert(::std::is_move_constructible_v<v>);
    static_assert(::std::is_copy_assignable_v<v>);
    static_assert(::std::is_move_assignable_v<v>);
    static_assert(::std::is_same_v<pdn::raw_error_message_variant, pdn::raw_error_message_type::detail::raw_error_message_variant>);
}

int main()
{
    v empty{};
    check(std::holds_alternative<::std::monostate>(empty), "default constructed variant holds monostate");
    check(empty.index() == 0, "monostate is alternative 0");

    v iden = rt::identifier{ .value = u8"name" };
    check(std::holds_alternative<rt::identifier>(iden), "identifier payload is stored");
    check(iden.index() == 4, "identifier is alternative 4");
    check(std::get<rt::identifier>(iden).value == u8"name", "identifier payload keeps its value");
    check(!std::holds_alternative<rt::utf8_decode_error>(iden), "identifier is not a decode error");
    check(std::get_if<rt::utf8_decode_error>(&iden) == nullptr, "get_if returns null for another alternative");

    bool threw = false;
    try
    {
        (void)std::get<rt::error_string>(iden);
    }
    catch (const ::std::bad_variant_access&)
    {
        threw = true;
    }
    check(threw, "reading a payload with the wrong type throws bad_variant_access");

    v copy = iden;
    std::get<rt::identifier>(copy).value = u8"other";
    check(std::get<rt::identifier>(iden).value == u8"name", "copy of the payload is independent");

    v moved = ::std::move(copy);
    check(std::holds_alternative<rt::identifier>(moved), "move keeps the alternative");

    v assign{};
    assign = rt::error_string{ .value = u8"boom" };
    check(std::holds_alternative<rt::error_string>(assign), "assignment switches the alternative");
    check(std::get<rt::error_string>(assign).value == u8"boom", "assigned payload keeps its value");

    v scalar = rt::not_unicode_scalar_value{ .value = 0xD800 };
    check(std::get<rt::not_unicode_scalar_value>(scalar).value == 0xD800, "lexer payload");

    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("all raw_error_message_variant checks passed\n");
    return 0;
}
