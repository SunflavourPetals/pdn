// Review test for pdn_dfa_state_code.h.
// The enum is partitioned into three contiguous regions by sentinel enumerators, and the two
// predicates are just comparisons against those sentinels. The checks below pin the partition
// itself, so adding a state without deciding its region breaks this test.

#include <array>
#include <cstddef>

#include <pdn/pdn_dfa_state_code.h>

namespace
{
	using pdn::dfa_state_code;
	using pdn::is_error_dfa_state;
	using pdn::is_final_dfa_state;

	// The lexer handles these by name, before any region test, so they are neither error nor
	// final. Listed once here to keep the count honest.
	constexpr ::std::array<dfa_state_code, 15> special_states{
		dfa_state_code::unmatched,
		dfa_state_code::unacceptable_character,
		dfa_state_code::identifier_string_with_LF,
		dfa_state_code::identifier_string_escape,
		dfa_state_code::string_with_LF,
		dfa_state_code::string_escape,
		dfa_state_code::character_with_LF,
		dfa_state_code::character_escape,
		dfa_state_code::raw_string_d_seq_opened,
		dfa_state_code::raw_string_received_CR,
		dfa_state_code::raw_string_received_right_parentheses,
		dfa_state_code::identifier_raw_string_d_seq_opened,
		dfa_state_code::identifier_raw_string_received_CR,
		dfa_state_code::identifier_raw_string_received_right_parentheses,
		dfa_state_code::infinity,
	};

	// States the lexer must report as errors: unterminated comments, strings, characters and
	// numbers. The block is contiguous and starts at block_comment.
	constexpr ::std::array<dfa_state_code, 34> error_states{
		dfa_state_code::block_comment,
		dfa_state_code::block_comment_closing,
		dfa_state_code::nested_block_comment,
		dfa_state_code::nested_block_comment_nesting,
		dfa_state_code::nested_block_comment_nested,
		dfa_state_code::nested_block_comment_closing,
		dfa_state_code::nested_block_comment_closed,
		dfa_state_code::identifier_string_opened,
		dfa_state_code::identifier_string,
		dfa_state_code::string_opened,
		dfa_state_code::string,
		dfa_state_code::character_opened,
		dfa_state_code::character,
		dfa_state_code::identifier_raw_string,
		dfa_state_code::raw_string,
		dfa_state_code::dec_seq_with_quote,
		dfa_state_code::fp_dec_part_with_quote,
		dfa_state_code::fp_exp_sign_or_first,
		dfa_state_code::fp_exp_first,
		dfa_state_code::fp_exp_with_quote,
		dfa_state_code::oct_seq_with_quote,
		dfa_state_code::dec_seq_start_with_0,
		dfa_state_code::dec_seq_start_with_0_with_quote,
		dfa_state_code::bin_seq_first,
		dfa_state_code::bin_seq_with_quote,
		dfa_state_code::hex_seq_first,
		dfa_state_code::hex_seq_with_quote,
		dfa_state_code::hex_fp_dec_part_first_after_hex_with_dot,
		dfa_state_code::hex_fp_dec_part,
		dfa_state_code::hex_fp_dec_part_with_quote,
		dfa_state_code::hex_fp_exp_sign_or_first,
		dfa_state_code::hex_fp_exp_first,
		dfa_state_code::hex_fp_exp_with_quote,
		dfa_state_code::hex_fp_seq_start_with_0x_dot,
	};

