#ifndef PDN_Header_pdn_dfa_state_objects
#define PDN_Header_pdn_dfa_state_objects

#include "pdn_unicode_base.h"
#include "pdn_lexer_utility.h"
#include "pdn_dfa_state_code.h"
#include "pdn_token_code.h"
#include "pdn_dfa_state_object.h"

namespace pdn::dfa_state_objects
{
	using unicode::code_point_t;
	using cdfaso = const dfa_state_object; // const dfa state object

	// transformer member of following states are nullptr
	//     state unmatched: lexer is required to stop dfa
	//     state other:     process and transform to other state by lexer
	// list:
	//     unmatched
	//     unacceptable_character
	//     identifier_string_escape
	//     identifier_string_with_LF
	//     string_escape
	//     string_with_LF
	//     character_escape
	//     character_with_LF
	//     raw_string_d_seq_opened
	//     raw_string_received_CR
	//     raw_string_received_right_parentheses
	//     identifier_raw_string_d_seq_opened
	//     identifier_raw_string_received_CR
	//     identifier_raw_string_received_right_parentheses

	inline extern cdfaso
		start;

	inline extern cdfaso
		unmatched;

	inline extern cdfaso
		unacceptable_character;

	inline extern cdfaso
		identifier;

	inline extern cdfaso
		identifier_string_opened,
		identifier_string_closed,
		identifier_string_escape,
		identifier_string,
		identifier_string_with_LF;
		
	inline extern cdfaso
		string_opened,
		string_closed,
		string_escape,
		string,
		string_with_LF;

	inline extern cdfaso
		character_opened,
		character_closed,
		character_escape,
		character,
		character_with_LF;
		
	inline extern cdfaso
		at_sign,
		at_identifier;

	inline extern cdfaso
		raw_string_d_seq_opened,
		raw_string_closed,
		raw_string,
		raw_string_received_CR,
		raw_string_received_right_parentheses;

	inline extern cdfaso
		identifier_raw_string_d_seq_opened,
		identifier_raw_string_closed,
		identifier_raw_string,
		identifier_raw_string_received_CR,
		identifier_raw_string_received_right_parentheses;

	inline extern cdfaso
		dec_seq_with_quote,
		fp_dec_part_with_quote,
		fp_exp_sign_or_first,
		fp_exp_first,
		fp_exp_with_quote,
		oct_seq_with_quote,
		dec_seq_start_with_0, // invalid oct number
		dec_seq_start_with_0_with_quote,
		bin_seq_first,
		bin_seq_with_quote,
		hex_seq_first,
		hex_seq_with_quote,
		hex_fp_dec_part_first_after_hex_with_dot,
		hex_fp_dec_part,
		hex_fp_dec_part_with_quote,
		hex_fp_exp_sign_or_first,
		hex_fp_exp_first,
		hex_fp_exp_with_quote,
		hex_fp_seq_start_with_0x_dot;

	inline extern cdfaso
		dec_seq,
		fp_dec_part_first_after_dec_with_dot,
		fp_dec_part,
		fp_exp,
		zero,
		oct_seq,
		bin_seq,
		hex_seq,
		hex_fp_exp;

	inline extern cdfaso
		slash,
		line_comment,
		block_comment,
		block_comment_closing;
			
	inline extern cdfaso
		less_than,
		nested_block_comment,
		nested_block_comment_nesting,
		nested_block_comment_nested,
		nested_block_comment_closing,
		nested_block_comment_closed;

	inline extern cdfaso
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
		question_mark;        // ? 

	inline extern cdfaso
		infinity;

	inline dfa_state_object to_unmatched(code_point_t) noexcept { return unmatched; }
	inline dfa_state_object start_state ()             noexcept { return start; }

	inline cdfaso unmatched             { dfa_state_code::unmatched,              pdn_token_code::invalid };
	inline cdfaso unacceptable_character{ dfa_state_code::unacceptable_character, pdn_token_code::invalid };

