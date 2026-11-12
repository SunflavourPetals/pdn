#ifndef PDN_Header_pdn_lexical_error_code
#define PDN_Header_pdn_lexical_error_code

namespace pdn
{
	enum class lexical_error_code
	{
		not_unicode_scalar_value = 1, // extra(not_unicode_scalar_value)

		unacceptable_character, // extra(error_string)

		// string/character terminated by LF
		identifier_string_terminated_by_LF, // extra(error_string) identifier
		string_literal_terminated_by_LF,    // extra(error_string) string
		character_literal_terminated_by_LF, // extra(error_string) character

		// character literal length not 1
		character_literal_length_is_zero, // extra(character_length_error)
		character_literal_length_is_greater_than_one, // extra(character_length_error)

		// comment not closed
		unterminated_block_comment,
		unterminated_nested_block_comment,

		// missing terminating character/sequence
		identifier_string_missing_terminating_character,    // extra(error_string) identifier
		string_missing_terminating_character,               // extra(error_string) string
		character_missing_terminating_character,            // extra(error_string) character
		identifier_raw_string_missing_terminating_sequence, // extra(missing_terminating_sequence) identifier and d_seq
		raw_string_missing_terminating_sequence,            // extra(missing_terminating_sequence) string and d_seq

		// number sequence error
		invalid_octal_number,       // extra(error_string) number string
		fp_dec_expect_exponent,     // extra(error_string) number string
		fp_hex_expect_exponent,     // extra(error_string) number string
		fp_hex_expect_decimal_part, // extra(error_string) number string
		bin_prefix_expect_bin_number_sequence,    // extra(error_string) number string
		hex_prefix_expect_hex_number_sequence,    // extra(error_string) number string
		more_than_one_separators_between_numbers, // extra(error_string) number string
		number_cannot_end_with_separator,         // extra(number_end_with_separator)

		// number from chars error // [[unlikely]]
		number_from_chars_errc_result_out_of_range, // extra(from_chars_error)
		number_from_chars_errc_invalid_argument,    // extra(from_chars_error)
		number_from_chars_errc_other,               // extra(from_chars_error)
		number_from_chars_parsing_incomplete,       // extra(from_chars_error)

		// escape error
		// extra(error_string) escape sequence except \, e.g. "\u{}" -> "u{}" vvv
		escape_error_unknown_escape_sequence,               // \@ (@ is illegal character in this escape sequence)
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
		escape_error_not_unicode_scalar_value, // extra(escape_not_unicode_scalar_value)

		// escape from_chars error // [[unlikely]]
		escape_error_from_chars_errc_result_out_of_range, // extra(from_chars_error)
		escape_error_from_chars_errc_invalid_argument,    // extra(from_chars_error)
		escape_error_from_chars_errc_other,               // extra(from_chars_error)
		escape_error_from_chars_parsing_incomplete,       // extra(from_chars_error)

		// raw string/identifier raw string error
		// extra(delimiter_error) delimiter sequence
		d_seq_error_cannot_find_end_sign, // @"... EOF
		// extra(delimiter_error) delimiter sequence and last is invalid character 
		d_seq_error_invalid_character_in_raw_string_delimiter, // @"d_seq...@
		// extra(delimiter_error) delimiter sequence
		d_seq_error_raw_string_delimiter_longer_than_16_characters, // A"aaaabbbbccccdddd@...()..."
	};
}

#endif
