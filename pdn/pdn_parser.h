#ifndef PDN_Header_pdn_parser
#define PDN_Header_pdn_parser

#include <cassert>
#include <utility>
#include <cstdint>
#include <variant>
#include <limits>
#include <concepts>
#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_code_convert.h"

#include "pdn_types.h"
#include "pdn_entity.h"

#include "pdn_token_code.h"
#include "pdn_token.h"
#include "pdn_token_convert.h"
#include "pdn_token_code_to_error_msg_string.h"

#include "pdn_error_string.h"
#include "pdn_syntax_error_code.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_type_code.h"
#include "pdn_type_code_to_type.h"
#include "pdn_type_to_type_code.h"
#include "pdn_type_code_to_error_msg_string.h"

#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_type_generator_concept.h"
#include "pdn_constant_generator_concept.h"

#include "pdn_make_slashes_string.h"
#include "pdn_parser_utility.h"

namespace pdn::concepts
{
	template <typename type, typename char_t>
	concept function_package_for_parser
		 = concepts::error_handler<type>
		&& concepts::error_message_generator<type>
		&& concepts::type_generator<type, char_t>
		&& concepts::constant_generator<type, char_t>;
}

namespace pdn
{
	template <unicode::concepts::code_unit char_t, concepts::function_package_for_parser<char_t> function_package>
	class parser
	{
	public:
		using char_type = char_t;
		using entity_type = entity<char_type>;
	private:
		using syn_ec = syntax_error_code;
	public:
		void parse(concepts::token_iterator<char_t> auto begin, auto end, types::object<char_t>& o)
		{
			using enum pdn_token_code;
			parse_start(begin, end, o);
			assert(tk.code == eof && "parse terminated");
		}
		auto parse(concepts::token_iterator<char_t> auto begin, auto end) -> entity_type
		{
			auto o = types::object<char_t>{};
			parse(begin, end, o);
			return make_proxy<types::object<char_t>>(::std::move(o));
		}
		explicit parser(function_package& function_pkg) : func_pkg{ &function_pkg } {}
	private:
		void parse_start(auto& begin, auto end, types::object<char_t>& o)
		{
			using enum pdn_token_code;
			using err_ms = error_msg_string;
			using unicode::code_convert;
			using parser_utility::is_expr_first;

			for (update_token(begin, end); tk.code != eof; )
			{
				if (tk.code == identifier)
				{
					using string_pr = proxy<types::string<char_t>>;
					auto iden_p = ::std::get<string_pr>(::std::move(tk.value));
					if (auto it = o.find(*iden_p); it == o.end())
					{
						o[::std::move(*iden_p)] = parse_decl(begin, end);
					}
					else
					{
						using raw_err_iden = raw_error_message_type::identifier;
						post_err(tk.position, syn_ec::entity_redefine, raw_err_iden{ code_convert<err_ms>(*iden_p) });
						parse_decl(begin, end);
					}
				}
				else
				{
					using parser_utility::to_raw_error_token;
					if (tk.code == semicolon)
					{
						update_token(begin, end);
					}
					else if (is_expr_first(tk.code))
					{
						post_err(tk.position, syn_ec::expect_entity_name, to_raw_error_token(tk));
						parse_expr(begin, end);
					}
					else
					{
						post_err(tk.position, syn_ec::expect_definition_of_named_entity, to_raw_error_token(tk));
						update_token(begin, end);
					}
				}
			}
		}

