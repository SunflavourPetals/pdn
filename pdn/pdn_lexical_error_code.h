#ifndef PDN_Header_pdn_lexical_error_code
#define PDN_Header_pdn_lexical_error_code

namespace pdn
{
	enum class lexical_error_code
	{
		success,

		not_unicode_scalar_value,

		unacceptable_character,

		at_value_not_found,

		// string/character terminated by LF
		identifier_string_terminated_by_LF,
		string_literal_terminated_by_LF,
		character_literal_terminated_by_LF,

		// character literal length not 1
		character_literal_length_is_zero,
		character_literal_length_is_greater_than_one,

		// comment not closed
		unterminated_block_comment,
		unterminated_nested_block_comment,

		// missing terminating character/sequence
		identifier_string_missing_terminating_character,
		string_missing_terminating_character,
		character_missing_terminating_character,
		identifier_raw_string_missing_terminating_sequence,
		raw_string_missing_terminating_sequence,

		// number sequence error
		invalid_octal_number,
		fp_dec_expect_exponent,
		fp_hex_expect_exponent,
		fp_hex_expect_decimal_part,
		bin_prefix_expect_bin_number_sequence,
		hex_prefix_expect_hex_number_sequence,
		more_than_one_separators_may_between_numbers,
		number_separator_cannot_be_appear_here,

		// number from chars error
		number_from_chars_errc_result_out_of_range,
		number_from_chars_errc_invalid_argument,
		number_from_chars_errc_other,
		number_from_chars_parsing_incomplete,

		// escape error
		escape_error_unknown_escape_sequence,               // \@ 
		escape_error_o_not_followed_by_left_brackets,       // \o@
		escape_error_o_not_terminated_with_right_brackets,  // \o{n...@ (missing '}')
		escape_error_o_empty_delimited_escape_sequence,     // \o{} 
		escape_error_x_not_terminated_with_right_brackets,  // \x{n...@
		escape_error_x_empty_delimited_escape_sequence,     // \x{}
		escape_error_x_used_with_no_following_hex_digits,   // \x@
		escape_error_u_incomplete_universal_character_name, // \u@ \un@ \unn@ \unnn@
		escape_error_u_not_terminated_with_right_brackets,  // \u{n...@
		escape_error_u_empty_delimited_escape_sequence,     // \u{}
		escape_error_U_incomplete_universal_character_name, // \U@ \Un@ \Unn@ \Unnn@ \Unnnn@ \Unnnnn@ \Unnnnnn@ \Unnnnnnn@

		// escape from_chars  error
		escape_error_from_chars_errc_result_out_of_range,
		escape_error_from_chars_errc_invalid_argument,
		escape_error_from_chars_errc_other,
		escape_error_from_chars_parsing_incomplete,
		escape_error_not_unicode_scalar_value,

		// raw string/identifier raw string error
		d_seq_error_cannot_find_end_sign,
		d_seq_error_invalid_character_in_raw_string_delimiter,      // @"d_seq...@
		d_seq_error_raw_string_delimiter_longer_than_16_characters, // A"aaaabbbbccccdddd@...()..."

		// inner error
		inner_error_binary_integer_without_0b_prefix,
		inner_error_hexadecimal_integer_without_0x_prefix,
		inner_error_hexadecimal_floating_point_without_0x_prefix,
		inner_error_make_non_final_dfa_state_to_token,
	};
}

#endif
