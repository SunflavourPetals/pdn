// Tests for pdn/include/pdn/pdn_lexical_error_code.h.
//
// The header only declares an enumeration, so there is no behavior to exercise here. What the
// tests pin down is the part other code and the error message generator rely on:
//   * the complete value list, so that adding a code forces an explicit update of this table;
//   * the numbering 1..N with no gap, because 0 means "no lexical error" (see the header);
//   * the fact that a default-constructed value is 0 and therefore not a real error code.

#include <array>
#include <cstddef>
#include <cstdint>

#include <pdn/pdn_lexical_error_code.h>

namespace
{
	using pdn::lexical_error_code;

	// One entry per enumerator, in declaration order. If the header gains or loses a code,
	// this table has to follow; the static_assert below keeps it honest.
	constexpr std::array all_codes{
		lexical_error_code::not_unicode_scalar_value,
		lexical_error_code::unacceptable_character,
		lexical_error_code::identifier_string_terminated_by_LF,
		lexical_error_code::string_literal_terminated_by_LF,
		lexical_error_code::character_literal_terminated_by_LF,
		lexical_error_code::character_literal_length_is_zero,
		lexical_error_code::character_literal_length_is_greater_than_one,
		lexical_error_code::unterminated_block_comment,
		lexical_error_code::unterminated_nested_block_comment,
		lexical_error_code::identifier_string_missing_terminating_character,
		lexical_error_code::string_missing_terminating_character,
		lexical_error_code::character_missing_terminating_character,
		lexical_error_code::identifier_raw_string_missing_terminating_sequence,
		lexical_error_code::raw_string_missing_terminating_sequence,
		lexical_error_code::invalid_octal_number,
		lexical_error_code::fp_dec_expect_exponent,
		lexical_error_code::fp_hex_expect_exponent,
		lexical_error_code::fp_hex_expect_decimal_part,
		lexical_error_code::bin_prefix_expect_bin_number_sequence,
		lexical_error_code::hex_prefix_expect_hex_number_sequence,
		lexical_error_code::more_than_one_separators_between_numbers,
		lexical_error_code::number_cannot_end_with_separator,
		lexical_error_code::number_from_chars_errc_result_out_of_range,
		lexical_error_code::number_from_chars_errc_invalid_argument,
		lexical_error_code::number_from_chars_errc_other,
		lexical_error_code::number_from_chars_parsing_incomplete,
		lexical_error_code::escape_error_unknown_escape_sequence,
		lexical_error_code::escape_error_o_not_followed_by_left_brackets,
		lexical_error_code::escape_error_o_not_terminated_with_right_brackets,
		lexical_error_code::escape_error_o_empty_delimited_escape_sequence,
		lexical_error_code::escape_error_x_not_terminated_with_right_brackets,
		lexical_error_code::escape_error_x_empty_delimited_escape_sequence,
		lexical_error_code::escape_error_x_used_with_no_following_hex_digits,
		lexical_error_code::escape_error_u_incomplete_universal_character_name,
		lexical_error_code::escape_error_u_not_terminated_with_right_brackets,
		lexical_error_code::escape_error_u_empty_delimited_escape_sequence,
		lexical_error_code::escape_error_U_incomplete_universal_character_name,
		lexical_error_code::escape_error_not_unicode_scalar_value,
		lexical_error_code::escape_error_from_chars_errc_result_out_of_range,
		lexical_error_code::escape_error_from_chars_errc_invalid_argument,
		lexical_error_code::escape_error_from_chars_errc_other,
		lexical_error_code::escape_error_from_chars_parsing_incomplete,
		lexical_error_code::d_seq_error_cannot_find_end_sign,
		lexical_error_code::d_seq_error_invalid_character_in_raw_string_delimiter,
		lexical_error_code::d_seq_error_raw_string_delimiter_longer_than_16_characters,
	};

	static_assert(all_codes.size() == 45, "[test] the enumerator table out of sync with the header");

	constexpr auto code_count = static_cast<int>(all_codes.size());

	// Values must run 1, 2, ..., N in declaration order: no duplicates, no gaps, and 0 unused.
	constexpr bool numbered_from_one()
	{
		for (int i = 0; i < code_count; ++i)
		{
			if (static_cast<int>(all_codes[static_cast<std::size_t>(i)]) != i + 1) { return false; }
		}
		return true;
	}

	static_assert(numbered_from_one(), "[test] lexical_error_code values must be consecutive from 1");

	// Value 0 is the "no lexical error stored yet" state, e.g. pdn_lexer.h builds the code inside
	// a switch; keep it out of the enumeration on purpose.
	static_assert(static_cast<int>(lexical_error_code{}) == 0, "[test] value 0 must stay unused");
}

int main()
{
	// Every code is a distinct, non-zero value; the constexpr checks above already prove the
	// layout, this call keeps a runtime symbol so the test executable is not optimized to nothing.
	int sum = 0;
	for (auto code : all_codes) { sum += static_cast<int>(code); }
	return sum == (code_count * (code_count + 1)) / 2 ? 0 : 1;
}