	inline cdfaso start{
		dfa_state_code::start,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\"': return string_opened;
			case U'\'': return character_opened;
			case U'`': return identifier_string_opened;
			case U'@': return at_sign;
			case U'~': return tilde;
			case U'!': return exclamation_mark;
			case U'#': return hash;
			case U'$': return dollar;
			case U'%': return percent;
			case U'^': return caret;
			case U'&': return ampersand;
			case U'*': return asterisk;
			case U'(': return left_parentheses;
			case U')': return right_parentheses;
			case U'[': return left_brackets;
			case U']': return right_brackets;
			case U'{': return left_curly_brackets;
			case U'}': return right_curly_brackets;
			case U'-': return minus;
			case U'+': return plus;
			case U'=': return equal;
			case U'/': return slash;
			case U'\\': return back_slash;
			case U'|': return bar;
			case U':': return colon;
			case U';': return semicolon;
			case U'<': return less_than;
			case U'>': return greater_than;
			case U',': return comma;
			case U'.': return dot;
			case U'?': return question_mark;
			case U'0': return zero;
			case U'\u221e': return infinity;
			default:
				using namespace lexer_utility;
				if (in_range(c, U'1', U'9'))
					return dec_seq;
				if (is_token_separator(c))
					return start;
				if (is_allowed_as_first_char_of_identifier(c))
					return identifier;
				break;
			}
			return unacceptable_character;
		}
	};

	inline cdfaso identifier{
		dfa_state_code::identifier,
		pdn_token_code::identifier,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_allowed_in_identifier(c))
				return identifier;
			return unmatched;
		}
	};
		
	inline cdfaso identifier_string_opened{
		dfa_state_code::identifier_string_opened,
		pdn_token_code::identifier,
		[](code_point_t c) noexcept
		{
			return identifier_string.transformer(c);
		}
	};
	inline cdfaso identifier_string{
		dfa_state_code::identifier_string,
		pdn_token_code::identifier,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\\':
				return identifier_string_escape;
			case U'\n':
				return identifier_string_with_LF;
			case U'`':
				return identifier_string_closed;
			default:
				if (lexer_utility::is_in_translation_character_set(c))
				{
					return identifier_string;
				}
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso identifier_string_closed{
		dfa_state_code::identifier_string_closed,
		pdn_token_code::identifier,
		to_unmatched
	};
	inline cdfaso identifier_string_escape{
		dfa_state_code::identifier_string_escape,
		pdn_token_code::identifier // transformer is nullptr
	};
	inline cdfaso identifier_string_with_LF{
		dfa_state_code::identifier_string_with_LF,
		pdn_token_code::identifier
	};

	inline cdfaso slash{
		dfa_state_code::slash,
		pdn_token_code::slash,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'/': // line comment
				return line_comment;
			case U'*': // block_comment
				return block_comment;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso line_comment{
		dfa_state_code::line_comment,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_new_line(c))
				return start;
			return line_comment;
		}
	};
	inline cdfaso block_comment{
		dfa_state_code::block_comment,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			if (c == U'*')
				return block_comment_closing;
			return block_comment;
		}
	};
	inline cdfaso block_comment_closing{
		dfa_state_code::block_comment_closing,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'/':
				return start;
			case U'*':
				return block_comment_closing;
			default:
				break;
			}
			return block_comment;
		}
	};

	inline cdfaso less_than{
		dfa_state_code::less_than,
		pdn_token_code::less_than,
		[](code_point_t c) noexcept
		{
			if (c == U'/')
				return nested_block_comment_nested;
			return unmatched;
		}
	};
	inline cdfaso nested_block_comment{
		dfa_state_code::nested_block_comment,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'<':
				return nested_block_comment_nesting;
			case U'/':
				return nested_block_comment_closing;
			default:
				break;
			}
			return nested_block_comment;
		}
	};
	inline cdfaso nested_block_comment_nesting{
		dfa_state_code::nested_block_comment_nesting,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'<':
				return nested_block_comment_nesting;
			case U'/':
				return nested_block_comment_nested;
			default:
				break;
			}
			return nested_block_comment;
		}
	};
	inline cdfaso nested_block_comment_nested{
		dfa_state_code::nested_block_comment_nested,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			return nested_block_comment.transformer(c);
		}
	};
	inline cdfaso nested_block_comment_closing{
		dfa_state_code::nested_block_comment_closing,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'<':
				return nested_block_comment_nesting;
			case U'>':
				return nested_block_comment_closed;
			case U'/':
				return nested_block_comment_closing;
			default:
				break;
			}
			return nested_block_comment;
		}
	};
	inline cdfaso nested_block_comment_closed{
		dfa_state_code::nested_block_comment_closed,
		pdn_token_code::eof,
		[](code_point_t c) noexcept
		{
			return nested_block_comment.transformer(c);
		}
	};

	inline cdfaso string_opened{
		dfa_state_code::string_opened,
		pdn_token_code::literal_string,
		[](code_point_t c) noexcept
		{
			return string.transformer(c);
		}
	};
	inline cdfaso string{
		dfa_state_code::string,
		pdn_token_code::literal_string,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\n':
				return string_with_LF;
			case U'\\':
				return string_escape;
			case U'\"':
				return string_closed;
			default:
				if (lexer_utility::is_in_translation_character_set(c))
				{
					return string;
				}
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso string_closed{
		dfa_state_code::string_closed,
		pdn_token_code::literal_string,
		to_unmatched
	};
	inline cdfaso string_escape{
		dfa_state_code::string_escape,
		pdn_token_code::literal_string
	};
	inline cdfaso string_with_LF{
		dfa_state_code::string_with_LF,
		pdn_token_code::literal_string
	};
		
	inline cdfaso character_opened{
		dfa_state_code::character_opened,
		pdn_token_code::literal_character,
		[](code_point_t c) noexcept
		{
			return character.transformer(c);
		}
	};
	inline cdfaso character{
		dfa_state_code::character,
		pdn_token_code::literal_character,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\n':
				return character_with_LF;
			case U'\\':
				return character_escape;
			case U'\'':
				return character_closed;
			default:
				if (lexer_utility::is_in_translation_character_set(c))
				{
					return character;
				}
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso character_closed{
		dfa_state_code::character_closed,
		pdn_token_code::literal_character,
		to_unmatched
	};
	inline cdfaso character_escape{
		dfa_state_code::character_escape,
		pdn_token_code::literal_character
	};
	inline cdfaso character_with_LF{
		dfa_state_code::character_with_LF,
		pdn_token_code::literal_character
	};

	inline cdfaso at_sign{
		dfa_state_code::at_sign,
		pdn_token_code::at_sign,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'`':
				return identifier_raw_string_d_seq_opened;
			case U'\"':
				return raw_string_d_seq_opened;
			default:
				if (lexer_utility::is_allowed_as_first_char_of_identifier(c))
				{
					return at_identifier;
				}
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso at_identifier{
		dfa_state_code::at_identifier,
		pdn_token_code::at_identifier,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_allowed_in_identifier(c))
				return at_identifier;
			return unmatched;
		}
	};

	inline cdfaso raw_string_d_seq_opened{
		dfa_state_code::raw_string_d_seq_opened,
		pdn_token_code::literal_string
	};
	inline cdfaso raw_string{
		dfa_state_code::raw_string,
		pdn_token_code::literal_string,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U')':
				return raw_string_received_right_parentheses;
			case U'\r':
				return raw_string_received_CR;
			default:
				if (lexer_utility::is_in_translation_character_set(c))
					return raw_string;
			}
			return unmatched;
		}
	};
	inline cdfaso raw_string_received_CR{
		dfa_state_code::raw_string_received_CR,
		pdn_token_code::literal_string
	};
	inline cdfaso raw_string_received_right_parentheses{
		dfa_state_code::raw_string_received_right_parentheses,
		pdn_token_code::literal_string
	};
	inline cdfaso raw_string_closed{
		dfa_state_code::raw_string_closed,
		pdn_token_code::literal_string,
		to_unmatched
	};

	inline cdfaso identifier_raw_string_d_seq_opened{
		dfa_state_code::identifier_raw_string_d_seq_opened,
		pdn_token_code::identifier
	};
	inline cdfaso identifier_raw_string{
		dfa_state_code::identifier_raw_string,
		pdn_token_code::identifier,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U')':
				return identifier_raw_string_received_right_parentheses;
			case U'\r':
				return identifier_raw_string_received_CR;
			default:
				if (lexer_utility::is_in_translation_character_set(c))
					return identifier_raw_string;
			}
			return unmatched;
		}
	};
	inline cdfaso identifier_raw_string_received_CR{
		dfa_state_code::identifier_raw_string_received_CR,
		pdn_token_code::identifier
	};
	inline cdfaso identifier_raw_string_received_right_parentheses{
		dfa_state_code::identifier_raw_string_received_right_parentheses,
		pdn_token_code::identifier
	};
	inline cdfaso identifier_raw_string_closed{
		dfa_state_code::identifier_raw_string_closed,
		pdn_token_code::identifier,
		to_unmatched
	};

	inline cdfaso dec_seq{
		dfa_state_code::dec_seq,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return dec_seq_with_quote;
			case U'.':
				return fp_dec_part_first_after_dec_with_dot;
			case U'E':
			case U'e':
				return fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_digit(c))
					return dec_seq;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso dec_seq_with_quote{
		dfa_state_code::dec_seq_with_quote,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return dec_seq;
			if (c == U'\'')
				return dec_seq_with_quote;
			return unmatched;
		}
	};
	inline cdfaso fp_dec_part_first_after_dec_with_dot{
		dfa_state_code::fp_dec_part_first_after_dec_with_dot,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'E':
			case U'e':
				return fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_digit(c))
					return fp_dec_part;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso fp_dec_part{
		dfa_state_code::fp_dec_part,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return fp_dec_part_with_quote;
			case U'E':
			case U'e':
				return fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_digit(c))
					return fp_dec_part;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso fp_dec_part_with_quote{
		dfa_state_code::fp_dec_part_with_quote,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return fp_dec_part;
			if (c == U'\'')
				return fp_dec_part_with_quote;
			return unmatched;
		}
	};
	inline cdfaso fp_exp_sign_or_first{
		dfa_state_code::fp_exp_sign_or_first,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'+':
			case U'-':
				return fp_exp_first;
			default:
				if (lexer_utility::is_digit(c))
					return fp_exp;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso fp_exp_first{
		dfa_state_code::fp_exp_first,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return fp_exp;
			return unmatched;
		}
	};
	inline cdfaso fp_exp{
		dfa_state_code::fp_exp,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return fp_exp;
			if (c == U'\'')
				return fp_exp_with_quote;
			return unmatched;
		}
	};
	inline cdfaso fp_exp_with_quote{
		dfa_state_code::fp_exp_with_quote,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return fp_exp;
			if (c == U'\'')
				return fp_exp_with_quote;
			return unmatched;
		}
	};
	inline cdfaso zero{
		dfa_state_code::zero,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'0':
			case U'1':
			case U'2':
			case U'3':
			case U'4':
			case U'5':
			case U'6':
			case U'7':
				return oct_seq;
			case U'8':
			case U'9':
				return dec_seq_start_with_0;
			case U'\'':
				return oct_seq_with_quote;
			case U'B':
			case U'b':
				return bin_seq_first;
			case U'X':
			case U'x':
				return hex_seq_first;
			case U'.':
				return fp_dec_part_first_after_dec_with_dot;
			case U'E':
			case U'e':
				return fp_exp_sign_or_first;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso oct_seq{
		dfa_state_code::oct_seq,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'0':
			case U'1':
			case U'2':
			case U'3':
			case U'4':
			case U'5':
			case U'6':
			case U'7':
				return oct_seq;
			case U'8':
			case U'9':
				return dec_seq_start_with_0;
			case U'\'':
				return oct_seq_with_quote;
			case U'.':
				return fp_dec_part_first_after_dec_with_dot;
			case U'E':
			case U'e':
				return fp_exp_sign_or_first;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso oct_seq_with_quote{
		dfa_state_code::oct_seq_with_quote,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return oct_seq_with_quote;
			case U'0':
			case U'1':
			case U'2':
			case U'3':
			case U'4':
			case U'5':
			case U'6':
			case U'7':
				return oct_seq;
			case U'8':
			case U'9':
				return dec_seq_start_with_0;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso dec_seq_start_with_0{
		dfa_state_code::dec_seq_start_with_0,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return dec_seq_start_with_0_with_quote;
			case U'.':
				return fp_dec_part_first_after_dec_with_dot;
			case U'E':
			case U'e':
				return fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_digit(c))
					return dec_seq_start_with_0;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso dec_seq_start_with_0_with_quote{
		dfa_state_code::dec_seq_start_with_0_with_quote,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return dec_seq_start_with_0;
			if (c == U'\'')
				return dec_seq_start_with_0_with_quote;
			return unmatched;
		}
	};
	inline cdfaso bin_seq_first{
		dfa_state_code::bin_seq_first,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'0':
			case U'1':
				return bin_seq;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso bin_seq{
		dfa_state_code::bin_seq,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'0':
			case U'1':
				return bin_seq;
			case U'\'':
				return bin_seq_with_quote;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso bin_seq_with_quote{
		dfa_state_code::bin_seq_with_quote,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return bin_seq_with_quote;
			case U'0':
			case U'1':
				return bin_seq;
			default:
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_seq_first{
		dfa_state_code::hex_seq_first,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'.':
				return hex_fp_seq_start_with_0x_dot;
			default:
				if (lexer_utility::is_hex(c))
					return hex_seq;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_seq{
		dfa_state_code::hex_seq,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return hex_seq_with_quote;
			case U'.':
				return hex_fp_dec_part_first_after_hex_with_dot;
			case U'P':
			case U'p':
				return hex_fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_hex(c))
					return hex_seq;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_seq_with_quote{
		dfa_state_code::hex_seq_with_quote,
		pdn_token_code::literal_integer,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_hex(c))
				return hex_seq;
			if (c == U'\'')
				return hex_seq_with_quote;
			return unmatched;
		}
	};
	inline cdfaso hex_fp_dec_part_first_after_hex_with_dot{
		dfa_state_code::hex_fp_dec_part_first_after_hex_with_dot,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'P':
			case U'p':
				return hex_fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_hex(c))
					return hex_fp_dec_part;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_fp_dec_part{
		dfa_state_code::hex_fp_dec_part,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return hex_fp_dec_part_with_quote;
			case U'P':
			case U'p':
				return hex_fp_exp_sign_or_first;
			default:
				if (lexer_utility::is_hex(c))
					return hex_fp_dec_part;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_fp_dec_part_with_quote{
		dfa_state_code::hex_fp_dec_part_with_quote,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_hex(c))
				return hex_fp_dec_part;
			if (c == U'\'')
				return hex_fp_dec_part_with_quote;
			return unmatched;
		}
	};
	inline cdfaso hex_fp_exp_sign_or_first{
		dfa_state_code::hex_fp_exp_sign_or_first,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'+':
			case U'-':
				return hex_fp_exp_first;
			default:
				if (lexer_utility::is_digit(c))
					return hex_fp_exp;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_fp_exp_first{
		dfa_state_code::hex_fp_exp_first,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return hex_fp_exp;
			return unmatched;
		}
	};
	inline cdfaso hex_fp_exp{
		dfa_state_code::hex_fp_exp,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			switch (c)
			{
			case U'\'':
				return hex_fp_exp_with_quote;
			default:
				if (lexer_utility::is_digit(c))
					return hex_fp_exp;
				break;
			}
			return unmatched;
		}
	};
	inline cdfaso hex_fp_exp_with_quote{
		dfa_state_code::hex_fp_exp_with_quote,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return hex_fp_exp;
			if (c == U'\'')
				return hex_fp_exp_with_quote;
			return unmatched;
		}
	};
	inline cdfaso hex_fp_seq_start_with_0x_dot{
		dfa_state_code::hex_fp_seq_start_with_0x_dot,
		pdn_token_code::literal_floating_point,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_hex(c))
				return hex_fp_dec_part;
			return unmatched;
		}
	};

	inline cdfaso dot{
		dfa_state_code::dot,
		pdn_token_code::dot,
		[](code_point_t c) noexcept
		{
			if (lexer_utility::is_digit(c))
				return fp_dec_part;
			return unmatched;
		}
	};

	inline cdfaso tilde               { dfa_state_code::tilde,                pdn_token_code::tilde,                to_unmatched };
	inline cdfaso exclamation_mark    { dfa_state_code::exclamation_mark,     pdn_token_code::exclamation_mark,     to_unmatched };
	inline cdfaso hash                { dfa_state_code::hash,                 pdn_token_code::hash,                 to_unmatched };
	inline cdfaso dollar              { dfa_state_code::dollar,               pdn_token_code::dollar,               to_unmatched };
	inline cdfaso percent             { dfa_state_code::percent,              pdn_token_code::percent,              to_unmatched };
	inline cdfaso caret               { dfa_state_code::caret,                pdn_token_code::caret,                to_unmatched };
	inline cdfaso ampersand           { dfa_state_code::ampersand,            pdn_token_code::ampersand,            to_unmatched };
	inline cdfaso asterisk            { dfa_state_code::asterisk,             pdn_token_code::asterisk,             to_unmatched };
	inline cdfaso left_parentheses    { dfa_state_code::left_parentheses,     pdn_token_code::left_parentheses,     to_unmatched };
	inline cdfaso right_parentheses   { dfa_state_code::right_parentheses,    pdn_token_code::right_parentheses,    to_unmatched };
	inline cdfaso left_brackets       { dfa_state_code::left_brackets,        pdn_token_code::left_brackets,        to_unmatched };
	inline cdfaso right_brackets      { dfa_state_code::right_brackets,       pdn_token_code::right_brackets,       to_unmatched };
	inline cdfaso left_curly_brackets { dfa_state_code::left_curly_brackets,  pdn_token_code::left_curly_brackets,  to_unmatched };
	inline cdfaso right_curly_brackets{ dfa_state_code::right_curly_brackets, pdn_token_code::right_curly_brackets, to_unmatched };
	inline cdfaso minus               { dfa_state_code::minus,                pdn_token_code::minus,                to_unmatched };
	inline cdfaso plus                { dfa_state_code::plus,                 pdn_token_code::plus,                 to_unmatched };
	inline cdfaso equal               { dfa_state_code::equal,                pdn_token_code::equal,                to_unmatched };
	inline cdfaso back_slash          { dfa_state_code::back_slash,           pdn_token_code::back_slash,           to_unmatched };
	inline cdfaso bar                 { dfa_state_code::bar,                  pdn_token_code::bar,                  to_unmatched };
	inline cdfaso colon               { dfa_state_code::colon,                pdn_token_code::colon,                to_unmatched };
	inline cdfaso semicolon           { dfa_state_code::semicolon,            pdn_token_code::semicolon,            to_unmatched };
	inline cdfaso greater_than        { dfa_state_code::greater_than,         pdn_token_code::greater_than,         to_unmatched };
	inline cdfaso comma               { dfa_state_code::comma,                pdn_token_code::comma,                to_unmatched };
	inline cdfaso question_mark       { dfa_state_code::question_mark,        pdn_token_code::question_mark,        to_unmatched };
	inline cdfaso infinity            { dfa_state_code::infinity,             pdn_token_code::invalid,              to_unmatched };
}

#endif