		auto parse_decl(auto& begin, auto end) -> entity_type
		{
			// ... iden CURPOS ...
			// expect : | expr

			using enum pdn_token_code;
			using parser_utility::is_expr_first;
			using parser_utility::default_entity_value;

			type_code type_c = type_code::unknown;
			auto type_pos = tk.position;

			update_token(begin, end);
			if (tk.code == colon)
			{
				update_token(begin, end);
				type_pos = tk.position;

				if (tk.code == identifier)
				{
					type_c = parse_type_spec();
					update_token(begin, end);
					// ... iden : typename CURPOS(expect expr) ...
				}
				// ... iden : CURPOS(expect expr) ...
			}

			if (!is_expr_first(tk.code))
			{
				post_err(tk.position, syn_ec::expect_expression, parser_utility::to_raw_error_token(tk));
				return default_entity_value<char_t>(type_c);
			}

			auto e = parse_expr(begin, end);
			// ... iden [colon [typename] ] expr CURPOS ...

			if (type_c != type_code::unknown)
			{
				e = entity_cast(::std::move(e), type_c, type_pos);
			}

			// decl end

			return e;
		}

		auto parse_type_spec() -> type_code
		{
			//    ... iden : CURPOS expr ...
			// or ... CURPOS : expr ...
			// expect typename(identifier)

			using enum pdn_token_code;
			using err_ms = error_msg_string;
			using unicode::code_convert;

			assert(tk.code == identifier && "generate type from non-identifier");
			using string_pr = proxy<types::string<char_t>>;
			auto type_c = type_gen(*::std::get<string_pr>(tk.value));
			if (type_c == type_code::unknown)
			{
				using raw_err_iden = raw_error_message_type::identifier;
				post_err(tk.position, syn_ec::unknown_type, raw_err_iden{ code_convert<err_ms>(*::std::get<string_pr>(tk.value)) });
			}
			return type_c;
		}

		auto parse_expr(auto& begin, auto end) -> entity_type
		{
			// expect
			//     - ...
			//     + ...
			//     literals
			//         integer | floating-points | string* | character
			//     @name
			//     { ... }
			//     [ ... ]

			using enum pdn_token_code;

			auto unary_rec = process_unary_operation(begin, end);

			if (!parser_utility::is_expr_first(tk.code))
			{
				post_err(tk.position, syn_ec::expect_expression, parser_utility::to_raw_error_token(tk));
				return types::cppint{};
			}

			auto result = parse_expr_without_unary(begin, end);

			if (!unary_rec.has_sign) return result;

			::std::visit([&](auto& arg)
			{
				using raw_err_unary_op = raw_error_message_type::unary_operation;
				using arg_t = ::std::decay_t<decltype(arg)>;
				using types::concepts::pdn_sint;
				using types::concepts::pdn_uint;
				using types::concepts::pdn_fp;
				if constexpr (pdn_fp<arg_t> || pdn_sint<arg_t>) // f32, f64, i8, i16, i32, i64
				{
					if (unary_rec.negative_sign_count % 2)
					{
						arg = -arg;
					}
				}
				else if constexpr (pdn_uint<arg_t>) // unary operator- on u8, u16, u32, u64 is illegal
				{
					if (unary_rec.negative_sign_count)
					{
						constexpr auto operand_type = type_to_type_code_v<arg_t, char_t>;
						post_err(unary_rec.last_negative_sign_pos,
						         syn_ec::invalid_unary_operation,
						         raw_err_unary_op{ { arg }, operand_type, true });
					}
				}
				else // boolean, character, string, list, object
				{
					using types::concepts::pdn_bool;
					auto err_tkv = token_value_variant<error_msg_char>{};
					if constexpr (pdn_bool<arg_t>)
					{
						err_tkv = arg;
					}
					else if constexpr (::std::same_as<arg_t, proxy<types::character<char_t>>>)
					{
						auto s = unicode::code_convert<error_msg_string>(arg.to_string_view());
						err_tkv = types::character<error_msg_char>{ s.data(), s.size() };
					}
					else if constexpr (::std::same_as<arg_t, proxy<types::string<char_t>>>)
					{
						using err_s_t = types::string<error_msg_char>;
						err_tkv = make_proxy<err_s_t>(unicode::code_convert<err_s_t>(*arg));
					}
					constexpr auto operand_type = type_to_type_code_v<remove_proxy_t<arg_t>, char_t>;
					post_err(unary_rec.is_last_sign_negative ? unary_rec.last_negative_sign_pos : unary_rec.last_positive_sign_pos,
					         syn_ec::invalid_unary_operation,
					         raw_err_unary_op{ err_tkv, operand_type, unary_rec.is_last_sign_negative });
				}
			}, result);

			return result;
		}

