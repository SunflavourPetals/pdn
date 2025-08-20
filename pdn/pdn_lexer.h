#ifndef PDN_Header_pdn_lexer
#define PDN_Header_pdn_lexer

#include <cassert>
#include <cstdint>
#include <string>
#include <memory>
#include <format>
#include <limits>
#include <utility>
#include <optional>
#include <concepts>
#include <algorithm>
#include <type_traits>

#include "pdn_dfa_state_code.h"
#include "pdn_dfa_state_object.h"
#include "pdn_dfa_state_objects.h"

#include "pdn_source_position_recorder_concept.h"
#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator_concept.h"

#include "pdn_code_convert.h"
#include "pdn_lexical_error_code.h"

namespace pdn::concepts
{
	template <typename type, typename char_t>
	concept function_package_for_lexer
		 = concepts::source_position_getter<type>
		&& concepts::error_handler<type>
		&& concepts::error_message_generator<type>;
}

namespace pdn
{
	template <unicode::concepts::code_unit char_t, concepts::function_package_for_lexer<char_t> function_package>
	class lexer;
}

namespace pdn
{
	template <typename char_t, typename function_package, typename it_begin_t, typename it_end_t>
	struct token_iterator_ctrlblk_from_lexer
	{
		using lexer_t = lexer<char_t, function_package>;
		using token_t = token<char_t>;
		lexer_t    lex;
		it_begin_t begin;
		it_end_t   end;
		token_t get_token()
		{
			return lex.get_token(begin, end); // if begin == end then return eof
		}
	};
	
	template <typename char_t, typename function_package, typename it_begin_t, typename it_end_t>
	class token_iterator_from_lexer
	{
	private:
		using ctrlblk_t = token_iterator_ctrlblk_from_lexer<char_t, function_package, it_begin_t, it_end_t>;
		using lexer_t   = lexer<char_t, function_package>;
		using token_t   = token<char_t>;
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using size_type         = ::std::size_t;
		using value_type        = token_t;
	public:
		bool eof() const noexcept
		{
			return ctrl_opt.has_value();
		}
		void to_next()
		{
			to_next_impl();
		}
		const value_type& operator*() const noexcept
		{
			return stored_token;
		}
		token_iterator_from_lexer& operator++()
		{
			to_next();
			return *this;
		}
		friend bool operator==(const token_iterator_from_lexer& lhs, const token_iterator_from_lexer& rhs)
		{
			return lhs.ctrl_opt.has_value() == rhs.ctrl_opt.has_value();
		}
		// construct end iter
		token_iterator_from_lexer() = default;
		token_iterator_from_lexer(lexer_t lex, it_begin_t begin, it_end_t end) :
			ctrl_opt{ ::std::make_optional<ctrlblk_t>(::std::move(lex), ::std::move(begin), ::std::move(end)) }
		{
			to_next();
		}
	private:
		token_t get_token()
		{
			return ctrl_opt->get_token();
		}
		void to_next_impl()
		{
			stored_token = get_token();
			if (stored_token.code == pdn_token_code::eof)
			{
				ctrl_opt = ::std::nullopt;
			}
		}
		token_t                    stored_token{ .position = {}, .code = pdn_token_code::eof, .value = {} };
		::std::optional<ctrlblk_t> ctrl_opt{ ::std::nullopt };
	};

	template <typename char_t, typename func_pkg, typename it_begin_t, typename it_end_t>
	auto make_token_iterator(lexer<char_t, func_pkg> lex, it_begin_t begin, it_end_t end)
	{
		return token_iterator_from_lexer<char_t, func_pkg, it_begin_t, it_end_t>{ ::std::move(lex), ::std::move(begin), ::std::move(end) };
	}
	template <typename char_t, typename func_pkg, typename it_begin_t, typename it_end_t>
	auto make_end_token_iterator(const lexer<char_t, func_pkg>&, const it_begin_t&, const it_end_t&)
	{
		return token_iterator_from_lexer<char_t, func_pkg, it_begin_t, it_end_t>{};
	}
	template <typename char_t, typename func_pkg, typename it_begin_t, typename it_end_t>
	auto make_end_token_iterator(const token_iterator_from_lexer<char_t, func_pkg, it_begin_t, it_end_t>&)
	{
		return token_iterator_from_lexer<char_t, func_pkg, it_begin_t, it_end_t>{};
	}
	template <typename char_t, typename func_pkg, typename it_begin_t, typename it_end_t>
	auto make_end_token_iterator()
	{
		return token_iterator_from_lexer<char_t, func_pkg, it_begin_t, it_end_t>{};
	}
}

