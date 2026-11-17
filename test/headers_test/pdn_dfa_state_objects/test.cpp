#include <pdn/pdn_dfa_state_objects.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <string_view>

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

    using dsc = pdn::dfa_state_code;
    using dso = pdn::dfa_state_object;
    namespace objs = pdn::dfa_state_objects;

    struct row
    {
        const char* name;
        const dso* state;
        dsc code;
    };

    constexpr row table[] = {
        { "unmatched", &objs::unmatched, dsc::unmatched },
        { "unacceptable_character", &objs::unacceptable_character, dsc::unacceptable_character },
        { "start", &objs::start, dsc::start },
        { "identifier", &objs::identifier, dsc::identifier },
        { "identifier_string_opened", &objs::identifier_string_opened, dsc::identifier_string_opened },
        { "identifier_string", &objs::identifier_string, dsc::identifier_string },
        { "identifier_string_closed", &objs::identifier_string_closed, dsc::identifier_string_closed },
        { "identifier_string_escape", &objs::identifier_string_escape, dsc::identifier_string_escape },
        { "identifier_string_with_LF", &objs::identifier_string_with_LF, dsc::identifier_string_with_LF },
        { "slash", &objs::slash, dsc::slash },
        { "line_comment", &objs::line_comment, dsc::line_comment },
        { "block_comment", &objs::block_comment, dsc::block_comment },
        { "block_comment_closing", &objs::block_comment_closing, dsc::block_comment_closing },
        { "less_than", &objs::less_than, dsc::less_than },
        { "nested_block_comment", &objs::nested_block_comment, dsc::nested_block_comment },
        { "nested_block_comment_nesting", &objs::nested_block_comment_nesting, dsc::nested_block_comment_nesting },
        { "nested_block_comment_nested", &objs::nested_block_comment_nested, dsc::nested_block_comment_nested },
        { "nested_block_comment_closing", &objs::nested_block_comment_closing, dsc::nested_block_comment_closing },
        { "nested_block_comment_closed", &objs::nested_block_comment_closed, dsc::nested_block_comment_closed },
        { "string_opened", &objs::string_opened, dsc::string_opened },
        { "string", &objs::string, dsc::string },
        { "string_closed", &objs::string_closed, dsc::string_closed },
        { "string_escape", &objs::string_escape, dsc::string_escape },
        { "string_with_LF", &objs::string_with_LF, dsc::string_with_LF },
        { "character_opened", &objs::character_opened, dsc::character_opened },
        { "character", &objs::character, dsc::character },
        { "character_closed", &objs::character_closed, dsc::character_closed },
        { "character_escape", &objs::character_escape, dsc::character_escape },
        { "character_with_LF", &objs::character_with_LF, dsc::character_with_LF },
        { "at_sign", &objs::at_sign, dsc::at_sign },
        { "at_identifier", &objs::at_identifier, dsc::at_identifier },
        { "raw_string_d_seq_opened", &objs::raw_string_d_seq_opened, dsc::raw_string_d_seq_opened },
        { "raw_string", &objs::raw_string, dsc::raw_string },
        { "raw_string_received_CR", &objs::raw_string_received_CR, dsc::raw_string_received_CR },
        { "raw_string_received_right_parentheses", &objs::raw_string_received_right_parentheses, dsc::raw_string_received_right_parentheses },
        { "raw_string_closed", &objs::raw_string_closed, dsc::raw_string_closed },
        { "identifier_raw_string_d_seq_opened", &objs::identifier_raw_string_d_seq_opened, dsc::identifier_raw_string_d_seq_opened },
        { "identifier_raw_string", &objs::identifier_raw_string, dsc::identifier_raw_string },
        { "identifier_raw_string_received_CR", &objs::identifier_raw_string_received_CR, dsc::identifier_raw_string_received_CR },
        { "identifier_raw_string_received_right_parentheses", &objs::identifier_raw_string_received_right_parentheses, dsc::identifier_raw_string_received_right_parentheses },
        { "identifier_raw_string_closed", &objs::identifier_raw_string_closed, dsc::identifier_raw_string_closed },
        { "dec_seq", &objs::dec_seq, dsc::dec_seq },
        { "dec_seq_with_quote", &objs::dec_seq_with_quote, dsc::dec_seq_with_quote },
        { "fp_dec_part_first_after_dec_with_dot", &objs::fp_dec_part_first_after_dec_with_dot, dsc::fp_dec_part_first_after_dec_with_dot },
        { "fp_dec_part", &objs::fp_dec_part, dsc::fp_dec_part },
        { "fp_dec_part_with_quote", &objs::fp_dec_part_with_quote, dsc::fp_dec_part_with_quote },
        { "fp_exp_sign_or_first", &objs::fp_exp_sign_or_first, dsc::fp_exp_sign_or_first },
        { "fp_exp_first", &objs::fp_exp_first, dsc::fp_exp_first },
        { "fp_exp", &objs::fp_exp, dsc::fp_exp },
        { "fp_exp_with_quote", &objs::fp_exp_with_quote, dsc::fp_exp_with_quote },
        { "zero", &objs::zero, dsc::zero },
        { "oct_seq", &objs::oct_seq, dsc::oct_seq },
        { "oct_seq_with_quote", &objs::oct_seq_with_quote, dsc::oct_seq_with_quote },
        { "dec_seq_start_with_0", &objs::dec_seq_start_with_0, dsc::dec_seq_start_with_0 },
        { "dec_seq_start_with_0_with_quote", &objs::dec_seq_start_with_0_with_quote, dsc::dec_seq_start_with_0_with_quote },
        { "bin_seq_first", &objs::bin_seq_first, dsc::bin_seq_first },
        { "bin_seq", &objs::bin_seq, dsc::bin_seq },
        { "bin_seq_with_quote", &objs::bin_seq_with_quote, dsc::bin_seq_with_quote },
        { "hex_seq_first", &objs::hex_seq_first, dsc::hex_seq_first },
        { "hex_seq", &objs::hex_seq, dsc::hex_seq },
        { "hex_seq_with_quote", &objs::hex_seq_with_quote, dsc::hex_seq_with_quote },
        { "hex_fp_dec_part_first_after_hex_with_dot", &objs::hex_fp_dec_part_first_after_hex_with_dot, dsc::hex_fp_dec_part_first_after_hex_with_dot },
        { "hex_fp_dec_part", &objs::hex_fp_dec_part, dsc::hex_fp_dec_part },
        { "hex_fp_dec_part_with_quote", &objs::hex_fp_dec_part_with_quote, dsc::hex_fp_dec_part_with_quote },
        { "hex_fp_exp_sign_or_first", &objs::hex_fp_exp_sign_or_first, dsc::hex_fp_exp_sign_or_first },
        { "hex_fp_exp_first", &objs::hex_fp_exp_first, dsc::hex_fp_exp_first },
        { "hex_fp_exp", &objs::hex_fp_exp, dsc::hex_fp_exp },
        { "hex_fp_exp_with_quote", &objs::hex_fp_exp_with_quote, dsc::hex_fp_exp_with_quote },
        { "hex_fp_seq_start_with_0x_dot", &objs::hex_fp_seq_start_with_0x_dot, dsc::hex_fp_seq_start_with_0x_dot },
        { "dot", &objs::dot, dsc::dot },
        { "tilde", &objs::tilde, dsc::tilde },
        { "exclamation_mark", &objs::exclamation_mark, dsc::exclamation_mark },
        { "hash", &objs::hash, dsc::hash },
        { "dollar", &objs::dollar, dsc::dollar },
        { "percent", &objs::percent, dsc::percent },
        { "caret", &objs::caret, dsc::caret },
        { "ampersand", &objs::ampersand, dsc::ampersand },
        { "asterisk", &objs::asterisk, dsc::asterisk },
        { "left_parentheses", &objs::left_parentheses, dsc::left_parentheses },
        { "right_parentheses", &objs::right_parentheses, dsc::right_parentheses },
        { "left_brackets", &objs::left_brackets, dsc::left_brackets },
        { "right_brackets", &objs::right_brackets, dsc::right_brackets },
        { "left_curly_brackets", &objs::left_curly_brackets, dsc::left_curly_brackets },
        { "right_curly_brackets", &objs::right_curly_brackets, dsc::right_curly_brackets },
        { "minus", &objs::minus, dsc::minus },
        { "plus", &objs::plus, dsc::plus },
        { "equal", &objs::equal, dsc::equal },
        { "back_slash", &objs::back_slash, dsc::back_slash },
        { "bar", &objs::bar, dsc::bar },
        { "colon", &objs::colon, dsc::colon },
        { "semicolon", &objs::semicolon, dsc::semicolon },
        { "greater_than", &objs::greater_than, dsc::greater_than },
        { "comma", &objs::comma, dsc::comma },
        { "question_mark", &objs::question_mark, dsc::question_mark },
        { "infinity", &objs::infinity, dsc::infinity },
    };

    constexpr std::size_t object_count = 95;

    constexpr bool in_error_range(dsc code) noexcept
    {
        return code >= dsc::flag_error_state_begin && code <= dsc::flag_error_state_end;
    }

    constexpr bool in_final_range(dsc code) noexcept
    {
        return code >= dsc::flag_final_state_begin;
    }

    // The states whose transformer is nullptr: the lexer consumes them by state code.
    constexpr const char* null_transformer_states[] = {
        "unmatched",
        "unacceptable_character",
        "identifier_string_escape",
        "identifier_string_with_LF",
        "string_escape",
        "string_with_LF",
        "character_escape",
        "character_with_LF",
        "raw_string_d_seq_opened",
        "raw_string_received_CR",
        "raw_string_received_right_parentheses",
        "identifier_raw_string_d_seq_opened",
        "identifier_raw_string_received_CR",
        "identifier_raw_string_received_right_parentheses",
    };

    constexpr std::size_t null_transformer_count = 14;

    bool is_null_transformer_state(const char* name)
    {
        for (const char* candidate : null_transformer_states)
        {
            if (std::string_view{ candidate } == name)
            {
                return true;
            }
        }
        return false;
    }
}

