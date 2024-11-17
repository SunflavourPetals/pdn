#ifndef PDN_Header_pdn_parser
#define PDN_Header_pdn_parser

#include <utility>
#include <variant>
#include <limits>
#include <concepts>
#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_code_convert.h"

#include "pdn_types.h"
#include "pdn_token.h"
#include "pdn_token_code.h"
#include "pdn_error_string.h"
#include "pdn_syntax_error_code.h"
#include "pdn_token_code_to_error_msg_string.h"
#include "pdn_make_slashes_string.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_type_code.h"
#include "pdn_type_code_to_type.h"
#include "pdn_type_to_type_code.h"
#include "pdn_type_code_to_error_msg_string.h"

#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_type_generator_concept.h"

namespace pdn::concepts
{
	template <typename type, typename char_t>
	concept function_package_for_parser
		 = concepts::error_handler<type>
		&& concepts::error_message_generator<type>
		&& concepts::type_generator<type, char_t>;
}

namespace pdn::dev_util
{
	template <typename type, typename char_t>
	concept token_iterator = requires (type it)
	{
		{ *it } -> ::std::convertible_to<token<char_t>>;
		++it;
	};
}

namespace pdn
{
	template <unicode::concepts::code_unit char_t, concepts::function_package_for_parser<char_t> function_package>
	class parser
	{
	public:
		using char_type = char_t;
		using entity = types::entity<char_type>;
	private:
		using si8    = types::i8;
		using si16   = types::i16;
		using si32   = types::i32;
		using si64   = types::i64;
		using ui8    = types::u8;
		using ui16   = types::u16;
		using ui32   = types::u32;
		using ui64   = types::u64;
		using fp32   = types::f32;
		using fp64   = types::f64;
		using bln    = types::boolean;
		using cha    = types::character<char_type>;
		using str    = types::string<char_type>;
		using lst    = types::list<char_type>;
		using obj    = types::object<char_type>;
		using str_pr = proxy<str>;
		using lst_pr = proxy<lst>;
		using obj_pr = proxy<obj>;
		using syn_ec = syntax_error_code;
	public:
		template <dev_util::token_iterator<char_t> it_t>
		void parse(it_t&& begin, auto end, obj& o)
		{
			using enum pdn_token_code;
			parse_start(begin, end, o);
			if (tk.code != eof)
			{
				post_err(tk.position, syn_ec::inner_error_parse_terminated, {});
			}
		}
		template <dev_util::token_iterator<char_t> it_t>
		entity parse(it_t&& begin, auto end)
		{
			obj o;
			parse(begin, end, o);
			return make_proxy<obj>(::std::move(o));
		}
		parser(function_package& function_pkg) : func_pkg{ &function_pkg } {}
	private:
		void parse_start(auto& begin, auto end, obj& o)
		{
			using enum pdn_token_code;
			using err_ms = error_msg_string;
			using unicode::code_convert;

			for (update_token(begin, end); tk.code != eof; )
			{
				if (tk.code == identifier)
				{
					auto iden_p = ::std::get<str_pr>(::std::move(tk.value));
					if (auto it = o.find(*iden_p); it == o.end())
					{
						o[::std::move(*iden_p)] = parse_decl(begin, end);
					}
					else
					{
						auto pos = tk.position;
						parse_decl(begin, end);
						auto slashes = make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*iden_p));
						auto raw_msg = raw_error_message_type::redefined_identifier{ ::std::move(slashes) };
						// flag 1
						post_err(pos, syn_ec::entity_redefine, ::std::move(raw_msg));
					}
				}
				else
				{
					using namespace error_message_literals;
					if (tk.code == semicolon)
					{
						update_token(begin, end);
					}
					else
					{
						auto pos = tk.position;
						raw_error_message_type::error_token raw_msg{ to_raw_error_token(tk) };
						if (is_expr_first(tk.code))
						{
							parse_expr(begin, end);
							// flag 2
							post_err(pos, syn_ec::expect_entity_name, ::std::move(raw_msg));
						}
						else
						{
							update_token(begin, end);
							// flag 2
							post_err(pos, syn_ec::unexpected_token, ::std::move(raw_msg));
						}
					}
				}
			}
		}

		entity parse_decl(auto& begin, auto end)
		{
			// ... iden CURPOS ...
			// expect : | expr

			using enum pdn_token_code;

			type_code type_c = type_code::unknown;
			auto type_pos = tk.position;

			update_token(begin, end);
			if (tk.code == colon)
			{
				update_token(begin, end);
				type_pos = tk.position;

				if (tk.code == identifier)
				{
					type_c = parse_type_spec(begin, end);
					// ... iden : typename CURPOS(expect expr) ...
				}
				// ... iden : CURPOS(expect expr) ...
			}

			if (!is_expr_first(tk.code))
			{
				post_err(tk.position, syn_ec::expect_expression, token_code_to_error_msg_string(tk.code));
				return default_entity_value(type_c);
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

		type_code parse_type_spec(auto& begin, auto end)
		{
			//    ... iden : CURPOS expr ...
			// or ... CURPOS : expr ...
			// expect typename(identifier)

			using enum pdn_token_code;
			using err_ms = error_msg_string;
			using unicode::code_convert;

			if (tk.code == identifier)
			{
				auto type_c = type_gen(*::std::get<str_pr>(tk.value));
				if (type_c == type_code::unknown)
				{
					post_err(tk.position, syn_ec::unknown_type, code_convert<err_ms>(*::std::get<str_pr>(tk.value)));
				}
				update_token(begin, end);
				return type_c;
			}

			post_err(tk.position, syn_ec::expect_type_name, token_code_to_error_msg_string(tk.code));

			return type_code::unknown;
		}

		entity parse_expr(auto& begin, auto end)
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
			using err_ms = error_msg_string;
			using unicode::code_convert;

			::std::size_t   sign_count{ 0 };
			source_position last_sign_pos{};
			source_position last_negative_sign_pos{};
			pdn_token_code  sign_code{};
			bool            negative_sign{ false };

			for (; tk.code == minus || tk.code == plus; update_token(begin, end))
			{
				last_sign_pos = tk.position;
				sign_code     = tk.code;
				if (tk.code == minus)
				{
					last_negative_sign_pos = tk.position;
					negative_sign          = !negative_sign;
				}
				++sign_count;
			}

			// check is operator + | - illegal, except unsigned integral types
			if (sign_count > 0)
			{
				using str_view = ::std::basic_string_view<char_t>;
				err_ms invalid_op_msg_str{};
				switch (tk.code)
				{
				case literal_boolean:
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code)
						.append(::std::get<bln>(tk.value) ? u8"@true"_em : u8"@false"_em);
					break;
				case literal_character:
				{
					auto c = ::std::get<cha>(tk.value);
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code)
						.append(u8"\'"_em)
						.append(make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(str_view{ c.data(), c.size() })))
						.append(u8"\'"_em);
					break;
				}
				case literal_string:
				{
					auto sp = ::std::get<str_pr>(tk.value);
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code)
						.append(u8"\""_em)
						.append(make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*sp)))
						.append(u8"\""_em);
					break;
				}
				case left_brackets:
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code).append(u8"[...]"_em);
					break;
				case left_curly_brackets:
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code).append(u8"{...}"_em);
					break;
				default:
					break;
				}
				if (!invalid_op_msg_str.empty())
				{
					post_err(last_sign_pos, syn_ec::invalid_operation, ::std::move(invalid_op_msg_str));
				}
			}

			switch (tk.code)
			{
			case literal_string:
			{
				str cat_string{};
				while (tk.code == literal_string) // concatenation
				{
					cat_string += *::std::get<str_pr>(tk.value);
					update_token(begin, end);
				}
				return token_value_to_entity(make_proxy<str>(::std::move(cat_string)), negative_sign, last_negative_sign_pos);
			}
			case literal_boolean:
			case literal_character:
			case literal_floating_point:
			case literal_integer:
			{
				auto result = token_value_to_entity(::std::move(tk.value), negative_sign, last_negative_sign_pos); // check is operator- illegal, for unsigned integral types
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
				post_err(tk.position, syn_ec::expect_expression, token_code_to_error_msg_string(tk.code));
				break;
			}

			return 0;
		}

		entity parse_list_expr(auto& begin, auto end, source_position left_brackets_pos)
		{
			// ... [ CURRPOS ...

			lst result{};

			for (bool with_comma{ true }; tk.code != pdn_token_code::right_brackets; )
			{
				if (tk.code == pdn_token_code::eof)
				{
					post_err(left_brackets_pos, syn_ec::missing_right_brackets, {});
					return make_proxy<lst>(::std::move(result));
				}
				if (!with_comma)
				{
					if (is_list_element_first(tk.code))
					{
						post_err(tk.position, syn_ec::expect_comma, token_code_to_error_msg_string(tk.code));
					}
					else
					{
						post_err(tk.position, syn_ec::unexpected_token, token_code_to_error_msg_string(tk.code));
						update_token(begin, end);
					}
				}
				result.push_back(parse_list_element(begin, end, with_comma));
			}

			// to ... [ ... ] CURRPOS
			update_token(begin, end);

			return make_proxy<lst>(::std::move(result));
		}

		entity parse_list_element(auto& begin, auto end, bool& with_comma)
		{
			// ... iden [colon [typename] ] [ ... ] CURPOS ...

			auto type_c = type_code::unknown;
			auto type_pos = tk.position;

			if (tk.code == pdn_token_code::identifier)
			{
				type_c = parse_type_spec(begin, end);
				if (tk.code == pdn_token_code::colon)
				{
					update_token(begin, end);
				}
				else
				{
					post_err(tk.position, syn_ec::expect_colon, token_code_to_error_msg_string(tk.code));
				}
			}

			entity e = parse_expr(begin, end);
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

		entity parse_object_expr(auto& begin, auto end, source_position left_curly_brackets_pos)
		{
			// ... { CURRPOS ...

			using err_ms = error_msg_string;
			using unicode::code_convert;

			obj result{};

			while (tk.code != pdn_token_code::right_curly_brackets)
			{
				if (tk.code == pdn_token_code::identifier)
				{
					auto iden_p = ::std::get<str_pr>(::std::move(tk.value));
					if (auto it = result.find(*iden_p); it == result.end())
					{
						result[::std::move(*iden_p)] = parse_decl(begin, end);
					}
					else
					{
						auto pos = tk.position;
						parse_decl(begin, end);
						auto slashes = make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*iden_p));
						auto raw_msg = raw_error_message_type::redefined_identifier{ ::std::move(slashes) };
						// flag 1
						post_err(pos, syn_ec::entity_redefine, ::std::move(raw_msg));
					}
				}
				else if (tk.code == pdn_token_code::semicolon)
				{
					update_token(begin, end);
				}
				else if (is_expr_first(tk.code))
				{
					auto pos = tk.position;
					auto tk_code = tk.code;
					parse_expr(begin, end);
					post_err(pos, syn_ec::expect_entity_name, token_code_to_error_msg_string(tk_code));
				}
				else if (tk.code == pdn_token_code::eof)
				{
					post_err(left_curly_brackets_pos, syn_ec::missing_right_curly_brackets, {});
					return make_proxy<obj>(::std::move(result));
				}
				else
				{
					post_err(tk.position, syn_ec::unexpected_token, token_code_to_error_msg_string(tk.code));
					update_token(begin, end);
				}
			}

			// to ... { ... } CURRPOS
			update_token(begin, end);

			return make_proxy<obj>(::std::move(result));
		}

		entity entity_cast(entity src, type_code target_type_c, source_position type_pos)
		{
			using enum type_code;
			switch (target_type_c)
			{
			case i8:
				return entity_cast<i8>(::std::move(src), type_pos);
			case i16:
				return entity_cast<i16>(::std::move(src), type_pos);
			case i32:
				return entity_cast<i32>(::std::move(src), type_pos);
			case i64:
				return entity_cast<i64>(::std::move(src), type_pos);
			case u8:
				return entity_cast<u8>(::std::move(src), type_pos);
			case u16:
				return entity_cast<u16>(::std::move(src), type_pos);
			case u32:
				return entity_cast<u32>(::std::move(src), type_pos);
			case u64:
				return entity_cast<u64>(::std::move(src), type_pos);
			case boolean:
				return entity_cast<boolean>(::std::move(src), type_pos);
			case f32:
				return entity_cast<f32>(::std::move(src), type_pos);
			case f64:
				return entity_cast<f64>(::std::move(src), type_pos);
			case character:
				return entity_cast<character>(::std::move(src), type_pos);
			case string:
				return entity_cast<string>(::std::move(src), type_pos);
			case list:
				return entity_cast<list>(::std::move(src), type_pos);
			case object:
				return entity_cast<object>(::std::move(src), type_pos);
			case unknown:
				return entity_cast<unknown>(::std::move(src), type_pos);
			default:
				break;
			}
			return 0;
		}

		template <type_code target_type_c>
		entity entity_cast(entity src, source_position type_pos)
		{
			using target_t_outer = type_code_to_type_t<target_type_c, char_t>; // unknown -> void

			return ::std::visit([&]<typename arg_t>(arg_t && arg) -> entity
			{
				using src_t = type_traits::remove_proxy_t<::std::decay_t<decltype(arg)>>;
				using tar_t = target_t_outer;

				using namespace error_message_literals;
				using namespace types::concepts;
				using enum syntax_error_code;

				auto domain_err_msg_gen = [&](const auto& arg, auto min, auto max)
					{
						constexpr auto src_tc = type_to_type_code_v<src_t, char_t>;
						constexpr auto tar_tc = type_to_type_code_v<tar_t, char_t>;

						using namespace error_message_literals;

						return type_code_to_error_msg_string(src_tc)
							.append(u8": "_em)
							.append(reinterpret_to_err_msg_str(::std::to_string(arg)))
							.append(u8" -> "_em)
							.append(type_code_to_error_msg_string(tar_tc))
							.append(u8": ["_em)
							.append(reinterpret_to_err_msg_str(::std::to_string(min)))
							.append(u8", "_em)
							.append(reinterpret_to_err_msg_str(::std::to_string(max)))
							.append(u8"]"_em);
					};

				auto bad_cast_err_msg_gen = []()
					{
						constexpr auto src_tc = type_to_type_code_v<src_t, char_t>;
						constexpr auto tar_tc = type_to_type_code_v<tar_t, char_t>;
						using namespace error_message_literals;
						return type_code_to_error_msg_string(src_tc)
							.append(u8" -> "_em)
							.append(type_code_to_error_msg_string(tar_tc));
					};

				// It does not work for msvc and clang (YMD:2024-09-02)
				// ```
				//     if constexpr (pdn_integral<src_t> && pdn_integral<tar_t>)
				//     {
				//         using u_src_t = ::std::make_unsigned_t<src_t>;
				//         using s_tar_t = ::std::make_signed_t<tar_t>;
				//         std::cout << u_src_t{} << s_tar_t{} << "\n"; // test
				//     }
				// ```

				if constexpr (::std::same_as<src_t, tar_t>)
				{
					return arg;
				}
				else if constexpr (target_type_c == type_code::unknown)
				{
					post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
					return arg;
				}
				else if constexpr (pdn_sint<src_t>)
				{
					if constexpr (pdn_sint<tar_t>)
					{
						constexpr auto max = ::std::numeric_limits<tar_t>::max();
						constexpr auto min = ::std::numeric_limits<tar_t>::min();
						if (arg > max || arg < min)
						{
							post_err(type_pos, casting_domain_error, domain_err_msg_gen(arg, min, max));
						}
						return tar_t(::std::forward<arg_t>(arg));
					}
					else if constexpr (pdn_uint<tar_t>)
					{
						constexpr auto max = ::std::numeric_limits<tar_t>::max();
						constexpr auto min = ::std::numeric_limits<tar_t>::min();
						using u_src_t = ::std::make_unsigned_t<src_t>;
						using s_tar_t = ::std::make_signed_t<tar_t>;
						if (arg < s_tar_t(min) || u_src_t(arg) > max)
						{
							post_err(type_pos, casting_domain_error, domain_err_msg_gen(arg, min, max));
						}
						return tar_t(::std::forward<arg_t>(arg));
					}
					else if constexpr (pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return tar_t(::std::forward<arg_t>(arg));
					}
					else
					{
						post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
						return default_entity_value(target_type_c);
					}
				}
				else if constexpr (pdn_uint<src_t>)
				{
					if constexpr (pdn_uint<tar_t>)
					{
						constexpr auto max = ::std::numeric_limits<tar_t>::max();
						constexpr auto min = ::std::numeric_limits<tar_t>::min();
						if (arg > max || arg < min)
						{
							post_err(type_pos, casting_domain_error, domain_err_msg_gen(arg, min, max));
						}
						return tar_t(::std::forward<arg_t>(arg));
					}
					else if constexpr (pdn_sint<tar_t>)
					{
						constexpr auto max = ::std::numeric_limits<tar_t>::max();
						constexpr auto min = ::std::numeric_limits<tar_t>::min();
						using u_tar_t = ::std::make_unsigned_t<tar_t>;
						// system error, uint::min < 0 or sint::min >= 0 is impossible,
						// if pdn::types::concept::pdn_uint|pdn_sint works
						static_assert(::std::numeric_limits<src_t>::min() >= 0, "[pdn] system error");
						static_assert(::std::numeric_limits<tar_t>::min() < 0, "[pdn] system error");
						if (arg > u_tar_t(max))
						{
							post_err(type_pos, casting_domain_error, domain_err_msg_gen(arg, min, max));
						}
					}
					else if constexpr (pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return tar_t(::std::forward<arg_t>(arg));
					}
					else
					{
						post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
						return default_entity_value(target_type_c);
					}
				}
				else if constexpr (pdn_fp<src_t>)
				{
					if constexpr (pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return tar_t(::std::forward<arg_t>(arg));
					}
					else if constexpr (pdn_integral<tar_t>)
					{
						post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
						return tar_t(::std::forward<arg_t>(arg));
					}
					else
					{
						post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
						return default_entity_value(target_type_c);
					}
				}
				else if constexpr (pdn_bool<src_t>)
				{
					if constexpr (pdn_integral<tar_t> || pdn_fp<tar_t> || pdn_bool<tar_t>)
					{
						return tar_t(::std::forward<arg_t>(arg));
					}
					else
					{
						post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
						return default_entity_value(target_type_c);
					}
				}
				else
				{
					post_err(type_pos, illegal_cast, bad_cast_err_msg_gen());
					return default_entity_value(target_type_c);
				}
				return arg;
			}, src);
		}

		entity token_value_to_entity(token_value_variant<char_t> v, bool neg, source_position last_negative_sign_pos)
		{
			return std::visit([&]<typename arg_fwd_t>(arg_fwd_t && arg) -> entity
			{
				using decay_arg_t = ::std::decay_t<decltype(arg)>;
				using types::concepts::pdn_sint;
				using types::concepts::pdn_uint;
				using types::concepts::pdn_fp;
				if constexpr (pdn_fp<decay_arg_t> || pdn_sint<decay_arg_t>)
				{
					return neg ? entity{ -::std::forward<arg_fwd_t>(arg) } : entity{ ::std::forward<arg_fwd_t>(arg) };
				}
				else if constexpr (::std::same_as<decay_arg_t, ::std::monostate>)
				{
					post_err(tk.position, syn_ec::inner_error_token_have_no_value, {});
					return 0;
				}
				else if constexpr (pdn_uint<decay_arg_t>) // check is operator- illegal, for unsigned integral types
				{
					if (neg)
					{
						using namespace error_message_literals;
						constexpr auto type_c = type_to_type_code_v<decay_arg_t, char_t>;
						auto msg = u8"-"_em
							.append(reinterpret_to_err_msg_str(::std::to_string(arg)))
							.append(u8" : "_em)
							.append(type_code_to_error_msg_string(type_c));
						post_err(last_negative_sign_pos, syn_ec::invalid_operation, ::std::move(msg));
					}
					return ::std::forward<arg_fwd_t>(arg);
				}
				else
				{
					return ::std::forward<arg_fwd_t>(arg);
				}
			}, ::std::move(v));
		}

		template <typename iter_t>
		void update_token(iter_t&& begin, auto end)
		{
			if (begin != end)
			{
				tk = *begin;
				++begin;
			}
			else
			{
				tk = token<char_t>{ .position = {}, .code = pdn_token_code::eof, .value = {} };
			}
		}

		type_code type_gen(const str& iden)
		{
			return func_pkg->generate_type(iden);
		}

		void post_err(source_position pos, auto error_code, raw_error_message_variant&& raw_msg)
		{
			func_pkg->handle_error(error_message{ error_code, pos, func_pkg->generate_error_message(error_code, ::std::move(raw_msg)) });
		}

		static constexpr bool is_expr_first(pdn_token_code c) noexcept
		{
			using enum pdn_token_code;
			switch (c)
			{
			case minus:
			case plus:
			case literal_boolean:
			case literal_character:
			case literal_string:
			case literal_floating_point:
			case literal_integer:
			case left_brackets:
			case left_curly_brackets:
				return true;
			default:
				break;
			}
			return false;
		}

		static constexpr bool is_list_element_first(pdn_token_code c) noexcept
		{
			return is_expr_first(c) || c == pdn_token_code::identifier;
		}

		static constexpr entity default_entity_value(type_code type_c)
		{
			using enum type_code;
			switch (type_c)
			{
			case i8:        return si8{};
			case i16:       return si16{};
			case i32:       return si32{};
			case i64:       return si64{};
			case u8:        return ui8{};
			case u16:       return ui16{};
			case u32:       return ui32{};
			case u64:       return ui64{};
			case f32:       return fp32{};
			case f64:       return fp64{};
			case boolean:   return bln{};
			case character: return cha{};
			case string:    return str_pr{ make_proxy<str>() };
			case list:      return lst_pr{ make_proxy<lst>() };
			case object:    return obj_pr{ make_proxy<obj>() };
			default:        break;
			}
			return 0;
		}

		static constexpr token<error_msg_char> to_raw_error_token(token<char_t> src)
		{
			if constexpr (::std::same_as<::std::remove_cv_t<char_t>, error_msg_char>)
			{
				return src;
			}
			else
			{
				auto converted_value = ::std::visit([](auto&& arg) -> token_value_variant<error_msg_char>
				{
					using unicode::code_convert;
					using arg_t = ::std::decay_t<decltype(arg)>;
					if constexpr (::std::same_as<arg_t, cha>)
					{
						auto converted = code_convert<error_msg_string>(arg.to_string_view());
						return types::character<error_msg_char>{ converted.cbegin(), converted.size() };
					}
					else if constexpr (::std::same_as<arg_t, str_pr>)
					{
						auto converted = code_convert<error_msg_string>(*arg);
						return types::character<error_msg_char>{ converted.cbegin(), converted.size() };
					}
					else
					{
						return arg;
					}
				}, ::std::move(src.value));
				return { src.position, src.code, ::std::move(converted_value) };
			}
		}
	private:
		function_package* func_pkg{};
		token<char_t>     tk{};
	};
}

#endif