		auto parse_expr_without_unary(auto& begin, auto end) -> entity_type
		{
			// expect
			//     literals
			//         integer | floating-points | string* | character
			//     @name
			//     { ... }
			//     [ ... ]

			using enum pdn_token_code;
			switch (tk.code)
			{
			case literal_string:
			{
				using string_t = types::string<char_t>;
				using string_pr = proxy<string_t>;
				auto start_pos = tk.position;
				auto concat_s = string_t{};
				while (tk.code == literal_string) // concatenation
				{
					concat_s += *::std::get<string_pr>(tk.value);
					update_token(begin, end);
				}
				return token_value_to_entity(token<char_t>{ start_pos, literal_string, make_proxy<string_t>(::std::move(concat_s)) });
			}
			case literal_boolean:
			case literal_character:
			case literal_floating_point:
			case literal_integer:
			case at_identifier:
			{
				auto result = token_value_to_entity(::std::move(tk));
				update_token(begin, end);
				return result;
			}
			case left_brackets:
			{
				auto left_brackets_pos = tk.position;
				update_token(begin, end);
				return parse_list_expr(begin, end, left_brackets_pos);
			}
			case left_curly_brackets:
			{
				auto left_curly_brackets_pos = tk.position;
				update_token(begin, end);
				return parse_object_expr(begin, end, left_curly_brackets_pos);
			}
			default:
				post_err(tk.position, syn_ec::expect_expression, parser_utility::to_raw_error_token(tk));
				return types::cppint{};
			}
		}

		auto process_unary_operation(auto& begin, auto end) -> parser_utility::unary_record
		{
			using parser_utility::is_unary_operator;
			using parser_utility::is_negative_sign;

			auto result = parser_utility::unary_record{ is_unary_operator(tk.code) };

			for (; is_unary_operator(tk.code); update_token(begin, end))
			{
				result.is_last_sign_negative = is_negative_sign(tk.code);
				if (result.is_last_sign_negative)
				{
					result.last_negative_sign_pos = tk.position;
					++result.negative_sign_count;
				}
				else
				{
					result.last_positive_sign_pos = tk.position;
				}
			}

			return result;
		}
		
		auto parse_list_expr(auto& begin, auto end, source_position left_brackets_pos) -> entity_type
		{
			// ... [ CURRPOS ...

			using parser_utility::is_list_element_first;

			auto result = types::list<char_t>{};

			for (bool with_comma{ true }; tk.code != pdn_token_code::right_brackets; )
			{
				using parser_utility::to_raw_error_token;
				if (tk.code == pdn_token_code::eof)
				{
					post_err(left_brackets_pos, syn_ec::missing_right_brackets, {});
					return make_proxy<types::list<char_t>>(::std::move(result));
				}
				if (is_list_element_first(tk.code))
				{
					if (!with_comma)
					{
						post_err(tk.position, syn_ec::expect_comma, to_raw_error_token(tk));
					}
					result.push_back(parse_list_element(begin, end, with_comma));
				}
				else
				{
					post_err(tk.position, syn_ec::expect_definition_of_list_element, to_raw_error_token(tk));
					update_token(begin, end);
				}
			}

			// to ... [ ... ] CURRPOS
			update_token(begin, end);

			return make_proxy<types::list<char_t>>(::std::move(result));
		}

