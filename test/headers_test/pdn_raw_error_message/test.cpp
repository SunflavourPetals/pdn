#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <utility>
#include <variant>

#include <pdn/pdn_raw_error_message.h>

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

    using msg = pdn::raw_error_message;
    using pdn::error_code_variant;
    using pdn::raw_error_message_variant;
    using pdn::source_position;
    namespace rt = pdn::raw_error_message_type;

    // An aggregate of exactly the three things a reporting site knows, in the order the three
    // producers use (pdn_code_point_iterator.h, pdn_lexer.h and pdn_parser.h).
    static_assert(::std::is_aggregate_v<msg>);
    static_assert(::std::is_same_v<decltype(msg::error_code), error_code_variant>);
    static_assert(::std::is_same_v<decltype(msg::position), source_position>);
    static_assert(::std::is_same_v<decltype(msg::raw_error_message), raw_error_message_variant>);
    static_assert(::std::is_copy_constructible_v<msg>);
    static_assert(::std::is_move_constructible_v<msg>);
    static_assert(::std::is_copy_assignable_v<msg>);
    static_assert(::std::is_move_assignable_v<msg>);

    msg make_syntax_error()
    {
        return msg{
            pdn::syntax_error_code::entity_redefine,
            source_position{ 3, 7 },
            rt::identifier{ u8"abc" }
        };
    }

    void test_syntax_error()
    {
        const auto m = make_syntax_error();
        check(::std::holds_alternative<pdn::syntax_error_code>(m.error_code), "syntax error code alternative");
        check(::std::get<pdn::syntax_error_code>(m.error_code) == pdn::syntax_error_code::entity_redefine,
              "syntax error code value");
        check(m.position == source_position{ 3, 7 }, "syntax error position");
        check(::std::holds_alternative<rt::identifier>(m.raw_error_message), "syntax payload alternative");
        check(::std::get<rt::identifier>(m.raw_error_message).value == u8"abc", "syntax payload value");
    }

    void test_lexical_error()
    {
        const msg m{
            pdn::lexical_error_code::unacceptable_character,
            source_position{ 2, 1 },
            rt::error_string{ u8"x" }
        };
        check(::std::holds_alternative<pdn::lexical_error_code>(m.error_code), "lexical error code alternative");
        check(::std::get<pdn::lexical_error_code>(m.error_code) == pdn::lexical_error_code::unacceptable_character,
              "lexical error code value");
        check(m.position == source_position{ 2, 1 }, "lexical error position");
        check(::std::get<rt::error_string>(m.raw_error_message).value == u8"x", "lexical payload value");
    }

    void test_decoding_error()
    {
        const msg m{
            pdn::unicode::utf8::decode_error_code::non_shortest_sequence,
            source_position{ 1, 5 },
            rt::utf8_decode_error{
                pdn::unicode::utf8::decode_result{},
                pdn::unicode::utf8::code_unit_t{ 0x80 },
                ::std::size_t{ 4 }
            }
        };
        check(::std::holds_alternative<pdn::unicode::utf8::decode_error_code>(m.error_code),
              "decoding error code alternative");
        check(::std::get<pdn::unicode::utf8::decode_error_code>(m.error_code)
                  == pdn::unicode::utf8::decode_error_code::non_shortest_sequence,
              "decoding error code value");
        check(m.position == source_position{ 1, 5 }, "decoding error position");
        const auto& payload = ::std::get<rt::utf8_decode_error>(m.raw_error_message);
        check(payload.last_code_unit == pdn::unicode::utf8::code_unit_t{ 0x80 }, "decoding payload last code unit");
        check(payload.last_code_unit_offset == 4, "decoding payload offset");
    }

    void test_copy_and_move()
    {
        const auto original = make_syntax_error();

        auto copy = original;
        copy.position = source_position{ 9, 9 };
        check(original.position == source_position{ 3, 7 }, "copy does not change the original position");
        check(::std::get<rt::identifier>(copy.raw_error_message).value == u8"abc", "copy keeps the payload");

        auto moved = ::std::move(copy);
        check(::std::get<rt::identifier>(moved.raw_error_message).value == u8"abc", "move keeps the payload");

        msg assigned{};
        assigned = original;
        check(::std::get<rt::identifier>(assigned.raw_error_message).value == u8"abc", "assignment keeps the payload");
        check(assigned.position == source_position{ 3, 7 }, "assignment keeps the position");
    }
}

int main()
{
    test_syntax_error();
    test_lexical_error();
    test_decoding_error();
    test_copy_and_move();

    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("all raw_error_message checks passed\n");
    return 0;
}
