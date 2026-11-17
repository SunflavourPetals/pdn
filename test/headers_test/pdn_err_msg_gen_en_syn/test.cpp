#include <pdn/pdn_err_msg_gen_en_syn.h>

#include <cstdio>
#include <type_traits>
#include <variant>

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

    namespace rt = pdn::raw_error_message_type;
    using char_t = pdn::error_msg_char;
    using variant_t = pdn::raw_error_message_variant;
    using value_t = pdn::token_value_variant<char_t>;
    using pos = pdn::source_position;
    using pdn::error_msg_string;
    using pdn::syntax_error_code;
    using pdn::type_code;

    static_assert(::std::is_same_v<decltype(pdn::detail::err_msg_gen_en(
        syntax_error_code::entity_redefine, pos{}, variant_t{ rt::identifier{ u8"x" } })), error_msg_string>);

    auto gen(syntax_error_code code, variant_t raw) -> error_msg_string
    {
        return pdn::detail::err_msg_gen_en(code, pos{}, raw);
    }

    auto token_of(value_t value) -> variant_t
    {
        return variant_t{ rt::error_token{ pdn::token<char_t>{ {}, pdn::pdn_token_code::literal_integer, ::std::move(value) } } };
    }

    auto casting(type_code source, type_code target, value_t operand) -> variant_t
    {
        return variant_t{ rt::casting_msg{ ::std::move(operand), source, target } };
    }

    void test_messages()
    {
        check(gen(syntax_error_code::entity_redefine, variant_t{ rt::identifier{ u8"x" } })
            == u8"entity \"x\" is redefined in this scope", "entity redefine");
        check(gen(syntax_error_code::casting_domain_error,
                casting(pdn::type_code::i32, pdn::type_code::u8, value_t{ pdn::type::i32{ 300 } }))
            == u8"cannot cast 300 from i32 to u8: out of range [0, 255]", "casting domain error");
        check(gen(syntax_error_code::illegal_cast,
                casting(pdn::type_code::i32, pdn::type_code::u8, value_t{ pdn::type::i32{ 300 } }))
            == u8"cannot cast 300 (i32) to u8", "illegal cast with a scalar operand");
        check(gen(syntax_error_code::illegal_cast,
                casting(pdn::type_code::list, pdn::type_code::i32, value_t{ ::std::monostate{} }))
            == u8"cannot cast list to i32", "illegal cast with a list operand");
        check(gen(syntax_error_code::at_value_not_found, variant_t{ rt::identifier{ u8"a b" } })
            == u8"at value \"@a b\" not found", "at value not found");
        check(gen(syntax_error_code::expect_entity_name, token_of(value_t{ pdn::type::i32{ 3 } }))
            == u8"expected entity name (identifier), but got integer-val with value 3", "expect entity name");
        check(gen(syntax_error_code::expect_expression, token_of(value_t{ pdn::type::i32{ 3 } }))
            == u8"expected expression, but got integer-val with value 3", "expect expression");
        check(gen(syntax_error_code::expect_comma, token_of(value_t{ pdn::type::i32{ 3 } }))
            == u8"expected comma (,), but got integer-val with value 3", "expect comma");
        check(gen(syntax_error_code::expect_colon, token_of(value_t{ pdn::type::i32{ 3 } }))
            == u8"expected colon (:), but got integer-val with value 3", "expect colon");
        check(gen(syntax_error_code::expect_definition_of_named_entity, token_of(value_t{ pdn::type::i32{ 3 } }))
            == u8"expected named entity definition, but got integer-val with value 3", "expect named entity");
        check(gen(syntax_error_code::expect_definition_of_list_element, token_of(value_t{ pdn::type::i32{ 3 } }))
            == u8"expected list element, but got integer-val with value 3", "expect list element");
        check(gen(syntax_error_code::invalid_unary_operation,
                variant_t{ rt::unary_operation{ value_t{ pdn::type::i32{ 5 } }, pdn::type_code::i32, true } })
            == u8"invalid unary operation: operator- on i32 with value 5", "invalid unary operation");
        check(gen(syntax_error_code::unknown_type, variant_t{ rt::identifier{ u8"x" } })
            == u8"type \"x\" not found", "unknown type");
        check(gen(syntax_error_code::missing_right_brackets, variant_t{ ::std::monostate{} })
            == u8"missing right bracket: ]", "missing right bracket");
        check(gen(syntax_error_code::missing_right_curly_brackets, variant_t{ ::std::monostate{} })
            == u8"missing right curly bracket: }", "missing right curly bracket");
    }

    void test_token_description_without_value()
    {
        // A token without a value is described by its code alone, without the "with value" suffix.
        check(pdn::detail::err_msg_gen_en(syntax_error_code::expect_expression, pos{},
                token_of(value_t{ ::std::monostate{} }))
            == u8"expected expression, but got integer-val", "expect expression without a token value");
    }
}

int main()
{
    test_messages();
    test_token_description_without_value();
    if (fails == 0)
    {
        std::printf("all pdn_err_msg_gen_en_syn checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