		auto parse_list_element(auto& begin, auto end, bool& with_comma) -> entity_type
		{
			// ... iden [colon [typename] ] [ ... ] CURPOS ...

			auto type_c = type_code::unknown;
			auto type_pos = tk.position;

			if (tk.code == pdn_token_code::identifier)
			{
				type_c = parse_type_spec();
				update_token(begin, end);
				if (tk.code == pdn_token_code::colon)
				{
					update_token(begin, end);
				}
				else
				{
					post_err(tk.position, syn_ec::expect_colon, parser_utility::to_raw_error_token(tk));
				}
			}

			entity_type e{ parse_expr(begin, end) };
			// to ... [ (element,)* element CURRPOS ...

			if (tk.code == pdn_token_code::comma)
			{
				update_token(begin, end);
				with_comma = true;
			}
			else
			{
				with_comma = false;
			}

			if (type_c != type_code::unknown)
			{
				e = entity_cast(::std::move(e), type_c, type_pos);
			}

			return e;
		}

		auto parse_object_expr(auto& begin, auto end, source_position left_curly_brackets_pos) -> entity_type
		{
			// ... { CURRPOS ...

			using err_ms = error_msg_string;
			using unicode::code_convert;
			using parser_utility::is_expr_first;

			auto result = types::object<char_t>{};

			while (tk.code != pdn_token_code::right_curly_brackets)
			{
				using parser_utility::to_raw_error_token;
				if (tk.code == pdn_token_code::identifier)
				{
					using string_pr = proxy<types::string<char_t>>;
					auto iden_p = ::std::get<string_pr>(::std::move(tk.value));
					if (auto it = result.find(*iden_p); it == result.end())
					{
						result[::std::move(*iden_p)] = parse_decl(begin, end);
					}
					else
					{
						using raw_err_iden = raw_error_message_type::identifier;
						post_err(tk.position, syn_ec::entity_redefine, raw_err_iden{ code_convert<err_ms>(*iden_p) });
						parse_decl(begin, end);
					}
				}
				else if (tk.code == pdn_token_code::semicolon)
				{
					update_token(begin, end);
				}
				else if (is_expr_first(tk.code))
				{
					post_err(tk.position, syn_ec::expect_entity_name, to_raw_error_token(tk));
					parse_expr(begin, end);
				}
				else if (tk.code == pdn_token_code::eof)
				{
					post_err(left_curly_brackets_pos, syn_ec::missing_right_curly_brackets, {});
					return make_proxy<types::object<char_t>>(::std::move(result));
				}
				else
				{
					post_err(tk.position, syn_ec::expect_definition_of_named_entity, to_raw_error_token(tk));
					update_token(begin, end);
				}
			}

			// to ... { ... } CURRPOS
			update_token(begin, end);

			return make_proxy<types::object<char_t>>(::std::move(result));
		}

		auto entity_cast(entity_type src, type_code target_type_c, source_position type_pos) -> entity_type
		{
			using enum type_code;
			switch (target_type_c)
			{
			case i8:        return entity_cast<i8>       (::std::move(src), type_pos);
			case i16:       return entity_cast<i16>      (::std::move(src), type_pos);
			case i32:       return entity_cast<i32>      (::std::move(src), type_pos);
			case i64:       return entity_cast<i64>      (::std::move(src), type_pos);
			case u8:        return entity_cast<u8>       (::std::move(src), type_pos);
			case u16:       return entity_cast<u16>      (::std::move(src), type_pos);
			case u32:       return entity_cast<u32>      (::std::move(src), type_pos);
			case u64:       return entity_cast<u64>      (::std::move(src), type_pos);
			case boolean:   return entity_cast<boolean>  (::std::move(src), type_pos);
			case f32:       return entity_cast<f32>      (::std::move(src), type_pos);
			case f64:       return entity_cast<f64>      (::std::move(src), type_pos);
			case character: return entity_cast<character>(::std::move(src), type_pos);
			case string:    return entity_cast<string>   (::std::move(src), type_pos);
			case list:      return entity_cast<list>     (::std::move(src), type_pos);
			case object:    return entity_cast<object>   (::std::move(src), type_pos);
			case unknown:   assert(0 && "cast to unknown type"); break;
			default:        assert(0 && "type cast unhandled");  break;
			}
			return types::cppint{};
		}

