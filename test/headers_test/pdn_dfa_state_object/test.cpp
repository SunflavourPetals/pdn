#include <array>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <type_traits>
#include <utility>

#include <pdn/pdn_dfa_state_object.h>

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

    constexpr pdn::dfa_state_object make_state(pdn::dfa_state_code code) noexcept
    {
        return pdn::dfa_state_object{ .state_code = code };
    }

    // A transformer that does not touch the code point: enough to prove the pointer is stored, callable
    // through the member and preserved by copies.
    pdn::dfa_state_object identity_transformer(pdn::unicode::code_point_t) noexcept
    {
        return pdn::dfa_state_object{ .state_code = pdn::dfa_state_code::start };
    }

    using dso = pdn::dfa_state_object;
    using dsc = pdn::dfa_state_code;
    using ptc = pdn::pdn_token_code;

    static_assert(std::is_aggregate_v<dso>);
    static_assert(std::is_trivially_copyable_v<dso>);
    static_assert(std::is_standard_layout_v<dso>);
    static_assert(std::is_nothrow_copy_assignable_v<dso>);
    static_assert(std::is_nothrow_move_assignable_v<dso>);
    static_assert(std::is_same_v<dso::transformer_ptr, dso (*)(pdn::unicode::code_point_t) noexcept>);

    // Default constructed: unmatched state, invalid token, no transformer.
    static_assert(dso{}.state_code == dsc::unmatched);
    static_assert(dso{}.token_code == ptc::invalid);
    static_assert(dso{}.transformer == nullptr);

    // Prediction wrappers follow the state code only.
    static_assert(!dso{}.is_final());
    static_assert(!dso{}.is_error());
    static_assert(make_state(dsc::start).is_final());
    static_assert(!make_state(dsc::start).is_error());
    static_assert(make_state(dsc::line_comment).is_final());
    static_assert(make_state(dsc::flag_error_state_begin).is_error());
    static_assert(!make_state(dsc::flag_error_state_begin).is_final());
    static_assert(make_state(dsc::flag_error_state_end).is_error());
    static_assert(!make_state(dsc::unacceptable_character).is_error());

    // Equality is state identity: token code and transformer are properties of that state.
    static_assert(dso{ .state_code = dsc::start } == dso{ .state_code = dsc::start, .token_code = ptc::eof });
    static_assert(dso{ .state_code = dsc::start } != dso{ .state_code = dsc::line_comment });
    static_assert(dso{} == dso{});
    static_assert(dso{} != make_state(dsc::start));

    void test_layout()
    {
        // Two enum values then the pointer: 4 + 4 + 8 with 8 byte alignment. Pinned so that adding a wide
        // member without thinking about table size is noticed.
        check(sizeof(dso) == 16, "sizeof(dfa_state_object) == 16");
        check(alignof(dso) == alignof(void*), "alignment follows the pointer");
        check(offsetof(dso, state_code) == 0, "state_code first");
        check(offsetof(dso, token_code) == 4, "token_code second");
        check(offsetof(dso, transformer) == 8, "transformer last");
    }

    void test_default_object()
    {
        const dso state{};
        check(state.state_code == dsc::unmatched, "default state is unmatched");
        check(state.token_code == ptc::invalid, "default token is invalid");
        check(state.transformer == nullptr, "default transformer is null");
        check(!state.is_final(), "default is not final");
        check(!state.is_error(), "default is not error");

        // The object exists so that members can be value initialized; a null transformer must never be
        // called. We only prove that the check is possible at all.
        check(state.transformer == nullptr, "caller can test the transformer for nullptr");
    }

    void test_transformer()
    {
        const dso state{ .state_code = dsc::string, .token_code = ptc::literal_string, .transformer = identity_transformer };
        check(state.transformer != nullptr, "transformer stored");

        const dso next = state.transformer(U'x');
        check(next.state_code == dsc::start, "transformer callable through the member");
        check(next.is_final(), "transformer result is a final state");

        // Copies keep the function pointer, comparisons ignore it.
        dso copy = state;
        check(copy.transformer == state.transformer, "copy keeps the transformer");
        check(copy == state, "copy compares equal");
        check(copy == dso{ .state_code = dsc::string }, "comparison ignores the transformer");
    }

    void test_equality_ignores_token()
    {
        const dso with_token{ .state_code = dsc::identifier_string, .token_code = ptc::identifier };
        const dso without_token{ .state_code = dsc::identifier_string };
        check(with_token == without_token, "same state code compares equal regardless of token code");
        check(!(with_token != without_token), "operator!= agrees");
        check(with_token.token_code != without_token.token_code, "token codes really differ");

        const dso other{ .state_code = dsc::string };
        check(with_token != other, "different state codes differ");
    }

    void test_state_predicates()
    {
        struct case_t
        {
            dsc code;
            bool final;
            bool error;
        };

        constexpr std::array<case_t, 8> cases{
            case_t{ dsc::unmatched, false, false },
            case_t{ dsc::unacceptable_character, false, false },
            case_t{ dsc::infinity, false, false },
            case_t{ dsc::flag_error_state_begin, false, true },
            case_t{ dsc::flag_error_state_end, false, true },
            case_t{ dsc::start, true, false },
            case_t{ dsc::line_comment, true, false },
            case_t{ dsc::flag_final_state_begin, true, false },
        };

        for (const case_t& c : cases)
        {
            const dso state{ .state_code = c.code };
            check(state.is_final() == c.final, "is_final matches the state code");
            check(state.is_error() == c.error, "is_error matches the state code");
        }
    }
}

int main()
{
    test_layout();
    test_default_object();
    test_transformer();
    test_equality_ignores_token();
    test_state_predicates();

    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("probe ok\n");
    return 0;
}
