#ifndef PDN_Header_pdn_dfa_state_code
#define PDN_Header_pdn_dfa_state_code

namespace pdn
{
	enum class dfa_state_code
	{
		// special state process by lexer vvv

		unmatched,

		unacceptable_character,

		identifier_string_with_LF,
		identifier_string_escape,

		string_with_LF,
		string_escape,

		character_with_LF,
		character_escape,

		raw_string_d_seq_opened,
		raw_string_received_CR,
		raw_string_received_right_parentheses,

		identifier_raw_string_d_seq_opened,
		identifier_raw_string_received_CR,
		identifier_raw_string_received_right_parentheses,

		infinity,

		// special state process by lexer ^^^



		// ---------------- ERROR ----------------

		FLAG_ERRER_STATE_BEGIN, // vvv

		// block comment not closed
		block_comment = FLAG_ERRER_STATE_BEGIN,
		block_comment_closing,

		// nested block comment not closed
		nested_block_comment,
		nested_block_comment_nesting,
		nested_block_comment_nested,
		nested_block_comment_closing,
		nested_block_comment_closed, // when layer = 0, lexer make it become start state

		// identifier string not closed
		identifier_string_opened,
		identifier_string,

		// string not closed
		string_opened,
		string,

		// character not closed
		character_opened,
		character,

		// identifier_raw string not closed
		identifier_raw_string,

		// raw string not closed
		raw_string,

		// numbers
		// number separator not allowed here
		dec_seq_with_quote,
		fp_dec_part_with_quote,
		// expect exp part
		fp_exp_sign_or_first,
		fp_exp_first,
		// number separator not allowed here
		fp_exp_with_quote,
		oct_seq_with_quote,
		// invalid oct number
		dec_seq_start_with_0,
		// number separator not allowed here
		dec_seq_start_with_0_with_quote,
		// expect binary number sequence
		bin_seq_first,
		// number separator not allowed here
		bin_seq_with_quote,
		// expect hexadecimal number sequence
		hex_seq_first,
		// number separator not allowed here
		hex_seq_with_quote,
		// expect exp part
		hex_fp_dec_part_first_after_hex_with_dot,
		hex_fp_dec_part,
		// number separator not allowed here and expect exp part
		hex_fp_dec_part_with_quote,
		// expect exp part
		hex_fp_exp_sign_or_first,
		hex_fp_exp_first,
		// number separator not allowed here
		hex_fp_exp_with_quote,
		// in this case, hexadecimal decimals are necessary "0x.[not hex]", and expect exp part
		hex_fp_seq_start_with_0x_dot,

		FLAG_ERRER_STATE_END = hex_fp_seq_start_with_0x_dot, // ^^^



		// ---------------- FINAL ----------------

		FLAG_FINAL_STATE_BEGIN, // vvv

		identifier = FLAG_FINAL_STATE_BEGIN,
		identifier_string_closed,
		string_closed,
		character_closed,
		at_identifier,
		raw_string_closed,
		identifier_raw_string_closed,

		dec_seq,
		fp_dec_part_first_after_dec_with_dot,
		fp_dec_part,
		fp_exp,
		zero,
		oct_seq,
		bin_seq,
		hex_seq,
		hex_fp_exp,

		// EOF
		start,
		line_comment, // token_code = eof
		// other
		at_sign,              // @ 
		tilde,                // ~ 
		exclamation_mark,     // ! 
		hash,                 // # 
		dollar,               // $ 
		percent,              // % 
		caret,                // ^ 
		ampersand,            // & 
		asterisk,             // * 
		left_parentheses,     // ( 
		right_parentheses,    // ) 
		left_brackets,        // [ 
		right_brackets,       // ] 
		left_curly_brackets,  // { 
		right_curly_brackets, // } 
		minus,                // - 
		plus,                 // + 
		equal,                // = 
		slash,                // (/) 
		back_slash,           // (\) 
		bar,                  // | 
		colon,                // : 
		semicolon,            // ; 
		less_than,            // < 
		greater_than,         // > 
		comma,                // , 
		dot,                  // . 
		question_mark,        // ? 
	};
	static_assert(dfa_state_code::FLAG_ERRER_STATE_END <= dfa_state_code::FLAG_FINAL_STATE_BEGIN,
	              "[pdn] dfa_state_code error: FLAG_ERRER_STATE_END <= FLAG_FINAL_STATE_BEGIN is false");

	constexpr bool is_final_dfa_state(dfa_state_code state) noexcept
	{
		return state >= dfa_state_code::FLAG_FINAL_STATE_BEGIN;
	}
	constexpr bool is_error_dfa_state(dfa_state_code state) noexcept
	{
		return state >= dfa_state_code::FLAG_ERRER_STATE_BEGIN && state <= dfa_state_code::FLAG_ERRER_STATE_END;
	}
}

#endif