		template <type_code target_type_c>
		auto entity_cast(entity_type src, source_position type_pos) -> entity_type
		{
			static_assert(target_type_c != type_code::unknown, "[pdn] cast to unknown type");

			using parser_utility::default_entity_value;

			return ::std::visit([&](auto& arg) -> entity_type
			{
				using src_t = remove_proxy_t<::std::decay_t<decltype(arg)>>;
				using tar_t = type_code_to_type_t<target_type_c, char_t>; // unknown -> void

				constexpr auto src_c = type_to_type_code_v<src_t, char_t>;
				constexpr auto tar_c = type_to_type_code_v<tar_t, char_t>;

				using namespace error_message_literals;
				using namespace types::concepts;
				using enum syntax_error_code;

				using raw_err_casting = raw_error_message_type::casting_msg;

				if constexpr (::std::same_as<src_t, tar_t>)
				{
					return entity_type{ ::std::move(arg) };
				}
				else if constexpr (pdn_sint<src_t>)
				{
					if constexpr (pdn_sint<tar_t>)
					{
						if (arg > ::std::numeric_limits<tar_t>::max() || arg < ::std::numeric_limits<tar_t>::min())
						{
							post_err(type_pos, casting_domain_error, raw_err_casting{ { arg }, src_c, tar_c });
						}
						return static_cast<tar_t>(arg);
					}
					else if constexpr (pdn_uint<tar_t>)
					{
						using u_src_t = ::std::make_unsigned_t<src_t>;
						using s_tar_t = ::std::make_signed_t<tar_t>;
						if (arg < s_tar_t(::std::numeric_limits<tar_t>::min()) || u_src_t(arg) > ::std::numeric_limits<tar_t>::max())
						{
							post_err(type_pos, casting_domain_error, raw_err_casting{ { arg }, src_c, tar_c });
						}
						return static_cast<tar_t>(arg);
					}
					else if constexpr (pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return static_cast<tar_t>(arg);
					}
					else // sint -> character|string|list|object
					{
						post_err(type_pos, illegal_cast, raw_err_casting{ { arg }, src_c, tar_c });
						return default_entity_value<char_t>(target_type_c);
					}
				}
				else if constexpr (pdn_uint<src_t>)
				{
					if constexpr (pdn_uint<tar_t>)
					{
						if (arg > ::std::numeric_limits<tar_t>::max() || arg < ::std::numeric_limits<tar_t>::min())
						{
							post_err(type_pos, casting_domain_error, raw_err_casting{ { arg }, src_c, tar_c });
						}
						return static_cast<tar_t>(arg);
					}
					else if constexpr (pdn_sint<tar_t>)
					{
						using u_tar_t = ::std::make_unsigned_t<tar_t>;
						// system error, uint::min < 0 or sint::min >= 0 is impossible,
						// if pdn::types::concept::pdn_uint and pdn_sint work.
						static_assert(::std::numeric_limits<src_t>::min() >= 0, "[pdn] system error");
						static_assert(::std::numeric_limits<tar_t>::min() < 0,  "[pdn] system error");
						if (arg > u_tar_t(::std::numeric_limits<tar_t>::max()))
						{
							post_err(type_pos, casting_domain_error, raw_err_casting{ { arg }, src_c, tar_c });
						}
						return static_cast<tar_t>(arg);
					}
					else if constexpr (pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return static_cast<tar_t>(arg);
					}
					else // uint -> character|string|list|object
					{
						post_err(type_pos, illegal_cast, raw_err_casting{ { arg }, src_c, tar_c });
						return default_entity_value<char_t>(target_type_c);
					}
				}
				else if constexpr (pdn_fp<src_t>)
				{
					if constexpr (pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return static_cast<tar_t>(arg);
					}
					else if constexpr (pdn_integral<tar_t>)
					{
						post_err(type_pos, illegal_cast, raw_err_casting{ { arg }, src_c, tar_c });
						return static_cast<tar_t>(arg);
					}
					else // fp -> character|string|list|object
					{
						post_err(type_pos, illegal_cast, raw_err_casting{ { arg }, src_c, tar_c });
						return default_entity_value<char_t>(target_type_c);
					}
				}
				else if constexpr (pdn_bool<src_t>)
				{
					if constexpr (pdn_integral<tar_t> || pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return static_cast<tar_t>(arg);
					}
					else // bool -> character|string|list|object
					{
						post_err(type_pos, illegal_cast, raw_err_casting{ { arg }, src_c, tar_c });
						return default_entity_value<char_t>(target_type_c);
					}
				}
				else // character|string|list|object
				{
					if constexpr (::std::same_as<src_t, types::character<char_t>>)
					{
						auto s = unicode::code_convert<error_msg_string>(arg.to_string_view());
						using err_char_t = types::character<error_msg_char>;
						post_err(type_pos, illegal_cast, raw_err_casting{ { err_char_t{ s.data(), s.size() } }, src_c, tar_c });
					}
					else if constexpr (::std::same_as<src_t, types::string<char_t>>)
					{
						using err_s_t = types::string<error_msg_char>;
						auto sp = make_proxy<err_s_t>(unicode::code_convert<err_s_t>(*arg));
						post_err(type_pos, illegal_cast, raw_err_casting{ { ::std::move(sp) }, src_c, tar_c});
					}
					else // list|object
					{
						post_err(type_pos, illegal_cast, raw_err_casting{ {}, src_c, tar_c });
					}
					return default_entity_value<char_t>(target_type_c);
				}
			}, src);
		}