	// States that can end a token: literals, punctuation, and the two non-token states that
	// still have to return an eof token at end of input.
	constexpr ::std::array<dfa_state_code, 46> final_states{
		dfa_state_code::identifier,
		dfa_state_code::identifier_string_closed,
		dfa_state_code::string_closed,
		dfa_state_code::character_closed,
		dfa_state_code::at_identifier,
		dfa_state_code::raw_string_closed,
		dfa_state_code::identifier_raw_string_closed,
		dfa_state_code::dec_seq,
		dfa_state_code::fp_dec_part_first_after_dec_with_dot,
		dfa_state_code::fp_dec_part,
		dfa_state_code::fp_exp,
		dfa_state_code::zero,
		dfa_state_code::oct_seq,
		dfa_state_code::bin_seq,
		dfa_state_code::hex_seq,
		dfa_state_code::hex_fp_exp,
		dfa_state_code::start,
		dfa_state_code::line_comment,
		dfa_state_code::at_sign,
		dfa_state_code::tilde,
		dfa_state_code::exclamation_mark,
		dfa_state_code::hash,
		dfa_state_code::dollar,
		dfa_state_code::percent,
		dfa_state_code::caret,
		dfa_state_code::ampersand,
		dfa_state_code::asterisk,
		dfa_state_code::left_parentheses,
		dfa_state_code::right_parentheses,
		dfa_state_code::left_brackets,
		dfa_state_code::right_brackets,
		dfa_state_code::left_curly_brackets,
		dfa_state_code::right_curly_brackets,
		dfa_state_code::minus,
		dfa_state_code::plus,
		dfa_state_code::equal,
		dfa_state_code::slash,
		dfa_state_code::back_slash,
		dfa_state_code::bar,
		dfa_state_code::colon,
		dfa_state_code::semicolon,
		dfa_state_code::less_than,
		dfa_state_code::greater_than,
		dfa_state_code::comma,
		dfa_state_code::dot,
		dfa_state_code::question_mark,
	};

	// Region layout: 15 special states, then the error block, then the final block. The three
	// flags normalize to the same values as the first member of each block, so the value of
	// flag_error_state_begin is exactly the number of special states, and the distance between
	// the two flags is exactly the size of the error block.
	static_assert(static_cast<int>(dfa_state_code::flag_error_state_begin) == special_states.size());
	static_assert(static_cast<int>(dfa_state_code::flag_error_state_end) -
	              static_cast<int>(dfa_state_code::flag_error_state_begin) == error_states.size() - 1);
	static_assert(static_cast<int>(dfa_state_code::question_mark) -
	              static_cast<int>(dfa_state_code::flag_final_state_begin) == final_states.size() - 1);

	// The special block must stay before the error block, otherwise is_error/is_final would
	// misclassify it. flag_special_state_end names the last special state; comparing the flag
	// against the block size also catches a special state appended after it without moving the flag.
	static_assert(static_cast<int>(dfa_state_code::flag_special_state_end) == special_states.size() - 1);
	static_assert(dfa_state_code::flag_special_state_end < dfa_state_code::flag_error_state_begin);

	// Every special state is neither error nor final.
	constexpr bool special_states_are_unclassified()
	{
		for (auto c : special_states)
		{
			if (is_error_dfa_state(c) || is_final_dfa_state(c)) return false;
		}
		return true;
	}
	static_assert(special_states_are_unclassified());

	// Every error state is an error state, and never a final one.
	constexpr bool error_states_are_errors()
	{
		for (auto c : error_states)
		{
			if (!is_error_dfa_state(c) || is_final_dfa_state(c)) return false;
		}
		return true;
	}
	static_assert(error_states_are_errors());

	// Every final state is final, and never an error one.
	constexpr bool final_states_are_final()
	{
		for (auto c : final_states)
		{
			if (!is_final_dfa_state(c) || is_error_dfa_state(c)) return false;
		}
		return true;
	}
	static_assert(final_states_are_final());

	// start and line_comment are final on purpose: at end of input the lexer must still be able
	// to hand back an eof token instead of tripping its "unhandled non-final state" assertion.
	static_assert(is_final_dfa_state(dfa_state_code::start));
	static_assert(is_final_dfa_state(dfa_state_code::line_comment));

	// The sentinel enumerators themselves classify as their own region's first member.
	static_assert(is_error_dfa_state(dfa_state_code::flag_error_state_begin));
	static_assert(is_error_dfa_state(dfa_state_code::flag_error_state_end));
	static_assert(is_final_dfa_state(dfa_state_code::flag_final_state_begin));
	static_assert(!is_error_dfa_state(dfa_state_code::flag_final_state_begin));
}

int main()
{
	return 0;
}