int main()
{
    static_assert(std::size(table) == object_count);
    static_assert(std::size(null_transformer_states) == null_transformer_count);

    // One object per state, and the object's declared code is the state it names.
    for (const row& entry : table)
    {
        check(entry.state->state_code == entry.code, entry.name);
    }

    // The three ranges are decided by the state code alone.
    std::size_t special_count = 0;
    std::size_t error_count = 0;
    std::size_t final_count = 0;
    for (const row& entry : table)
    {
        check(entry.state->is_error() == in_error_range(entry.code), entry.name);
        check(entry.state->is_final() == in_final_range(entry.code), entry.name);
        if (in_error_range(entry.code))
        {
            ++error_count;
        }
        else if (in_final_range(entry.code))
        {
            ++final_count;
        }
        else
        {
            ++special_count;
        }
    }
    check(special_count == 15, "15 special states");
    check(error_count == 34, "34 error states");
    check(final_count == 46, "46 final states");

    // Every state code appears exactly once.
    for (std::size_t i = 0; i != object_count; ++i)
    {
        for (std::size_t j = i + 1; j != object_count; ++j)
        {
            check(table[i].code != table[j].code, table[i].name);
        }
    }

    // Exactly the documented states have no transformer, and each of them is listed.
    std::size_t null_count = 0;
    for (const row& entry : table)
    {
        const bool has_null_transformer = entry.state->transformer == nullptr;
        check(has_null_transformer == is_null_transformer_state(entry.name), entry.name);
        if (has_null_transformer)
        {
            ++null_count;
        }
    }
    check(null_count == null_transformer_count, "null transformer states");

    // Entry points.
    check(objs::start_state().state_code == dsc::start, "start_state is start");
    check(objs::start_state().transformer != nullptr, "start has a transformer");
    check(objs::start.token_code == pdn::pdn_token_code::eof, "start produces eof");
    check(objs::to_unmatched(U'a').state_code == dsc::unmatched, "to_unmatched");
    check(objs::to_unmatched(U'\u221e').state_code == dsc::unmatched, "to_unmatched any code point");

    // Transitions out of start.
    check(objs::start.transformer(U'"').state_code == dsc::string_opened, "start '\"'");
    check(objs::start.transformer(U'\'').state_code == dsc::character_opened, "start '\\''");
    check(objs::start.transformer(U'`').state_code == dsc::identifier_string_opened, "start '`'");
    check(objs::start.transformer(U'@').state_code == dsc::at_sign, "start '@'");
    check(objs::start.transformer(U'/').state_code == dsc::slash, "start '/'");
    check(objs::start.transformer(U'.').state_code == dsc::dot, "start '.'");
    check(objs::start.transformer(U'0').state_code == dsc::zero, "start '0'");
    check(objs::start.transformer(U'1').state_code == dsc::dec_seq, "start '1'");
    check(objs::start.transformer(U'9').state_code == dsc::dec_seq, "start '9'");
#ifndef PDN_NO_EXTENSIONS
    check(objs::start.transformer(U'\u221e').state_code == dsc::infinity, "start infinity");
#else
    check(objs::start.transformer(U'\u221e').state_code == dsc::unacceptable_character,
          "start infinity is an extension");
#endif
    check(objs::start.transformer(U' ').state_code == dsc::start, "start space loops");
    check(objs::start.transformer(U'\n').state_code == dsc::start, "start newline loops");
    check(objs::start.transformer(U'a').state_code == dsc::identifier, "start 'a'");
    check(objs::start.transformer(U'_').state_code == dsc::identifier, "start '_'");
    check(objs::start.transformer(U'?').state_code == dsc::question_mark, "start '?'");
    check(objs::start.transformer(U'\u0001').state_code == dsc::unacceptable_character, "start control char");

    // Deeper transitions.
    check(objs::slash.transformer(U'/').state_code == dsc::line_comment, "slash '/'");
    check(objs::slash.transformer(U'*').state_code == dsc::block_comment, "slash '*'");
    check(objs::slash.transformer(U'a').state_code == dsc::unmatched, "slash 'a'");
    check(objs::at_sign.transformer(U'`').state_code == dsc::identifier_raw_string_d_seq_opened, "at_sign '`'");
    check(objs::at_sign.transformer(U'"').state_code == dsc::raw_string_d_seq_opened, "at_sign '\"'");
    check(objs::at_sign.transformer(U'a').state_code == dsc::at_identifier, "at_sign 'a'");
    check(objs::at_sign.transformer(U'0').state_code == dsc::unmatched, "at_sign '0'");
    check(objs::identifier.transformer(U'a').state_code == dsc::identifier, "identifier loops");
    check(objs::identifier.transformer(U'(').state_code == dsc::unmatched, "identifier '(' stops");

    // The zero state starts every integer prefix.
    check(objs::zero.transformer(U'7').state_code == dsc::oct_seq, "zero '7' -> oct");
    check(objs::zero.transformer(U'9').state_code == dsc::dec_seq_start_with_0, "zero '9' -> dec");
    check(objs::zero.transformer(U'\'').state_code == dsc::oct_seq_with_quote, "zero quote");
    check(objs::zero.transformer(U'x').state_code == dsc::hex_seq_first, "zero 'x' -> hex");
    check(objs::zero.transformer(U'b').state_code == dsc::bin_seq_first, "zero 'b' -> bin");
    check(objs::zero.transformer(U'.').state_code == dsc::fp_dec_part_first_after_dec_with_dot, "zero '.'");
    check(objs::zero.transformer(U'e').state_code == dsc::fp_exp_sign_or_first, "zero 'e'");

    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("all dfa state object checks passed\n");
    return 0;
}