		auto token_value_to_entity(token<char_t> src) -> entity<char_t>
		{
			return ::std::visit([&]<typename arg_t>(arg_t arg) -> entity<char_t>
			{
				if constexpr (::std::same_as<arg_t, ::std::monostate>)
				{
					assert(0 && "token have no value");
					return types::cppint{};
				}
				else if constexpr (::std::same_as<arg_t, dev_util::at_iden_string_proxy>)
				{
					if (auto value_opt = func_pkg->generate_constant(arg.get_id()); value_opt)
					{
						return ::std::move(*value_opt);
					}
					else
					{
						using raw_err_iden = raw_error_message_type::identifier;
						using unicode::code_convert;
						post_err(src.position, syn_ec::at_value_not_found, raw_err_iden{ code_convert<error_msg_string>(arg.get_id()) });
					}
					return types::cppint{};
				}
				else
				{
					return entity<char_t>{ ::std::move(arg) };
				}
			}, ::std::move(src.value));
		}

		void update_token(auto& begin, auto end)
		{
			if (begin != end)
			{
				tk = ::std::move(*begin);
				++begin;
			}
			else
			{
				tk = token<char_t>{ .position = tk.position, .code = pdn_token_code::eof, .value = {} };
			}
		}

		auto type_gen(const types::string<char_t>& iden) -> type_code
		{
			return func_pkg->generate_type(iden);
		}

		auto err_msg_gen(source_position pos, auto err_c, raw_error_message_variant raw_msg) -> error_msg_string
		{
			return func_pkg->generate_error_message(raw_error_message{ { err_c }, pos, ::std::move(raw_msg) });
		}

		void post_err(source_position pos, auto err_c, raw_error_message_variant raw_msg)
		{
			func_pkg->handle_error(error_message{ err_c, pos, err_msg_gen(pos, err_c, ::std::move(raw_msg)) });
		}
	private:
		function_package* func_pkg{};
		token<char_t>     tk{};
	};
}

#endif
