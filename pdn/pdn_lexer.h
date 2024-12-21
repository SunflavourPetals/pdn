#ifndef PDN_Header_pdn_lexer
#define PDN_Header_pdn_lexer

#include <cstdint>
#include <string>
#include <memory>
#include <format>
#include <limits>
#include <utility>
#include <optional>
#include <concepts>
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
			unicode::code_point_string text{};            // source text
			unicode::code_point_string open_d_seq{};      // delimiter-sequence for raw string
			::std::string              number_sequence{}; // Unicode[U+0000, U+007f] -> ASCII -> form_chars
			::std::size_t              nested_block_comment_layer{};
			::std::size_t              number_delimiter_count{};
			source_position            position{ func_pkg->position() };
			auto                       dfa_state = dfa_state_objects::start_state();

			using unicode::code_convert;
			using lex_ec = lexical_error_code;
			using err_ms = error_msg_string;

			while (begin != end)
			{
				auto c = *begin;

				if (!unicode::is_scalar_value(c))
				{
					using raw_err = raw_error_message_type::not_unicode_scalar_value;
					post_err(func_pkg->position(), lex_ec::not_unicode_scalar_value, raw_err{ c });
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
						position = func_pkg->position();
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
					post_err(func_pkg->position(), lex_ec::unacceptable_character, code_convert<err_ms>(unicode::code_point_string_view{ &cp, 1 }));
					break;
				}
				case infinity:
				{
					using namespace unicode_literals;
					text = U"infinity"_us;
					new_dfa_state.state_code = dfa_state_objects::at_identifier.state_code;
					new_dfa_state.token_code = dfa_state_objects::at_identifier.token_code;
					break;
				}
				case identifier_string_with_LF:
				{
					new_dfa_state = dfa_state_objects::identifier_string_closed;
					post_err(position, lex_ec::identifier_string_terminated_by_LF, code_convert<err_ms>(text));
					goto label_update_dfa_state;
				}
				case string_with_LF:
				{
					new_dfa_state = dfa_state_objects::string_closed;
					post_err(position, lex_ec::string_literal_terminated_by_LF, code_convert<err_ms>(text));
					goto label_update_dfa_state;
				}
				case character_with_LF:
				{
					new_dfa_state = dfa_state_objects::character_closed;
					post_err(position, lex_ec::character_literal_terminated_by_LF, code_convert<err_ms>(text));
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
					text += c;
					break;
				case identifier_string_escape:
					++begin;
					new_dfa_state = dfa_state_objects::identifier_string;
					if (get_escape(c, begin, end, true))
					{
						text += c;
					}
					goto label_update_dfa_state;
				case string_escape:
					++begin;
					new_dfa_state = dfa_state_objects::string;
					if (get_escape(c, begin, end))
					{
						text += c;
					}
					goto label_update_dfa_state;
				case character_escape:
					++begin;
					new_dfa_state = dfa_state_objects::character;
					if (get_escape(c, begin, end))
					{
						text += c;
					}
					goto label_update_dfa_state;
				case raw_string_d_seq_opened:
					++begin;
					// @"d_seq()d_seq"
					//  ^ -> to first d sequence character or '('
					get_raw_string_opening_d_seq(open_d_seq, begin, end);
					new_dfa_state = dfa_state_objects::raw_string;
					goto label_update_dfa_state;
				case raw_string_received_CR:
					++begin;
					if (begin == end)
					{
						text += U'\r';
						new_dfa_state = dfa_state_objects::raw_string;
						goto label_update_dfa_state;
					}
					c = *begin;
					if (c != U'\n')
					{
						text += U'\r';
					}
					text += c;
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
						text += U')';
						text += close_d_seq;
						new_dfa_state = dfa_state_objects::raw_string;
					}
					goto label_update_dfa_state;
				}
				case identifier_raw_string_d_seq_opened:
					++begin;
					// @`d_seq()d_seq`
					//  ^ -> to first d sequence character or '('
					get_raw_string_opening_d_seq(open_d_seq, begin, end);
					new_dfa_state = dfa_state_objects::identifier_raw_string;
					goto label_update_dfa_state;
				case identifier_raw_string_received_CR:
					++begin;
					if (begin == end)
					{
						text += U'\r';
						new_dfa_state = dfa_state_objects::identifier_raw_string;
						goto label_update_dfa_state;
					}
					c = *begin;
					if (c != U'\n')
					{
						text += U'\r';
					}
					text += c;
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
						text += U')';
						text += close_d_seq;
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
					number_delimiter_count = 1;
					break;
				case dec_seq_with_quotes:
				case fp_dec_part_with_quotes:
				case fp_exp_with_quotes:
				case oct_seq_with_quotes:
				case dec_seq_start_with_0_with_quotes:
				case bin_seq_with_quotes:
				case hex_seq_with_quotes:
				case hex_fp_dec_part_with_quotes:
				case hex_fp_exp_with_quotes:
				{
					++number_delimiter_count;
					auto msg = reinterpret_to_err_msg_str(number_sequence);
					for (::std::size_t i{}; i < number_delimiter_count; ++i)
					{
						msg.push_back(u8'\'');
					}
					post_err(func_pkg->position(), lex_ec::more_than_one_separators_may_between_numbers, ::std::move(msg));
					break;
				}
				default:
					break;
				}
			label_move_iterator:
				++begin;
			label_update_dfa_state:
				dfa_state = new_dfa_state;
			}

		label_out_of_loop:

			auto num_seq_to_err_msg_str = [&]() { return reinterpret_to_err_msg_str(number_sequence); };

			auto from_chars_result_check = [&](::std::from_chars_result from_chars_r, err_ms extra) -> bool
			{
				auto [unresolved_char_ptr, err_code] = from_chars_r;

				using enum lexical_error_code;
				if (err_code == ::std::errc{})
				{
					if (unresolved_char_ptr != number_sequence.data() + number_sequence.size())
					{
						using namespace error_message_literals;
						auto msg =
							num_seq_to_err_msg_str() +
							u8" ("_em + extra + u8") o: '"_em +
							reinterpret_to_err_msg_str(number_sequence.data(), unresolved_char_ptr) +
							u8"', x: '"_em +
							reinterpret_to_err_msg_str(unresolved_char_ptr, number_sequence.data() + number_sequence.size()) +
							u8"'"_em;
						post_err(position, number_from_chars_parsing_incomplete, ::std::move(msg));
						return false;
					}
				}
				else
				{
					auto msg = num_seq_to_err_msg_str();
					switch (err_code)
					{
					case ::std::errc::result_out_of_range:
						post_err(position, number_from_chars_errc_result_out_of_range, ::std::move(msg));
						break;
					case ::std::errc::invalid_argument:
						post_err(position, number_from_chars_errc_invalid_argument, ::std::move(msg));
						break;
					default:
						post_err(position, number_from_chars_errc_other, ::std::move(msg));
						break;
					}
					return false;
				}

				return true;
			};

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
				// ERROR STATES ^^^
			case identifier:
			case identifier_string_closed:
			case string_closed:
			case raw_string_closed:
			case identifier_raw_string_closed:
				switch (dfa_state.state_code) // ERROR STATES
				{
				case identifier_string_opened:
				case identifier_string:
					post_err(position, lex_ec::identifier_string_missing_terminating_character, code_convert<err_ms>(text));
					break;
				case string_opened:
				case string:
					post_err(position, lex_ec::string_missing_terminating_character, code_convert<err_ms>(text));
					break;
				case identifier_raw_string:
					post_err(position, lex_ec::identifier_raw_string_missing_terminating_sequence, code_convert<err_ms>(text));
					break;
				case raw_string:
					post_err(position, lex_ec::raw_string_missing_terminating_sequence, code_convert<err_ms>(text));
					break;
				default:
					break;
				}
				result.value = make_proxy<types::string<char_t>>(text_code_convert<char_t>(text, position));
				break;
			case at_identifier:
				result.value = dev_util::at_iden_string_proxy{ text_code_convert<unicode::utf_8_code_unit_t>(text, position) };
				break;
			case character_opened: // <<< ERROR STATE
			case character:        // <<< ERROR STATE
				post_err(position, lex_ec::character_missing_terminating_character, code_convert<err_ms>(text));
				[[fallthrough]];
			case character_closed:
			{
				if (text.size() != 1)
				{
					if (text.size() == 0)
					{
						using namespace unicode_literals;
						using namespace error_message_literals;
						post_err(position, lex_ec::character_literal_length_is_zero, code_convert<err_ms>(text));
						text = U"\0"_us;
					}
					else
					{
						using namespace error_message_literals;
						post_err(position, lex_ec::character_literal_length_is_greater_than_one, code_convert<err_ms>(text));
						text = text.substr(0, 1);
					}
				}
				auto processed_text = text_code_convert<char_t>(text, position);
				result.value = types::character<char_t>(processed_text.data(), processed_text.size());
				break;
			}
			case dec_seq_start_with_0:             // <<< ERROR STATE
			case dec_seq_start_with_0_with_quote:  // <<< ERROR STATE
			case dec_seq_start_with_0_with_quotes: // <<< ERROR STATE
			case dec_seq_with_quote:               // <<< ERROR STATE
			case dec_seq_with_quotes:              // <<< ERROR STATE
				switch (dfa_state.state_code)
				{
				case dec_seq_start_with_0:
				case dec_seq_start_with_0_with_quote:
				case dec_seq_start_with_0_with_quotes:
					post_err(position, lex_ec::invalid_octal_number, num_seq_to_err_msg_str());
					break;
				default:
					break;
				}
				switch (dfa_state.state_code)
				{
				case dec_seq_start_with_0_with_quote:
				case dec_seq_start_with_0_with_quotes:
				case dec_seq_with_quote:
				case dec_seq_with_quotes:
					post_err(position, lex_ec::number_cannot_end_with_separator, num_seq_to_err_msg_str() + u8"'"_em);
					break;
				default:
					break;
				}
				[[fallthrough]];
			case dec_seq:
			{
				::std::uint64_t integer_value{};
				const auto& n_seq = number_sequence;
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 10);
				to_appropriate_int_type(result.value, integer_value);
				from_chars_result_check(from_chars_r, u8"decimal integer"_em);
			}
			break;
			case fp_exp_sign_or_first:    // <<< ERROR STATE
			case fp_exp_first:            // <<< ERROR STATE
			case fp_dec_part_with_quote:  // <<< ERROR STATE
			case fp_dec_part_with_quotes: // <<< ERROR STATE
			case fp_exp_with_quote:       // <<< ERROR STATE
			case fp_exp_with_quotes:      // <<< ERROR STATE
				switch (dfa_state.state_code)
				{
				case fp_exp_sign_or_first:
				case fp_exp_first:
					post_err(position, lex_ec::fp_dec_expect_exponent, num_seq_to_err_msg_str());
					number_sequence += '0'; // add 0 for exponent
					break;
				default:
					break;
				}
				switch (dfa_state.state_code)
				{
				case fp_dec_part_with_quote:
				case fp_dec_part_with_quotes:
				case fp_exp_with_quote:
				case fp_exp_with_quotes:
					post_err(position, lex_ec::number_cannot_end_with_separator, num_seq_to_err_msg_str() + u8"'"_em);
					break;
				default:
					break;
				}
				[[fallthrough]];
			case fp_dec_part_first_after_dec_with_dot:
			case fp_dec_part:
			case fp_exp:
			{
				types::f64 fp_value{};
				const auto& n_seq = number_sequence;
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), fp_value);
				result.value = fp_value;
				from_chars_result_check(from_chars_r, u8"decimal floating point"_em);
			}
			break;
			case oct_seq_with_quote:  // <<< ERROR STATE
			case oct_seq_with_quotes: // <<< ERROR STATE
				post_err(position, lex_ec::number_cannot_end_with_separator, num_seq_to_err_msg_str() + u8"'"_em);
				[[fallthrough]];
			case zero:
			case oct_seq:
			{
				::std::uint64_t integer_value{};
				const auto& n_seq = number_sequence;
				auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 8);
				to_appropriate_int_type(result.value, integer_value);
				from_chars_result_check(from_chars_r, u8"octal integer"_em);
			}
			break;
			case bin_seq_first:       // <<< ERROR STATE
			case bin_seq_with_quote:  // <<< ERROR STATE
			case bin_seq_with_quotes: // <<< ERROR STATE
				if (dfa_state.state_code == bin_seq_first)
				{
					post_err(position, lex_ec::bin_prefix_expect_bin_number_sequence, num_seq_to_err_msg_str());
					number_sequence += '0';
				}
				switch (dfa_state.state_code)
				{
				case bin_seq_with_quote:
				case bin_seq_with_quotes:
					post_err(position, lex_ec::number_cannot_end_with_separator, num_seq_to_err_msg_str() + u8"'"_em);
					break;
				default:
					break;
				}
				[[fallthrough]];
			case bin_seq:
				if (check_no_0b_prefix(number_sequence))
				{
					to_appropriate_int_type(result.value, types::u64{ 0 });
					post_err(position, lex_ec::inner_error_binary_integer_without_0b_prefix, num_seq_to_err_msg_str());
				}
				else // have prefix 0b or 0B
				{
					types::u64 integer_value{};
					::std::string_view n_seq(number_sequence.begin() + 2, number_sequence.end());
					auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 2);
					to_appropriate_int_type(result.value, integer_value);
					from_chars_result_check(from_chars_r, u8"binary integer"_em);
				}
				break;
			case hex_seq_first:       // <<< ERROR STATE
			case hex_seq_with_quote:  // <<< ERROR STATE
			case hex_seq_with_quotes: // <<< ERROR STATE
				if (dfa_state.state_code == hex_seq_first)
				{
					post_err(position, lex_ec::hex_prefix_expect_hex_number_sequence, num_seq_to_err_msg_str());
					number_sequence += '0';
				}
				switch (dfa_state.state_code)
				{
				case hex_seq_with_quote:
				case hex_seq_with_quotes:
					post_err(position, lex_ec::number_cannot_end_with_separator, num_seq_to_err_msg_str() + u8"'"_em);
					break;
				default:
					break;
				}
				[[fallthrough]];
			case hex_seq:
				if (check_no_0x_prefix(number_sequence))
				{
					to_appropriate_int_type(result.value, types::u64{ 0 });
					post_err(position, lex_ec::inner_error_hexadecimal_integer_without_0x_prefix, num_seq_to_err_msg_str());
				}
				else // have prefix 0x or 0X
				{
					types::u64 integer_value{};
					::std::string_view n_seq(number_sequence.begin() + 2, number_sequence.end());
					auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), integer_value, 16);
					to_appropriate_int_type(result.value, integer_value);
					from_chars_result_check(from_chars_r, u8"hexadecimal integer"_em);
				}
				break;
			case hex_fp_dec_part_with_quote:               // <<< ERROR STATE
			case hex_fp_dec_part_with_quotes:              // <<< ERROR STATE
			case hex_fp_dec_part_first_after_hex_with_dot: // <<< ERROR STATE
			case hex_fp_dec_part:                          // <<< ERROR STATE
			case hex_fp_exp_sign_or_first:                 // <<< ERROR STATE
			case hex_fp_exp_first:                         // <<< ERROR STATE
			case hex_fp_exp_with_quote:                    // <<< ERROR STATE
			case hex_fp_exp_with_quotes:                   // <<< ERROR STATE
			case hex_fp_seq_start_with_0x_dot:             // <<< ERROR STATE
				process_hex_fp_errors(dfa_state.state_code, number_sequence, position);
				[[fallthrough]];
			case hex_fp_exp:
				if (check_no_0x_prefix(number_sequence))
				{
					result.value = types::f64{};
					post_err(position, lex_ec::inner_error_hexadecimal_floating_point_without_0x_prefix, num_seq_to_err_msg_str());
				}
				else // have prefix 0x or 0X
				{
					types::f64 fp_value{};
					::std::string_view n_seq(number_sequence.begin() + 2, number_sequence.end());
					auto from_chars_r = ::std::from_chars(n_seq.data(), n_seq.data() + n_seq.size(), fp_value, ::std::chars_format::hex);
					result.value = fp_value;
					from_chars_result_check(from_chars_r, u8"hexadecimal floating point"_em);
				}
				break;
			default:
				// if dfa state is not final-state，then should process error in non-default case
				if (!dfa_state.is_final())
				{
					post_err(position, lex_ec::inner_error_make_non_final_dfa_state_to_token, {});
				}
				break;
			}

			return result;
		}

		lexer(function_package& function_pkg) : func_pkg{ &function_pkg } {}
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
			constexpr auto cppint_max = static_cast<types::u64>(::std::numeric_limits<types::cppint>::max());
			constexpr auto i8_max     = static_cast<types::u64>(::std::numeric_limits<types::i8>::max());
			constexpr auto i16_max    = static_cast<types::u64>(::std::numeric_limits<types::i16>::max());
			constexpr auto i32_max    = static_cast<types::u64>(::std::numeric_limits<types::i32>::max());
			constexpr auto i64_max    = static_cast<types::u64>(::std::numeric_limits<types::i64>::max());
			if (val <= cppint_max)
			{
				target = static_cast<types::cppint>(val);
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
		static bool check_no_0x_prefix(const ::std::string& seq)
		{
			return seq.size() < 2 || seq[0] != '0' || (seq[1] != 'x' && seq[1] != 'X');
		}

		bool get_escape(auto& oc,    // out param, the result of get escape
		                auto& begin, // pointing first char witch after '\'
		                auto  end,
		                bool  enable_escape_back_quote = false) // is enable escape sequence \`
		{
			static_assert(sizeof(::std::uint32_t) == sizeof(unicode::code_point_t));
			using escape_value_t = ::std::uint32_t;

			source_position position{ func_pkg->position() };

			using enum lexical_error_code;
			if (begin == end)
			{
				post_err(position, escape_error_unknown_escape_sequence, u8"\\"_em);
				return false;
			}

			::std::string sequence{};

			auto seq_to_err_msg_str = [&]() { return reinterpret_to_err_msg_str(sequence); };

			auto from_chars_error_check = [&](
				::std::from_chars_result from_chars_r,
				error_msg_string escape_sign, // u -> \u x -> \x 
				escape_value_t parse_val,
				bool is_with_curly_brackets) -> bool
			{
				auto [unresolved_char_ptr, errc] = from_chars_r;
				auto l_bracket = is_with_curly_brackets ? u8"{"_em : u8""_em;
				auto r_bracket = is_with_curly_brackets ? u8"}"_em : u8""_em;
				if (errc == ::std::errc{})
				{
					if (unresolved_char_ptr != sequence.data() + sequence.size())
					{
						auto msg = u8"\\"_em
							.append(::std::move(escape_sign))
							.append(l_bracket)
							.append(seq_to_err_msg_str())
							.append(r_bracket)
							.append(u8" o: '"_em)
							.append(reinterpret_to_err_msg_str(sequence.data(), unresolved_char_ptr))
							.append(u8"', x: '"_em)
							.append(reinterpret_to_err_msg_str(unresolved_char_ptr, sequence.data() + sequence.size()))
							.append(u8"'"_em);
						post_err(position, escape_error_from_chars_parsing_incomplete, ::std::move(msg));
						return false;
					}
				}
				else
				{
					// If parsed correctly, the sequence contains only the numbers 0-7 and 0-f,
					// but there may be result_out_of_range
					auto msg = u8"\\"_em + escape_sign + l_bracket + seq_to_err_msg_str() + r_bracket;
					switch (errc)
					{
					case ::std::errc::result_out_of_range:
						post_err(position, escape_error_from_chars_errc_result_out_of_range, ::std::move(msg));
						break;
					case ::std::errc::invalid_argument:
						post_err(position, escape_error_from_chars_errc_invalid_argument, ::std::move(msg));
						break;
					default:
						post_err(position, escape_error_from_chars_errc_other, ::std::move(msg));
						break;
					}
					return false;
				}
				if (!unicode::is_scalar_value(static_cast<unicode::code_point_t>(parse_val)))
				{
					// Not a Unicode scalar will result in a decoding/encoding error
					post_err(position,
						escape_error_not_unicode_scalar_value,
						u8"\\"_em + escape_sign + l_bracket + seq_to_err_msg_str() + r_bracket);
					return false;
				}
				return true;
			};

			auto c = *begin;

			switch (c)
			{
				// simple escape sequence
			case U'`':
				if (enable_escape_back_quote)
				{
					oc = U'`';
					goto simple_escape;
				}
				break;
			case U'\'':
				oc = U'\'';
				goto simple_escape;
			case U'\"':
				oc = U'\"';
				goto simple_escape;
			case U'\?':
				oc = U'\?';
				goto simple_escape;
			case U'\\':
				oc = U'\\';
				goto simple_escape;
			case U'a':
				oc = U'\a';
				goto simple_escape;
			case U'b':
				oc = U'\b';
				goto simple_escape;
			case U'f':
				oc = U'\f';
				goto simple_escape;
			case U'n':
				oc = U'\n';
				goto simple_escape;
			case U'r':
				oc = U'\r';
				goto simple_escape;
			case U't':
				oc = U'\t';
				goto simple_escape;
			case U'v':
				oc = U'\v';
				goto simple_escape;

			simple_escape:
				++begin;
				return true;

			case U'o': // \o{n...}
			{
				++begin; // -> {

				if (begin == end || *begin != U'{')
				{
					post_err(position, escape_error_o_not_followed_by_left_brackets, u8"\\o"_em);
					return false;
				}
				c = *begin;
				++begin; // -> n (oct)

				get_numeric_escape_sequence(sequence, begin, end, lexer_utility::is_oct);

				if (begin == end || *begin != U'}')
				{
					post_err(position, escape_error_o_not_terminated_with_right_brackets, u8"\\o{"_em + seq_to_err_msg_str());
					return false;
				}
				c = *begin;
				++begin;

				if (sequence.size() == 0)
				{
					// The contents of the curly braces cannot be empty
					post_err(position, escape_error_o_empty_delimited_escape_sequence, u8"\\o{}"_em);
					return false;
				}

				escape_value_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 8);

				if (!from_chars_error_check(from_chars_r, u8"o"_em, parse_val, true))
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
					post_err(position, escape_error_x_used_with_no_following_hex_digits, u8"\\x"_em);
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
						post_err(position, escape_error_x_not_terminated_with_right_brackets, u8"\\x{"_em + seq_to_err_msg_str());
						return false;
					}
					c = *begin;
					++begin;
				}

				if (sequence.size() == 0)
				{
					// length of hexadecimal number sequence cannot be 0 in \x... and \x{...}
					auto msg = u8"\\x"_em;
					if (is_with_curly_brackets)
					{
						msg += u8"{}"_em;
					}
					post_err(position, escape_error_x_empty_delimited_escape_sequence, ::std::move(msg));
					return false;
				}

				::std::uint32_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 16);
				if (!from_chars_error_check(from_chars_r, u8"x"_em, parse_val, is_with_curly_brackets))
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
					post_err(position, escape_error_u_incomplete_universal_character_name, u8"\\u"_em);
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
						post_err(position, escape_error_u_not_terminated_with_right_brackets, u8"\\u{"_em + seq_to_err_msg_str());
						return false;
					}
					c = *begin;
					++begin;

					if (sequence.size() == 0)
					{
						// length of hexadecimal number sequence cannot be 0 in \u{n...}
						post_err(position, escape_error_u_empty_delimited_escape_sequence, u8"\\u{}"_em);
						return false;
					}
				}
				else
				{
					if (sequence.size() != 4)
					{
						// length of hexadecimal number sequence cannot be number other than 4 in \unnnn
						post_err(position, escape_error_u_incomplete_universal_character_name, u8"\\u"_em + seq_to_err_msg_str());
						return false;
					}
				}

				::std::uint32_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 16);

				if (!from_chars_error_check(from_chars_r, u8"u"_em, parse_val, is_with_curly_brackets))
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
					post_err(position, escape_error_U_incomplete_universal_character_name, u8"\\U"_em);
					return false;
				}

				c = *begin;

				get_numeric_escape_sequence(8, sequence, begin, end, lexer_utility::is_hex);

				if (sequence.size() != 8)
				{
					// length of hexadecimal number sequence cannot be number other than 8 in \Unnnnnnnn
					post_err(position, escape_error_U_incomplete_universal_character_name, u8"\\U"_em + seq_to_err_msg_str());
					return false;
				}

				::std::uint32_t parse_val{};
				auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 16);

				if (!from_chars_error_check(from_chars_r, u8"U"_em, parse_val, false))
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

					if (sequence.size() == 0) // sequence.size() == 0 never be true
					{
						post_err(position, escape_error_o_empty_delimited_escape_sequence, u8"\\"_em);
						return false;
						// length of octal number sequence cannot be 0
					}

					::std::uint32_t parse_val{};
					auto from_chars_r = ::std::from_chars(sequence.data(), sequence.data() + sequence.size(), parse_val, 8);

					if (!from_chars_error_check(from_chars_r, u8""_em, parse_val, false))
					{
						return false;
					}
					oc = parse_val;
					return true;
				}
				break;
			}
			auto cp = unicode::code_point_t(c);
			auto err_msg = u8"\\"_em + unicode::code_convert<error_msg_string>(unicode::code_point_string_view{ &cp, 1 });
			post_err(position, escape_error_unknown_escape_sequence, ::std::move(err_msg));
			return false;
		}

		void get_numeric_escape_sequence(::std::string& sequence,
		                                          auto& begin,
		                                          auto  end,
		  ::std::predicate<unicode::code_point_t> auto  is_valid_char)
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
		  ::std::predicate<unicode::code_point_t> auto  is_valid_char) -> ::std::size_t
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
		bool get_raw_string_opening_d_seq(unicode::code_point_string& out_sequence, auto& begin, auto end)
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

			using err_ms = error_msg_string;
			using unicode::code_convert;

			source_position position{ func_pkg->position() };
			bool invalid_character_in_raw_string_delimiter{ false };

			using enum lexical_error_code;

			while (begin != end)
			{
				auto c = *begin;
				if (c == U'(')
				{
					++begin;
					bool is_success{ true };
					if (invalid_character_in_raw_string_delimiter)
					{
						is_success = false;
						post_err(position, d_seq_error_invalid_character_in_raw_string_delimiter, code_convert<err_ms>(out_sequence));
					}
					if (out_sequence.length() > 16)
					{
						is_success = false;
						post_err(position, d_seq_error_raw_string_delimiter_longer_than_16_characters, code_convert<err_ms>(out_sequence));
					}
					return is_success;
				}
				else
				{
					if (!lexer_utility::is_allowed_in_raw_string_d_sequence(c))
					{
						invalid_character_in_raw_string_delimiter = true;
					}
					out_sequence += c;
					++begin;
				}
			}

			using namespace literals::unicode_literals;
			auto sequence_slice = (out_sequence.size() > 20) ? out_sequence.substr(0, 20) + U"..."_us : out_sequence;
			post_err(position, d_seq_error_cannot_find_end_sign, code_convert<err_ms>(sequence_slice));
			return false;
		}

		// When ')' is detected, ++begin and then call this function.
		// Returns true when the sequence we read matches the referenced delimiter sequence.
		// When it fails (does not successfully match in_sequence),
		//     begin points to the first character that is not processed
		// On success, the double/back quote is read in and it is skipped,
		//     so that begin points to the first character after the double/back quote.
		bool get_raw_string_closing_d_seq(const unicode::code_point_string_view in_sequence, // delimiter sequence for reference
		                                  unicode::code_point_string&           out_sequence, // closing delimiter sequence we are reading
		                                  auto&                                 begin,
		                                  auto                                  end,
		                                  unicode::code_point_t                 end_quote)
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

		template <typename target_char_t>
		auto text_code_convert(const unicode::code_point_string& text, source_position position) -> types::string<target_char_t>
		{
			using decision = unicode::convert_decision<unicode::code_point_string, types::string<target_char_t>>;
			using encode_error_type = typename decision::encode_result::error_type;
			using decode_error_type = typename decision::decode_result::error_type;
			using unicode::code_convert;
			using err_ms = error_msg_string;

			encode_error_type encode_error{};
			decode_error_type decode_error{};
			::std::size_t encode_error_pos{};
			::std::size_t decode_error_pos{};

			auto processed_text = code_convert<types::string<target_char_t>>(
				text,
				[&](auto r, auto p) { decode_error = r.error(); decode_error_pos = p; return true; },
				[&](auto r, auto p) { encode_error = r.error(); encode_error_pos = p; return true; });

			// Lexer doesn't allow accepting non-Unicode scalar values,
			// so if code_convert is implemented correctly, no errors will be reported here
			// (Unicode scalar values can be converted between UTF-8 UTF-16 UTF-32 without conversion failure)
			// If a non-Unicode scalar value is received, the error should be reported when lexer receives it,
			// not when calling cond_convert.
			// But the following error handling is still useful for this system

			if (encode_error != encode_error_type{})
			{
				using namespace literals;
				auto value_str = ::std::format("0x{:08X}", static_cast<::std::uint32_t>(text[encode_error_pos]));
				auto msg = u8"encode error when code convert -> '"_em
					.append(code_convert<err_ms>(text.substr(0, encode_error_pos)))
					.append(u8">>>"_em)
					.append(reinterpret_to_err_msg_str(value_str))
					.append(u8"<<<"_em)
					.append(code_convert<err_ms>(text.substr(encode_error_pos + 1)))
					.append(u8"'"_em);
				post_err(position, encode_error, ::std::move(msg));
			}
			if (decode_error != decode_error_type{})
			{
				using namespace literals;
				auto value_str = ::std::format("0x{:08X}", static_cast<::std::uint32_t>(text[decode_error_pos]));
				auto msg = u8"decode error when code convert -> '"_em
					.append(code_convert<err_ms>(text.substr(0, decode_error_pos)))
					.append(u8">>>"_em)
					.append(reinterpret_to_err_msg_str(value_str))
					.append(u8"<<<"_em)
					.append(code_convert<err_ms>(text.substr(decode_error_pos + 1)))
					.append(u8"'"_em);
				post_err(position, decode_error, ::std::move(msg));
			}
			return processed_text;
		}

		void process_hex_fp_errors(dfa_state_code c, ::std::string& num_seq, source_position pos)
		{
			using lex_ec = lexical_error_code;
			using enum dfa_state_code;
			using namespace ::std::string_literals;

			switch (c)
			{
			case hex_fp_dec_part_with_quote:  // <<< ERROR STATE
			case hex_fp_dec_part_with_quotes: // <<< ERROR STATE
				post_err(pos,
					lex_ec::number_cannot_end_with_separator,
					reinterpret_to_err_msg_str(num_seq) + u8"'"_em);
				[[fallthrough]];
			case hex_fp_dec_part_first_after_hex_with_dot: // <<< ERROR STATE
			case hex_fp_dec_part:          // <<< ERROR STATE
			case hex_fp_exp_sign_or_first: // <<< ERROR STATE
			case hex_fp_exp_first:         // <<< ERROR STATE
				post_err(pos,
					lex_ec::fp_hex_expect_exponent,
					reinterpret_to_err_msg_str(num_seq));
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
			case hex_fp_exp_with_quotes: // <<< ERROR STATE
				post_err(pos,
					lex_ec::number_cannot_end_with_separator,
					reinterpret_to_err_msg_str(num_seq) + u8"'"_em);
				break;
			case hex_fp_seq_start_with_0x_dot: // <<< ERROR STATE
				post_err(pos,
					lex_ec::fp_hex_expect_decimal_part,
					reinterpret_to_err_msg_str(num_seq));
				post_err(pos,
					lex_ec::fp_hex_expect_exponent,
					reinterpret_to_err_msg_str(num_seq));
				num_seq += "0P0"s;
				break;
			default:
				break;
			}
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