namespace pdn
{
	template <unicode::concepts::code_unit char_t, concepts::function_package_for_lexer<char_t> function_package>
	class lexer
	{
	public:
		token<char_t> get_token(auto&& begin, auto end)
		{
			token<char_t>              result{};
			types::string<char_t>      text{};   // rename text
			unicode::code_point_string open_d_seq{};      // delimiter-sequence for raw string
			::std::string              number_sequence{}; // Unicode[U+0000, U+007f] -> ASCII -> form_chars
			::std::size_t              nested_block_comment_layer{};
			::std::size_t              cont_n_delimiter_count{}; // consecutive number delimiters' count
			source_position            position{ get_pos() };
			auto                       dfa_state = dfa_state_objects::start_state();

			auto num_seq_to_ems = [&]() { return reinterpret_to_err_msg_str(number_sequence); };
			auto append = [](types::string<char_t>& src, unicode::code_point_t c)
			{
				auto encode_r = unicode::encode<char_t>(c);
				assert((bool)encode_r && "encode failed");
				src.append(encode_r.cbegin(), encode_r.cend());
			};

			using unicode::code_convert;
			using lex_ec = lexical_error_code;
			using err_ms = error_msg_string;

			while (begin != end)
			{
				auto c = *begin;

				if (!unicode::is_scalar_value(c))
				{
					using raw_err = raw_error_message_type::not_unicode_scalar_value;
					post_err(get_pos(), lex_ec::not_unicode_scalar_value, raw_err{ c });
					++begin;
					continue;
				}

				auto new_dfa_state = dfa_state.transformer(c);

				auto update_token_pos = [&]()
				{
					auto prev = dfa_state.state_code;
					auto next = new_dfa_state.state_code;
					if (is_non_token_state(prev) && is_token_state(next))
					{
						position = get_pos();
					}
				};

				update_token_pos();

				using enum dfa_state_code;
				switch (new_dfa_state.state_code)
				{
					// stop dfa and make token
				case unmatched:
					goto label_out_of_loop;

					// error state

					// accept non-comments, non-whitespace characters, non-first-char-of-tokens from the Start state
				case unacceptable_character:
				{
					new_dfa_state = dfa_state_objects::start_state();
					auto cp = unicode::code_point_t(c);
					auto view = unicode::code_point_string_view{ &cp, 1 };
					post_err(get_pos(), lex_ec::unacceptable_character, to_raw_err_str(code_convert<err_ms>(view)));
					break;
				}
				case infinity:
				{
					using namespace unicode_literals;
					using ct = char_t;
					text = types::string<ct>{ ct('i'), ct('n'), ct('f'), ct('i'), ct('n'), ct('i'), ct('t'), ct('y') };
					new_dfa_state.state_code = dfa_state_objects::at_identifier.state_code;
					new_dfa_state.token_code = dfa_state_objects::at_identifier.token_code;
					break;
				}
				case identifier_string_with_LF:
				{
					new_dfa_state = dfa_state_objects::identifier_string_closed;
					post_err(position, lex_ec::identifier_string_terminated_by_LF, to_raw_err_str(code_convert<err_ms>(text)));
					goto label_update_dfa_state;
				}
				case string_with_LF:
				{
					new_dfa_state = dfa_state_objects::string_closed;
					post_err(position, lex_ec::string_literal_terminated_by_LF, to_raw_err_str(code_convert<err_ms>(text)));
					goto label_update_dfa_state;
				}
				case character_with_LF:
				{
					new_dfa_state = dfa_state_objects::character_closed;
					post_err(position, lex_ec::character_literal_terminated_by_LF, to_raw_err_str(code_convert<err_ms>(text)));
					goto label_update_dfa_state;
				}
				case start:
				case line_comment:
				case block_comment:
				case block_comment_closing:
				case nested_block_comment:
				case nested_block_comment_nesting:
				case nested_block_comment_closing:
					break;
				case nested_block_comment_nested:
					++nested_block_comment_layer;
					break;
				case nested_block_comment_closed:
					--nested_block_comment_layer;
					if (nested_block_comment_layer == 0)
					{
						new_dfa_state = dfa_state_objects::start_state();
					}
					break;
				case identifier:
				case identifier_string:
				case string:
				case character:
				case at_identifier:
				case raw_string:
				case identifier_raw_string:
					append(text, c);
					break;
				case identifier_string_escape:
					++begin;
					new_dfa_state = dfa_state_objects::identifier_string;
					if (get_escape(c, begin, end, true))
					{
						append(text, c);
					}
					goto label_update_dfa_state;
				case string_escape:
					++begin;
					new_dfa_state = dfa_state_objects::string;
					if (get_escape(c, begin, end))
					{
						append(text, c);
					}
					goto label_update_dfa_state;
				case character_escape:
					++begin;
					new_dfa_state = dfa_state_objects::character;
					if (get_escape(c, begin, end))
					{
						append(text, c);
					}
					goto label_update_dfa_state;
				case raw_string_d_seq_opened:
					++begin;
					// @"d_seq()d_seq"
					//  ^ -> to first d sequence character or '('
					get_raw_string_opening_d_seq(open_d_seq, begin, end, false);
					new_dfa_state = dfa_state_objects::raw_string;
					goto label_update_dfa_state;
				case raw_string_received_CR:
					++begin;
					if (begin == end)
					{
						text += char_t('\r');
						new_dfa_state = dfa_state_objects::raw_string;
						goto label_update_dfa_state;
					}
					c = *begin;
					if (c != U'\n')
					{
						text += char_t('\r');
					}
					append(text, c);
					new_dfa_state = dfa_state_objects::raw_string;
					goto label_move_iterator;
				case raw_string_received_right_parentheses:
				{
					unicode::code_point_string close_d_seq{};
					++begin;
					if (get_raw_string_closing_d_seq(open_d_seq, close_d_seq, begin, end, U'"'))
					{
						new_dfa_state = dfa_state_objects::raw_string_closed;
					}
					else
					{
						text += char_t(')');
						text += unicode::code_convert<types::string<char_t>>(close_d_seq);
						new_dfa_state = dfa_state_objects::raw_string;
					}
					goto label_update_dfa_state;
				}
				case identifier_raw_string_d_seq_opened:
					++begin;
					// @`d_seq()d_seq`
					//  ^ -> to first d sequence character or '('
					get_raw_string_opening_d_seq(open_d_seq, begin, end, true);
					new_dfa_state = dfa_state_objects::identifier_raw_string;
					goto label_update_dfa_state;
				case identifier_raw_string_received_CR:
					++begin;
					if (begin == end)
					{
						text += char_t('\r');
						new_dfa_state = dfa_state_objects::identifier_raw_string;
						goto label_update_dfa_state;
					}
					c = *begin;
					if (c != U'\n')
					{
						text += char_t('\r');
					}
					append(text, c);
					new_dfa_state = dfa_state_objects::identifier_raw_string;
					goto label_move_iterator;
				case identifier_raw_string_received_right_parentheses:
				{
					unicode::code_point_string close_d_seq{};
					++begin;
					if (get_raw_string_closing_d_seq(open_d_seq, close_d_seq, begin, end, U'`'))
					{
						new_dfa_state = dfa_state_objects::identifier_raw_string_closed;
					}
					else
					{
						text += char_t(')');
						text += unicode::code_convert<types::string<char_t>>(close_d_seq);
						new_dfa_state = dfa_state_objects::identifier_raw_string;
					}
					goto label_update_dfa_state;
				}
				case zero:
				case dot:
				case dec_seq:
				case fp_dec_part_first_after_dec_with_dot:
				case fp_exp_sign_or_first:
				case fp_exp_first:
				case bin_seq_first:
				case hex_seq_first:
				case hex_fp_dec_part_first_after_hex_with_dot:
				case hex_fp_dec_part:
				case hex_fp_exp_sign_or_first:
				case hex_fp_exp_first:
				case hex_fp_seq_start_with_0x_dot:
				case fp_dec_part:
				case fp_exp:
				case oct_seq:
				case dec_seq_start_with_0:
				case bin_seq:
				case hex_seq:
				case hex_fp_exp:
					number_sequence += char(c);
					if (cont_n_delimiter_count > 1)
					{
						post_err(get_pos(), lex_ec::more_than_one_separators_between_numbers, to_raw_err_str(num_seq_to_ems()));
					}
					cont_n_delimiter_count = 0;
					break;
				case dec_seq_with_quote:
				case fp_dec_part_with_quote:
				case fp_exp_with_quote:
				case oct_seq_with_quote:
				case dec_seq_start_with_0_with_quote:
				case bin_seq_with_quote:
				case hex_seq_with_quote:
				case hex_fp_dec_part_with_quote:
				case hex_fp_exp_with_quote:
					number_sequence += char(c);
					++cont_n_delimiter_count;
					break;
				default:
					break;
				}
			label_move_iterator:
				++begin;
			label_update_dfa_state:
				dfa_state = new_dfa_state;
			}

		label_out_of_loop:

			// Make token now.
			// The processing of text needs to be done in a non-default case,
			//     for example, a char sequence of integer needs to be converted to an integer.
			// Other final-states go into the default case without additional processing.
			// If the non-final state goes to default, an internal error is reported.
			// The error status must be handled in a non-default case (at least the error needs to be reported).

			result.code = dfa_state.token_code;
			result.position = position;

			using enum dfa_state_code;
			switch (dfa_state.state_code)
			{
				// ERROR STATES vvv
			case block_comment:
			case block_comment_closing:
				post_err(position, lexical_error_code::unterminated_block_comment, {});
				break;
			case nested_block_comment:
			case nested_block_comment_nesting:
			case nested_block_comment_nested:
			case nested_block_comment_closing:
			case nested_block_comment_closed:
				post_err(position, lex_ec::unterminated_nested_block_comment, {});
				break;
			case identifier_string_opened:
			case identifier_string:
			case string_opened:
			case string:
			case identifier_raw_string:
			case raw_string:
				process_string_like_error(dfa_state.state_code, position, to_err_ms(text), open_d_seq);
				[[fallthrough]];
				// ERROR STATES ^^^
			case identifier:
			case identifier_string_closed:
			case string_closed:
			case raw_string_closed:
			case identifier_raw_string_closed:
				result.value = make_proxy<types::string<char_t>>(::std::move(text));
				break;
			case at_identifier:
				if constexpr (::std::same_as<char_t, unicode::utf_8_code_unit_t>)
				{
					result.value = dev_util::at_iden_string_proxy{ ::std::move(text) };
				}
				else
				{
					result.value = dev_util::at_iden_string_proxy{ code_convert<unicode::utf_8_code_unit_string>(text) };
				}
				break;
			case character_opened: // <<< ERROR STATE
			case character:        // <<< ERROR STATE
				process_char_missing_quote_error(position, to_err_ms(text));
				[[fallthrough]];
			case character_closed:
				result.value = process_char_literal(text, position);
				break;
			case dec_seq_start_with_0:             // <<< ERROR STATE
			case dec_seq_start_with_0_with_quote:  // <<< ERROR STATE
			case dec_seq_with_quote:               // <<< ERROR STATE
				process_dec_seq_error(dfa_state.state_code, position, num_seq_to_ems());
				[[fallthrough]];
			case dec_seq:
			{
				::std::uint_least64_t integer_value{};
				remove_all_separator(number_sequence);
				const auto& n_seq = number_sequence;
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 10);
				to_appropriate_int_type(result.value, integer_value);
				if (!check_from_chars_succeed(from_chars_r, n_seq.data() + n_seq.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, n_seq, position, dec_integer, false);
				}
				break;
			}
			case fp_exp_sign_or_first:    // <<< ERROR STATE
			case fp_exp_first:            // <<< ERROR STATE
			case fp_dec_part_with_quote:  // <<< ERROR STATE
			case fp_exp_with_quote:       // <<< ERROR STATE
				if (dfa_state.state_code == fp_exp_sign_or_first || dfa_state.state_code == fp_exp_first)
				{
					post_err(position, lex_ec::fp_dec_expect_exponent, to_raw_err_str(num_seq_to_ems()));
					number_sequence += '0'; // add 0 for exponent
				}
				else if (dfa_state.state_code == fp_dec_part_with_quote || dfa_state.state_code == fp_exp_with_quote)
				{
					using raw_err = raw_error_message_type::number_end_with_separator;
					using enum raw_error_message_type::number_type;
					post_err(position, lex_ec::number_cannot_end_with_separator, raw_err{ num_seq_to_ems(), dec_floating });
				}
				[[fallthrough]];
			case fp_dec_part_first_after_dec_with_dot:
			case fp_dec_part:
			case fp_exp:
			{
				types::f64 fp_value{};
				remove_all_separator(number_sequence);
				const auto& n_seq = number_sequence;
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), fp_value);
				result.value = fp_value;
				if (!check_from_chars_succeed(from_chars_r, n_seq.data() + n_seq.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, n_seq, position, dec_floating, false);
				}
				break;
			}
			case oct_seq_with_quote:  // <<< ERROR STATE
			{
				using raw_err = raw_error_message_type::number_end_with_separator;
				using enum raw_error_message_type::number_type;
				post_err(position, lex_ec::number_cannot_end_with_separator, raw_err{ num_seq_to_ems(), oct_integer });
				[[fallthrough]];
			}
			case zero:
			case oct_seq:
			{
				::std::uint_least64_t integer_value{};
				remove_all_separator(number_sequence);
				const auto& n_seq = number_sequence;
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 8);
				to_appropriate_int_type(result.value, integer_value);
				if (!check_from_chars_succeed(from_chars_r, n_seq.data() + n_seq.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, n_seq, position, oct_integer, false);
				}
			}
			break;
			case bin_seq_first:       // <<< ERROR STATE
			case bin_seq_with_quote:  // <<< ERROR STATE
				if (dfa_state.state_code == bin_seq_first)
				{
					post_err(position, lex_ec::bin_prefix_expect_bin_number_sequence, to_raw_err_str(num_seq_to_ems()));
					number_sequence += '0';
				}
				else if (dfa_state.state_code == bin_seq_with_quote)
				{
					using raw_err = raw_error_message_type::number_end_with_separator;
					using enum raw_error_message_type::number_type;
					post_err(position, lex_ec::number_cannot_end_with_separator, raw_err{ num_seq_to_ems(), bin_integer });
				}
				[[fallthrough]];
			case bin_seq:
			{
				assert(check_0b_prefix(number_sequence) && "binary literal without prefix");
				types::u64 integer_value{};
				remove_all_separator(number_sequence);
				auto n_seq = ::std::string_view{ number_sequence.begin() + 2, number_sequence.end() };
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 2);
				to_appropriate_int_type(result.value, integer_value);
				if (!check_from_chars_succeed(from_chars_r, n_seq.data() + n_seq.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, n_seq, position, bin_integer, false);
				}
				break;
			}
			case hex_seq_first:       // <<< ERROR STATE
			case hex_seq_with_quote:  // <<< ERROR STATE
				if (dfa_state.state_code == hex_seq_first)
				{
					post_err(position, lex_ec::hex_prefix_expect_hex_number_sequence, to_raw_err_str(num_seq_to_ems()));
					number_sequence += '0';
				}
				else if (dfa_state.state_code == hex_seq_with_quote)
				{
					using raw_err = raw_error_message_type::number_end_with_separator;
					using enum raw_error_message_type::number_type;
					post_err(position, lex_ec::number_cannot_end_with_separator, raw_err{ num_seq_to_ems(), hex_integer });
				}
				[[fallthrough]];
			case hex_seq:
			{
				assert(check_0x_prefix(number_sequence) && "hexadecimal literal without prefix");
				types::u64 integer_value{};
				remove_all_separator(number_sequence);
				auto n_seq = ::std::string_view{ number_sequence.begin() + 2, number_sequence.end() };
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 16);
				to_appropriate_int_type(result.value, integer_value);
				if (!check_from_chars_succeed(from_chars_r, n_seq.data() + n_seq.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, n_seq, position, hex_integer, false);
				}
				break;
			}
			case hex_fp_dec_part_with_quote:               // <<< ERROR STATE
			case hex_fp_dec_part_first_after_hex_with_dot: // <<< ERROR STATE
			case hex_fp_dec_part:                          // <<< ERROR STATE
			case hex_fp_exp_sign_or_first:                 // <<< ERROR STATE
			case hex_fp_exp_first:                         // <<< ERROR STATE
			case hex_fp_exp_with_quote:                    // <<< ERROR STATE
			case hex_fp_seq_start_with_0x_dot:             // <<< ERROR STATE
				process_hex_fp_errors(dfa_state.state_code, number_sequence, position);
				[[fallthrough]];
			case hex_fp_exp:
			{
				assert(check_0x_prefix(number_sequence) && "hexadecimal literal without prefix");
				types::f64 fp_value{};
				remove_all_separator(number_sequence);
				auto n_seq = ::std::string_view{ number_sequence.begin() + 2, number_sequence.end() };
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), fp_value, ::std::chars_format::hex);
				result.value = fp_value;
				if (!check_from_chars_succeed(from_chars_r, n_seq.data() + n_seq.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, n_seq, position, hex_floating, false);
				}
				break;
			}
			default:
				// if dfa state is not final-state，then should process error in non-default case
				assert(dfa_state.is_final() && "error unhandled");
				break;
			}

			return result;
		}

		explicit lexer(function_package& function_pkg) : func_pkg{ &function_pkg } {}
	private:
		static constexpr bool is_non_token_state(dfa_state_code c) noexcept
		{
			using enum dfa_state_code;
			switch (c)
			{
				// these states mean that the delimiter is currently being processed, not the token
			case start:
			case line_comment:
			case block_comment:
			case block_comment_closing:
			case nested_block_comment:
			case nested_block_comment_nesting:
			case nested_block_comment_closing:
			case nested_block_comment_nested:
			case nested_block_comment_closed:
			case unacceptable_character:
				return true;
			default:
				break;
			}
			return false;
		}
		static constexpr bool is_token_state(dfa_state_code c) noexcept
		{
			return !is_non_token_state(c);
		}
		static void to_appropriate_int_type(token_value_variant<char_t>& target, types::u64 val)
		{
			constexpr auto cppint_max = static_cast<types::u64>(::std::numeric_limits<types::auto_int>::max());
			constexpr auto i8_max     = static_cast<types::u64>(::std::numeric_limits<types::i8>::max());
			constexpr auto i16_max    = static_cast<types::u64>(::std::numeric_limits<types::i16>::max());
			constexpr auto i32_max    = static_cast<types::u64>(::std::numeric_limits<types::i32>::max());
			constexpr auto i64_max    = static_cast<types::u64>(::std::numeric_limits<types::i64>::max());
			if (val <= cppint_max)
			{
				target = static_cast<types::auto_int>(val);
			}
			else if (val <= i8_max)
			{
				target = static_cast<types::i8>(val);
			}
			else if (val <= i16_max)
			{
				target = static_cast<types::i16>(val);
			}
			else if (val <= i32_max)
			{
				target = static_cast<types::i32>(val);
			}
			else if (val <= i64_max)
			{
				target = static_cast<types::i64>(val);
			}
			else
			{
				target = val;
			}
		}
		static bool check_no_0b_prefix(const ::std::string& seq)
		{
			return seq.size() < 2 || seq[0] != '0' || (seq[1] != 'b' && seq[1] != 'B');
		}
		static bool check_0b_prefix(const ::std::string& seq)
		{
			return !check_no_0b_prefix(seq);
		}
		static bool check_no_0x_prefix(const ::std::string& seq)
		{
			return seq.size() < 2 || seq[0] != '0' || (seq[1] != 'x' && seq[1] != 'X');
		}
		static bool check_0x_prefix(const ::std::string& seq)
		{
			return !check_no_0x_prefix(seq);
		}
		static bool check_from_chars_succeed(const ::std::from_chars_result from_chars_r, const char* expected)
		{
			return (from_chars_r.ec == ::std::errc{}) && (from_chars_r.ptr == expected);
		}
		static void remove_all_separator(::std::string& num_str)
		{
			::std::erase(num_str, '\'');
		}
		static auto to_raw_err_str(error_msg_string err_msg_str) -> raw_error_message_type::error_string
		{
			return raw_error_message_type::error_string{ ::std::move(err_msg_str) };
		}
		static auto to_err_ms(::std::basic_string_view<char_t> s) -> error_msg_string
		{
			if constexpr (::std::same_as<char_t, error_msg_char>)
			{
				return error_msg_string(s.cbegin(), s.cend());
			}
			else
			{
				return unicode::code_convert<error_msg_string>(s);
			}
		}
		static auto get_esc_not_scalar_err_msg(error_msg_string escape_sign, // e.g. "u" means "\u"
		                                       error_msg_string sequence, // escape sequence
		                                       unicode::code_point_t parse_val,
		                                       bool is_with_curly_brackets) -> raw_error_message_type::escape_not_unicode_scalar_value
		{
			using namespace literals::error_message_literals;
			auto l_bracket = is_with_curly_brackets ? u8"{"_em : u8""_em;
			auto r_bracket = is_with_curly_brackets ? u8"}"_em : u8""_em;
			escape_sign.append(l_bracket).append(sequence).append(r_bracket);
			return { ::std::move(escape_sign), parse_val };
		};
		
		bool get_escape(auto& oc,    // out param, the result of get escape
		                auto& begin, // pointing first char witch after '\'
		                auto  end,
		                bool  enable_escape_back_quote = false) // is enable escape sequence \`
		{
			using escape_value_t = ::std::uint_least32_t;

			auto position = get_pos();

			using enum lexical_error_code;
			if (begin == end)
			{
				post_err(position, escape_error_unknown_escape_sequence, to_raw_err_str(u8""_em));
				return false;
			}

			::std::string sequence{};

			auto seq_to_err_msg_str = [&]() { return reinterpret_to_err_msg_str(sequence); };

			auto c = *begin;

			switch (c)
			{
				// simple escape sequence
			case U'`':
				if (enable_escape_back_quote)
				{
					oc = U'`'; ++begin; return true;
				}
				break;
			case U'\'': oc = U'\''; ++begin; return true;
			case U'\"': oc = U'\"'; ++begin; return true;
			case U'\?': oc = U'\?'; ++begin; return true;
			case U'\\': oc = U'\\'; ++begin; return true;
			case U'a' : oc = U'\a'; ++begin; return true;
			case U'b' : oc = U'\b'; ++begin; return true;
			case U'f' : oc = U'\f'; ++begin; return true;
			case U'n' : oc = U'\n'; ++begin; return true;
			case U'r' : oc = U'\r'; ++begin; return true;
			case U't' : oc = U'\t'; ++begin; return true;
			case U'v' : oc = U'\v'; ++begin; return true;

			case U'o': // \o{n...}
			{
				++begin; // -> {

				if (begin == end || *begin != U'{')
				{
					post_err(position, escape_error_o_not_followed_by_left_brackets, to_raw_err_str(u8"o"_em));
					return false;
				}
				c = *begin;
				++begin; // -> n (oct)

				get_numeric_escape_sequence(sequence, begin, end, lexer_utility::is_oct);

				if (begin == end || *begin != U'}')
				{
					post_err(position, escape_error_o_not_terminated_with_right_brackets, to_raw_err_str(u8"o{"_em + seq_to_err_msg_str()));
					if (sequence.empty())
					{
						post_err(position, escape_error_o_empty_delimited_escape_sequence, to_raw_err_str(u8"o{"_em));
					}
					return false;
				}
				c = *begin;
				++begin;

				if (sequence.empty())
				{
					// The contents of the curly braces cannot be empty
					post_err(position, escape_error_o_empty_delimited_escape_sequence, to_raw_err_str(u8"o{}"_em));
					return false;
				}

				escape_value_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 8);

				bool rt_flag{};
				if (!check_from_chars_succeed(from_chars_r, sequence.data() + sequence.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, sequence, position, oct_integer, true);
					rt_flag = true;
				}
				if (!unicode::is_scalar_value(parse_val)) [[unlikely]]
				{
					auto msg = get_esc_not_scalar_err_msg(u8"o"_em, seq_to_err_msg_str(), parse_val, true);
					post_err(position, escape_error_not_unicode_scalar_value, ::std::move(msg));
					rt_flag = true;
				}
				if (rt_flag) [[unlikely]]
				{
					return false;
				}
				oc = parse_val;
				return true;
			}
			case U'x': // \xn... \x{n...}
			{
				bool is_with_curly_brackets{ false };
				++begin; // -> { or n (hex/)

				if (begin == end)
				{
					post_err(position, escape_error_x_used_with_no_following_hex_digits, to_raw_err_str(u8"x"_em));
					return false;
				}
				c = *begin;
				if (c == U'{')
				{
					is_with_curly_brackets = true;
					++begin; // -> n (hex)
					// No need to update the value of c here
				}

				get_numeric_escape_sequence(sequence, begin, end, lexer_utility::is_hex);

				if (is_with_curly_brackets)
				{
					if (begin == end || *begin != U'}')
					{
						post_err(position, escape_error_x_not_terminated_with_right_brackets, to_raw_err_str(u8"x{"_em + seq_to_err_msg_str()));
						if (sequence.empty())
						{
							post_err(position, escape_error_x_empty_delimited_escape_sequence, to_raw_err_str(u8"x{"_em));
						}
						return false;
					}
					c = *begin;
					++begin;
				}

				if (sequence.empty())
				{
					// length of hexadecimal number sequence cannot be 0 in \x... and \x{...}
					post_err(position, escape_error_x_empty_delimited_escape_sequence,
						to_raw_err_str(is_with_curly_brackets ? u8"x{}"_em : u8"x"_em));
					return false;
				}

				::std::uint_least32_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 16);

				bool rt_flag{};
				if (!check_from_chars_succeed(from_chars_r, sequence.data() + sequence.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, sequence, position, hex_integer, true);
					rt_flag = true;
				}
				if (!unicode::is_scalar_value(parse_val)) [[unlikely]]
				{
					auto msg = get_esc_not_scalar_err_msg(u8"x"_em, seq_to_err_msg_str(), parse_val, is_with_curly_brackets);
					post_err(position, escape_error_not_unicode_scalar_value, ::std::move(msg));
					rt_flag = true;
				}
				if (rt_flag) [[unlikely]]
				{
					return false;
				}
				oc = parse_val;
				return true;
			}
			case U'u': // \unnnn \u{n...} Universal character names
			{
				bool is_with_curly_brackets{ false };
				++begin; // -> { or n (hex/)
				if (begin == end)
				{
					post_err(position, escape_error_u_incomplete_universal_character_name, to_raw_err_str(u8"u"_em));
					return false;
				}
				c = *begin;
				if (c == U'{')
				{
					is_with_curly_brackets = true;
					++begin; // -> n (hex)
					// No need to update the value of c here
				}

				if (is_with_curly_brackets)
				{
					get_numeric_escape_sequence(sequence, begin, end, lexer_utility::is_hex);
				}
				else
				{
					get_numeric_escape_sequence(4, sequence, begin, end, lexer_utility::is_hex);
				}

				if (is_with_curly_brackets)
				{
					if (begin == end || *begin != U'}')
					{
						post_err(position, escape_error_u_not_terminated_with_right_brackets,
							to_raw_err_str(u8"u{"_em + seq_to_err_msg_str()));
						if (sequence.empty())
						{
							post_err(position, escape_error_u_empty_delimited_escape_sequence, to_raw_err_str(u8"u{"_em));
						}
						return false;
					}
					c = *begin;
					++begin;

					if (sequence.empty())
					{
						// length of hexadecimal number sequence cannot be 0 in \u{n...}
						post_err(position, escape_error_u_empty_delimited_escape_sequence, to_raw_err_str(u8"u{}"_em));
						return false;
					}
				}
				else
				{
					if (sequence.size() != 4)
					{
						// length of hexadecimal number sequence cannot be number other than 4 in \unnnn
						post_err(position, escape_error_u_incomplete_universal_character_name,
							to_raw_err_str(u8"u"_em + seq_to_err_msg_str()));
						return false;
					}
				}

				::std::uint_least32_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 16);

				bool rt_flag{};
				if (!check_from_chars_succeed(from_chars_r, sequence.data() + sequence.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, sequence, position, hex_integer, true);
					rt_flag = true;
				}
				if (!unicode::is_scalar_value(parse_val)) [[unlikely]]
				{
					auto msg = get_esc_not_scalar_err_msg(u8"u"_em, seq_to_err_msg_str(), parse_val, is_with_curly_brackets);
					post_err(position, escape_error_not_unicode_scalar_value, ::std::move(msg));
					rt_flag = true;
				}
				if (rt_flag) [[unlikely]]
				{
					return false;
				}
				oc = parse_val;
				return true;
			}
			case U'U':
				// \Unnnnnnnn Universal character names
			{
				++begin; // -> n (hex/)

				if (begin == end)
				{
					post_err(position, escape_error_U_incomplete_universal_character_name, to_raw_err_str(u8"U"_em));
					return false;
				}

				c = *begin;

				get_numeric_escape_sequence(8, sequence, begin, end, lexer_utility::is_hex);

				if (sequence.size() != 8)
				{
					// length of hexadecimal number sequence cannot be number other than 8 in \Unnnnnnnn
					post_err(position, escape_error_U_incomplete_universal_character_name,
						to_raw_err_str(u8"U"_em + seq_to_err_msg_str()));
					return false;
				}

				::std::uint_least32_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 16);

				bool rt_flag{};
				if (!check_from_chars_succeed(from_chars_r, sequence.data() + sequence.size())) [[unlikely]]
				{
					using enum raw_error_message_type::number_type;
					process_from_chars_error(from_chars_r, sequence, position, hex_integer, true);
					rt_flag = true;
				}
				if (!unicode::is_scalar_value(parse_val)) [[unlikely]]
				{
					auto msg = get_esc_not_scalar_err_msg(u8"U"_em, seq_to_err_msg_str(), parse_val, false);
					post_err(position, escape_error_not_unicode_scalar_value, ::std::move(msg));
					rt_flag = true;
				}
				if (rt_flag) [[unlikely]]
				{
					return false;
				}
				oc = parse_val;
				return true;
			}
			default:
				if (lexer_utility::is_oct(c))
				{
					// \n or \nn or \nnn (n is octal number)

					// the function must be implemented correctly to ensure that sequence.size() must be greater than 0
					get_numeric_escape_sequence(3, sequence, begin, end, lexer_utility::is_oct);

					assert(sequence.size() != 0 && "sequence.size() == 0 never be true, logically");

					::std::uint_least32_t parse_val{};
					auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 8);

					bool rt_flag{};
					if (!check_from_chars_succeed(from_chars_r, sequence.data() + sequence.size())) [[unlikely]]
					{
						using enum raw_error_message_type::number_type;
						process_from_chars_error(from_chars_r, sequence, position, oct_integer, true);
						rt_flag = true;
					}
					if (!unicode::is_scalar_value(parse_val)) [[unlikely]]
					{
						auto msg = get_esc_not_scalar_err_msg(u8""_em, seq_to_err_msg_str(), parse_val, false);
						post_err(position, escape_error_not_unicode_scalar_value, ::std::move(msg));
						rt_flag = true;
					}
					if (rt_flag) [[unlikely]]
					{
						return false;
					}
					oc = parse_val;
					return true;
				}
				break;
			}
			auto cp = unicode::code_point_t(c);
			auto err_msg = unicode::code_convert<error_msg_string>(unicode::code_point_string_view{ &cp, 1 });
			post_err(position, escape_error_unknown_escape_sequence, to_raw_err_str(::std::move(err_msg)));
			return false;
		}

		void get_numeric_escape_sequence(::std::string& sequence,
		                                          auto& begin,
		                                          auto  end,
		  ::std::predicate<unicode::code_point_t> auto  is_valid_char) const
		{
			for (; begin != end; ++begin)
			{
				auto c = *begin;
				if (!is_valid_char(c))
				{
					break;
				}
				sequence += static_cast<char>(c);
			}
		}

		auto get_numeric_escape_sequence(::std::size_t  count,
		                                 ::std::string& sequence,
		                                          auto& begin,
		                                          auto  end,
		  ::std::predicate<unicode::code_point_t> auto  is_valid_char) const -> ::std::size_t
		{
			for (::std::size_t read_count{ 0 }; read_count < count && begin != end; ++read_count, ++begin)
			{
				auto c = *begin;
				if (!is_valid_char(c))
				{
					return read_count;
				}
				sequence += static_cast<char>(c);
			}
			return 0;
		}

		// If the function succeeds,
		// begin points to the first character of the raw string,
		// or to ')' if it is an empty string.
		// Returns false when the d_seq is longer than 16 or contains illegal characters,
		// but '(' will be handled by the function just as normal
		bool get_raw_string_opening_d_seq(unicode::code_point_string& out_sequence, auto& begin, auto end, bool is_raw_id_s)
		{
			// begin pointing:
			// 
			// @"d_seq()d_seq"
			//   ^
			// 
			// or:
			// 
			// @`()`
			//   ^

			using raw_err = raw_error_message_type::delimiter_error;
			using err_ms = error_msg_string;
			using namespace literals::error_message_literals;
			using unicode::code_convert;

			auto position = get_pos();
			bool invalid_character_in_raw_string_delimiter{ false };

			using enum lexical_error_code;

			while (begin != end)
			{
				auto c = *begin;
				if (c == U'(')
				{
					++begin;
					if (out_sequence.length() > 16) [[unlikely]]
					{
						post_err(position,
						         d_seq_error_raw_string_delimiter_longer_than_16_characters,
						         raw_err{ code_convert<err_ms>(out_sequence), is_raw_id_s });
						return false;
					}
					return !invalid_character_in_raw_string_delimiter;
				}
				else
				{
					if (!lexer_utility::is_allowed_in_raw_string_d_sequence(c)) [[unlikely]]
					{
						post_err(get_pos(),
						         d_seq_error_invalid_character_in_raw_string_delimiter,
						         raw_err{ code_convert<err_ms>(out_sequence), is_raw_id_s });
						invalid_character_in_raw_string_delimiter = true;
					}
					out_sequence += c;
					++begin;
				}
			}

			post_err(position, d_seq_error_cannot_find_end_sign, raw_err{ code_convert<err_ms>(out_sequence), is_raw_id_s });

			return false;
		}

		// When ')' is detected, ++begin and then call this function.
		// Returns true when the sequence we read matches the referenced delimiter sequence.
		// When it fails (does not successfully match in_sequence),
		//     begin points to the first character that is not processed
		// On success, the double/back quote is read in and it is skipped,
		//     so that begin points to the first character after the double/back quote.
		bool get_raw_string_closing_d_seq(unicode::code_point_string_view in_sequence, // delimiter sequence for reference
		                                  unicode::code_point_string&     out_sequence, // closing delimiter sequence we are reading
		                                  auto&                           begin,
		                                  auto                            end,
		                                  unicode::code_point_t           end_quote) const
		{
			for (::std::size_t index{}; begin != end; ++begin, ++index)
			{
				auto c = *begin;
				if (out_sequence.length() == in_sequence.length())
				{
					if (c == end_quote)
					{
						++begin; // skip the double quotation marks that are read
						return true;
					}
					else
					{
						return false;
					}
				}
				if (c != in_sequence[index])
				{
					return false;
				}
				out_sequence += c;
			}
			return false;
		}

		void process_hex_fp_errors(dfa_state_code c, ::std::string& num_seq, source_position pos)
		{
			using lex_ec = lexical_error_code;
			using enum dfa_state_code;
			using namespace ::std::string_literals;

			auto num_seq_to_ems = [&]() { return reinterpret_to_err_msg_str(num_seq); };
			switch (c)
			{
			case hex_fp_dec_part_with_quote:  // <<< ERROR STATE
			{
				using raw_err = raw_error_message_type::number_end_with_separator;
				using enum raw_error_message_type::number_type;
				post_err(pos, lex_ec::number_cannot_end_with_separator, raw_err{ num_seq_to_ems(), hex_floating });
				[[fallthrough]];
			}
			case hex_fp_dec_part_first_after_hex_with_dot: // <<< ERROR STATE
			case hex_fp_dec_part:          // <<< ERROR STATE
			case hex_fp_exp_sign_or_first: // <<< ERROR STATE
			case hex_fp_exp_first:         // <<< ERROR STATE
				post_err(pos, lex_ec::fp_hex_expect_exponent, to_raw_err_str(num_seq_to_ems()));
				switch (c)
				{
				case hex_fp_dec_part_first_after_hex_with_dot:
				case hex_fp_dec_part:
					num_seq += 'P';
					[[fallthrough]];
				case hex_fp_exp_sign_or_first:
				case hex_fp_exp_first:
					num_seq += '0';
					break;
				default:
					break;
				}
				break;
			case hex_fp_exp_with_quote:  // <<< ERROR STATE
			{
				using raw_err = raw_error_message_type::number_end_with_separator;
				using enum raw_error_message_type::number_type;
				post_err(pos, lex_ec::number_cannot_end_with_separator, raw_err{ num_seq_to_ems(), hex_floating });
				break;
			}
			case hex_fp_seq_start_with_0x_dot: // <<< ERROR STATE
				post_err(pos, lex_ec::fp_hex_expect_decimal_part, to_raw_err_str(num_seq_to_ems()));
				post_err(pos, lex_ec::fp_hex_expect_exponent, to_raw_err_str(num_seq_to_ems()));
				num_seq += "0P0"s;
				break;
			default:
				break;
			}
		}

		void process_string_like_error(dfa_state_code dfa_state_c, source_position position, error_msg_string err_s, unicode::code_point_string_view d_seq)
		{
			using lex_ec = lexical_error_code;
			using enum dfa_state_code;
			auto make_missing_d_seq_err = [&](bool is_raw_iden_s)
			{
				using miss_d_seq_msg = raw_error_message_type::missing_terminating_sequence;
				using unicode::code_convert;
				return miss_d_seq_msg
				{
					.content = ::std::move(err_s),
					.d_seq   = code_convert<error_msg_string>(d_seq),
					.is_raw_identifier_string = is_raw_iden_s
				};
			};
			switch (dfa_state_c) // ERROR STATES
			{
			case identifier_string_opened:
			case identifier_string:
				post_err(position, lex_ec::identifier_string_missing_terminating_character, to_raw_err_str(::std::move(err_s)));
				break;
			case string_opened:
			case string:
				post_err(position, lex_ec::string_missing_terminating_character, to_raw_err_str(::std::move(err_s)));
				break;
			case identifier_raw_string:
				post_err(position, lex_ec::identifier_raw_string_missing_terminating_sequence, make_missing_d_seq_err(true));
				break;
			case raw_string:
				post_err(position, lex_ec::raw_string_missing_terminating_sequence, make_missing_d_seq_err(false));
				break;
			default:
				break;
			}
		}

		void process_char_missing_quote_error(source_position position, error_msg_string err_s)
		{
			using lex_ec = lexical_error_code;
			post_err(position, lex_ec::character_missing_terminating_character, to_raw_err_str(::std::move(err_s)));
		}

		void process_dec_seq_error(const dfa_state_code code, source_position position, error_msg_string error_str)
		{
			using lex_ec = lexical_error_code;
			using enum dfa_state_code;
			using raw_err = raw_error_message_type::number_end_with_separator;
			if (code == dec_seq_start_with_0 || code == dec_seq_start_with_0_with_quote)
			{
				post_err(position, lex_ec::invalid_octal_number, to_raw_err_str(::std::move(error_str)));
			}
			else if (code == dec_seq_start_with_0_with_quote)
			{
				using enum raw_error_message_type::number_type;
				post_err(position, lex_ec::number_cannot_end_with_separator, raw_err{ ::std::move(error_str), oct_integer });
			}
			else if (code == dec_seq_with_quote)
			{
				using enum raw_error_message_type::number_type;
				post_err(position, lex_ec::number_cannot_end_with_separator, raw_err{ ::std::move(error_str), dec_integer });
			}
		}

		auto process_char_literal(::std::basic_string_view<char_t> src, const source_position& position) -> types::character<char_t>
		{
			using unicode::code_convert;
			auto cp_s = code_convert<unicode::code_point_string>(src);
			if (cp_s.size() == 1) [[likely]]
			{
				return types::character<char_t>(src.data(), src.size());
			}
			using lex_ec = lexical_error_code;
			using err_ch = raw_error_message_type::character_length_error;
			if (cp_s.size() == 0)
			{
				post_err(position, lex_ec::character_literal_length_is_zero, err_ch{ to_err_ms(src), cp_s.size()});
				using namespace literals::unicode_literals;
				return types::character<char_t>{};
			}
			else
			{
				post_err(position, lex_ec::character_literal_length_is_greater_than_one, err_ch{ to_err_ms(src), cp_s.size() });
				cp_s = cp_s.substr(0, 1);
				auto processed = code_convert<types::string<char_t>>(cp_s);
				return types::character<char_t>(processed.data(), processed.size());
			}
		}

		void process_from_chars_error(const ::std::from_chars_result            from_chars_r,
		                              const ::std::string_view                  src,
		                              const source_position                     pos,
		                              const raw_error_message_type::number_type lit_type,
		                              const bool                                for_escape)
		{
			using raw_err = raw_error_message_type::from_chars_error;
			auto [unresolved_char_ptr, err_code] = from_chars_r;

			assert(unresolved_char_ptr >= src.data() && unresolved_char_ptr <= (src.data() + src.size())
				&& "invalid arguments");

			auto num_seq_to_ems = [&]() { return reinterpret_to_err_msg_str(src); };
			using enum lexical_error_code;
			auto ptr_diff = unresolved_char_ptr - src.data();
			if (err_code == ::std::errc{})
			{
				if (unresolved_char_ptr != src.data() + src.size())
				{
					const auto ec = for_escape ? escape_error_from_chars_parsing_incomplete : number_from_chars_parsing_incomplete;
					post_err(pos, ec, raw_err{ num_seq_to_ems(), ptr_diff, err_code, lit_type });
				}
			}
			else
			{
				auto ec = lexical_error_code{};
				switch (err_code)
				{
				case ::std::errc::result_out_of_range:
					ec = for_escape ? escape_error_from_chars_errc_result_out_of_range : number_from_chars_errc_result_out_of_range;
					post_err(pos, ec, raw_err{ num_seq_to_ems(), ptr_diff, err_code, lit_type });
					break;
				case ::std::errc::invalid_argument:
					ec = for_escape ? escape_error_from_chars_errc_invalid_argument : number_from_chars_errc_invalid_argument;
					post_err(pos, ec, raw_err{ num_seq_to_ems(), ptr_diff, err_code, lit_type });
					break;
				default:
					ec = for_escape ? escape_error_from_chars_errc_other : number_from_chars_errc_other;
					post_err(pos, ec, raw_err{ num_seq_to_ems(), ptr_diff, err_code, lit_type });
					break;
				}
			}
		}

		auto get_pos() -> source_position
		{
			return func_pkg->position();
		}

		auto err_msg_gen(source_position pos, auto err_c, raw_error_message_variant raw_msg) -> error_msg_string
		{
			return func_pkg->generate_error_message(raw_error_message{ { err_c }, pos, ::std::move(raw_msg) });
		}

		void post_err(source_position pos, auto err_c, raw_error_message_variant&& raw_msg)
		{
			func_pkg->handle_error(error_message{ err_c, pos, err_msg_gen(pos, err_c, ::std::move(raw_msg)) });
		}
	private:
		function_package* func_pkg{};
	};
}

#endif
