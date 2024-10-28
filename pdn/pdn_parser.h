#ifndef PDN_Header_pdn_parser
#define PDN_Header_pdn_parser

#include <utility>
#include <variant>
#include <limits>
#include <string>
#include <type_traits>
#include <concepts>
#include <format>
#include <codecvt>
#include <fstream>

#include "pdn_type_code.h"
#include "pdn_type_code_to_type.h"
#include "pdn_type_to_type_code.h"
#include "pdn_type_code_to_error_msg_string.h"

#include "pdn_type_generator.h"
#include "pdn_type_generator_std.h"

#include "pdn_unicode_base.h"
#include "pdn_code_convert.h"

#include "pdn_lexer.h"
#include "pdn_types.h"
#include "pdn_token.h"
#include "pdn_token_code.h"
#include "pdn_error_string.h"
#include "pdn_syntax_error_code.h"
#include "pdn_token_code_to_error_msg_string.h"
#include "pdn_make_slashes_string.h"

#include "pdn_bom_reader.h"
#include "pdn_swap_chain.h"
#include "pdn_code_unit_iterator.h"
#include "pdn_code_point_iterator.h"

#include "pdn_exception.h"

#include "pdn_source_position_recorder.h"

namespace pdn
{
	template <typename char_t>
	class default_function_package
	{
	private:
		pdn::source_position_recorder pos_recorder{};
		pdn::default_error_handler err_handler{};
		pdn::default_error_message_generator err_msg_gen{};
		pdn::default_constants_generator<char_t> const_gen{};
		pdn::default_type_generator<char_t> type_gen{};
	public:
		pdn::source_position position() const
		{
			return pos_recorder.position();
		}
		void update(char32_t c)
		{
			pos_recorder.update(c);
		}
		void handle_error(const pdn::error_message& msg)
		{
			err_handler.handle_error(msg);
		}
		pdn::error_msg_string generate_error_message(pdn::error_code_variant errc_variant, pdn::error_msg_string err_msg_str)
		{
			return err_msg_gen.generate_error_message(std::move(errc_variant), std::move(err_msg_str));
		}
		::std::optional<pdn::constant_variant<char_t>> generate_constant(pdn::unicode::utf_8_code_unit_string iden)
		{
			return const_gen.generate_constant(std::move(iden));
		}
		type_code generate_type(const types::string<char_t>& iden)
		{
			return type_gen.generate_type(iden);
		}
	};
}

namespace pdn::dev_util
{
	template <typename type, typename char_t>
	concept function_package_for_parser
		 = concepts::source_position_getter<type>
		&& concepts::error_handler<type>
		&& concepts::error_message_generator<type>
		&& concepts::constants_generator<type, char_t>;
}

namespace pdn::experimental
{
	template <unicode::concepts::unicode_code_unit char_t, dev_util::function_package_for_parser<char_t> function_package>
	class parser
	{
	public:
		using char_type = char_t;
		using entity = types::entity<char_type>;
	private:
		using si8 = types::i8;
		using si16 = types::i16;
		using si32 = types::i32;
		using si64 = types::i64;
		using ui8 = types::u8;
		using ui16 = types::u16;
		using ui32 = types::u32;
		using ui64 = types::u64;
		using fp32 = types::f32;
		using fp64 = types::f64;
		using bln = types::boolean;
		using cha = types::character<char_type>;
		using str = types::string<char_type>;
		using lst = types::list<char_type>;
		using obj = types::object<char_type>;
		using str_pr = proxy<str>;
		using lst_pr = proxy<lst>;
		using obj_pr = proxy<obj>;
		using syn_ec = syntax_error_code;
	private:
		function_package* func_pkg{};
		token<char_t> tk{};
	public:
		void parse(auto&& begin, auto end, obj& o)
		{
			using enum pdn_token_code;
			parse_start(begin, end, o);
			if (tk.code != eof)
			{
				post_err(tk.position, syn_ec::inner_error_parse_terminated, {});
			}
		}
		entity parse(auto&& begin, auto end)
		{
			obj o;
			parse(begin, end, o);
			return make_proxy<obj>(::std::move(o));
		}
	public:
		parser(function_package& function_pkg) : func_pkg{ &function_pkg } {}
	private:
		void parse_start(auto& begin, auto end, obj& o)
		{
			using enum pdn_token_code;
			using err_ms = error_msg_string;
			using unicode::code_convert;

			for (tk = get_token(begin, end); tk.code != eof; )
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
						auto msg = make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*iden_p));
						parse_decl(begin, end);
						post_err(pos, syn_ec::entity_redefine, ::std::move(msg));
					}
				}
				else
				{
					using namespace error_message_literals;
					if (tk.code == semicolon)
					{
						tk = get_token(begin, end);
					}
					else if (is_expr_first(tk.code))
					{
						auto pos = tk.position;
						parse_expr(begin, end);
						post_err(pos, syn_ec::expect_entity_name, {});
					}
					else
					{
						post_err(tk.position, syn_ec::unexpected_token, token_code_to_error_msg_string(tk.code));
						tk = get_token(begin, end);
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

			tk = get_token(begin, end);
			if (tk.code == colon)
			{
				tk = get_token(begin, end);
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

			entity e = parse_expr(begin, end);
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
				tk = get_token(begin, end);
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

			::std::size_t sign_count{ 0 };
			auto sign_pos = tk.position;
			auto sign_code = tk.code;
			bool negative_sign{ false };

			for (; tk.code == minus || tk.code == plus; tk = get_token(begin, end))
			{
				sign_pos = tk.position;
				sign_code = tk.code;
				if (tk.code == minus)
				{
					negative_sign = !negative_sign;
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
				}
				break;
				case literal_string:
				{
					auto sp = ::std::get<str_pr>(tk.value);
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code)
						.append(u8"\""_em)
						.append(make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*sp)))
						.append(u8"\""_em);
				}
				break;
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
					post_err(sign_pos, syn_ec::invalid_operation, ::std::move(invalid_op_msg_str));
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
					tk = get_token(begin, end);
				}
				return token_value_to_entity(make_proxy<str>(::std::move(cat_string)), negative_sign);
			}
			case literal_boolean:
			case literal_character:
			case literal_floating_point:
			case literal_integer:
			{
				auto result = token_value_to_entity(::std::move(tk.value), negative_sign); // check is operator- illegal, for unsigned integral types
				tk = get_token(begin, end);
				return result;
			}
			case left_brackets:
			{
				auto left_brackets_pos = tk.position;
				tk = get_token(begin, end);
				return parse_list_expr(begin, end, left_brackets_pos);
			}
			case left_curly_brackets:
			{
				auto left_curly_brackets_pos = tk.position;
				tk = get_token(begin, end);
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

			for (bool with_comma{}; tk.code != pdn_token_code::right_brackets; )
			{
				if (tk.code == pdn_token_code::eof)
				{
					post_err(left_brackets_pos, syn_ec::missing_right_brackets, {});
					return make_proxy<lst>(::std::move(result));
				}
				result.push_back(parse_list_element(begin, end, with_comma));
				if (!with_comma && tk.code != pdn_token_code::right_brackets)
				{
					post_err(tk.position, syn_ec::expect_comma, token_code_to_error_msg_string(tk.code));
					tk = get_token(begin, end);
				}
			}

			// to ... [ ... ] CURRPOS
			tk = get_token(begin, end);

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
					tk = get_token(begin, end);
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
				tk = get_token(begin, end);
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
						auto msg = make_slashes_string<error_msg_string>(unicode::code_convert<unicode::code_point_string>(*iden_p));
						parse_decl(begin, end);
						post_err(pos, syn_ec::entity_redefine, ::std::move(msg));
					}
				}
				else
				{
					if (tk.code == pdn_token_code::semicolon)
					{
						tk = get_token(begin, end);
					}
					else if (is_expr_first(tk.code))
					{
						auto pos = tk.position;
						parse_expr(begin, end);
						post_err(pos, syn_ec::expect_entity_name, {});
					}
					else if (tk.code == pdn_token_code::eof)
					{
						post_err(left_curly_brackets_pos, syn_ec::missing_right_curly_brackets, {});
						return make_proxy<obj>(::std::move(result));
					}
					else
					{
						post_err(tk.position, syn_ec::unexpected_token, token_code_to_error_msg_string(tk.code));
						tk = get_token(begin, end);
					}
				}
			}

			// to ... { ... } CURRPOS
			tk = get_token(begin, end);

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

		entity token_value_to_entity(token_value_variant<char_t> v, bool neg)
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
						post_err(tk.position, syn_ec::invalid_operation, ::std::move(msg));
						return ::std::forward<arg_fwd_t>(arg);
					}
				}
				else
				{
					return ::std::forward<arg_fwd_t>(arg);
				}
				return 0;
			}, ::std::move(v));
		}

	private:
		template <typename iter_t>
		auto get_token(iter_t&& begin, auto end)
		{
			if (begin != end)
			{
				auto my_tk = *begin;
				++begin;
				return my_tk;
			}
			return token<char_t>{ .position = {}, .code = pdn_token_code::eof, .value = {} };
		}
		type_code type_gen(const str& iden)
		{
			return func_pkg->generate_type(iden);
		}
		void post_err(source_position pos, auto error_code, error_msg_string&& str_for_msg_gen)
		{
			func_pkg->handle_error(error_message{ pos, error_code, func_pkg->generate_error_message(error_code, ::std::move(str_for_msg_gen)) });
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
	};
}

namespace pdn
{
	
}



namespace pdn::inline legacy
{
	template <unicode::concepts::unicode_code_unit char_t = unicode::utf_8_code_unit_t,
	          concepts::source_position_recorder source_position_recorder_t = source_position_recorder>
	class parser : public lexer<char_t, source_position_recorder_t>
	{
	private:
		using base_type = lexer<char_t, source_position_recorder_t>;
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
	private:
		type_generator<char_t> type_gen{ type_generator_std<char_t> };
		token<char_t> tk{};
	public:
		entity parse_code_point_sequence(auto&& begin, auto end)
		{
			obj o;
			parse_code_point_sequence(begin, end, o);
			return make_proxy<obj>(::std::move(o));
		}
		template <typename it_fwd_t>
		entity parse(it_fwd_t&& begin, auto end)
		{
			struct my_handler_test : public pdn::default_error_message_generator
			{
				parser* my_parser;
				source_position_recorder my_pos_r{};
				my_handler_test(parser* p) : my_parser{ p } {}
				pdn::source_position position()
				{
					return my_parser->position();
				}
				void update(char32_t c)
				{
					my_pos_r.update(c);
				}
				void handle_error(const error_message& msg)
				{
					my_parser->err_handler(msg);
				}
				pdn::error_msg_string generate_error_message(pdn::error_code_variant errc_variant, pdn::error_msg_string err_msg_str)
				{
					return my_parser->err_msg_gen(std::move(errc_variant), std::move(err_msg_str));
				}
			};
			my_handler_test my_hd_test{ this };
			auto code_point_it = pdn::make_code_point_iterator(::std::forward<it_fwd_t>(begin), end, my_hd_test);

		//	auto pos_getter = [&]() { return this->position(); };
		//	auto code_point_it = make_code_point_iterator(::std::forward<it_fwd_t>(begin),
		//	                                              end,
		//	                                              pos_getter,
		//	                                              this->err_handler,
		//	                                              this->err_msg_gen);
			return this->parse_code_point_sequence(code_point_it, end);
		}
		entity parse(const ::std::string& filename)
		{
			::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
			if (!source_file.is_open())
			{
				using namespace ::std::string_literals;
				throw failed_in_open_file_error{ "failed in open file \""s + filename + "\""s };
			}
			return parse_file(source_file);
		}
		entity parse(const char* const filename)
		{
			::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
			if (!source_file.is_open())
			{
				using namespace ::std::string_literals;
				throw failed_in_open_file_error{ "failed in open file \""s + filename + "\""s };
			}
			return parse_file(source_file);
		}
		entity parse(const ::std::wstring& filename)
		{
			::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
			if (!source_file.is_open())
			{
				auto& facet = std::use_facet<::std::codecvt<wchar_t, char, ::std::mbstate_t>>(::std::locale());
				::std::mbstate_t mb{};
				::std::string external(filename.size() * facet.max_length(), '\0');
				const wchar_t* from_next{};
				char* to_next{};
				facet.out(mb,
				          filename.data(),
				          filename.data() + filename.size(),
				          from_next,
				          &external[0],
				          &external[external.size()],
				          to_next);
				// skip result checking
				external.resize(to_next - &external[0]);
				using namespace ::std::string_literals;
				throw failed_in_open_file_error{ "failed in open file \""s + external + "\""s };
			}
			return parse_file(source_file);
		}
		entity parse(const wchar_t* const filename)
		{
			::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
			if (!source_file.is_open())
			{
				auto& facet = ::std::use_facet<::std::codecvt<wchar_t, char, ::std::mbstate_t>>(::std::locale());
				::std::mbstate_t mb{};
				::std::wstring_view filename_view{ filename };
				::std::string external(filename_view.size() * facet.max_length(), '\0');
				const wchar_t* from_next{};
				char* to_next{};
				facet.out(mb,
				          filename_view.data(),
				          filename_view.data() + filename_view.size(),
				          from_next,
				          &external[0],
				          &external[external.size()],
				          to_next);
				// skip result checking
				external.resize(to_next - &external[0]);
				using namespace ::std::string_literals;
				throw failed_in_open_file_error{ "failed in open file \""s + external + "\""s };
			}
			return parse_file(source_file);
		}
		void parse_code_point_sequence(auto&& begin, auto end, obj& o)
		{
			this->base_type::reset_position_recorder();
			using enum pdn_token_code;
			parse_start(begin, end, o);
			if (tk.code != eof)
			{
				post_err(tk.position, syn_ec::inner_error_parse_terminated, {});
			}
		}
	public:
		parser(error_handler               err_handler,
		       error_message_generator     err_msg_gen   = error_message_generator_en,
		       constants_generator<char_t> constants_gen = constants_generator_std<char_t>,
		       type_generator<char_t>      type_gen      = type_generator_std<char_t>) :
			lexer<char_t, source_position_recorder_t>(::std::move(err_handler),
			                                          ::std::move(err_msg_gen),
			                                          ::std::move(constants_gen)),
			type_gen{ ::std::move(type_gen) } {}
	private:
		void parse_start(auto& begin, auto end, obj& o)
		{
			using enum pdn_token_code;
			using err_ms = error_msg_string;
			using unicode::code_convert;

			for (tk = get_token(begin, end); tk.code != eof; )
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
						auto msg = make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*iden_p));
						parse_decl(begin, end);
						post_err(pos, syn_ec::entity_redefine, ::std::move(msg));
					}
				}
				else
				{
					using namespace error_message_literals;
					if (tk.code == semicolon)
					{
						tk = get_token(begin, end);
					}
					else if (is_expr_first(tk.code))
					{
						auto pos = tk.position;
						parse_expr(begin, end);
						post_err(pos, syn_ec::expect_entity_name, {});
					}
					else
					{
						post_err(tk.position, syn_ec::unexpected_token, token_code_to_error_msg_string(tk.code));
						tk = get_token(begin, end);
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

			tk = get_token(begin, end);
			if (tk.code == colon)
			{
				tk = get_token(begin, end);
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

			entity e = parse_expr(begin, end);
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
				tk = get_token(begin, end);
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

			::std::size_t sign_count{ 0 };
			auto sign_pos = tk.position;
			auto sign_code = tk.code;
			bool negative_sign{ false };

			for (; tk.code == minus || tk.code == plus; tk = get_token(begin, end))
			{
				sign_pos = tk.position;
				sign_code = tk.code;
				if (tk.code == minus)
				{
					negative_sign = !negative_sign;
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
				}
					break;
				case literal_string:
				{
					auto sp = ::std::get<str_pr>(tk.value);
					invalid_op_msg_str = token_code_to_error_msg_string(sign_code)
						.append(u8"\""_em)
						.append(make_slashes_string<err_ms>(code_convert<unicode::code_point_string>(*sp)))
						.append(u8"\""_em);
				}
					break;
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
					post_err(sign_pos, syn_ec::invalid_operation, ::std::move(invalid_op_msg_str));
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
					tk = get_token(begin, end);
				}
				return token_value_to_entity(make_proxy<str>(::std::move(cat_string)), negative_sign);
			}
			case literal_boolean:
			case literal_character:
			case literal_floating_point:
			case literal_integer:
			{
				auto result = token_value_to_entity(::std::move(tk.value), negative_sign); // check is operator- illegal, for unsigned integral types
				tk = get_token(begin, end);
				return result;
			}
			case left_brackets:
			{
				auto left_brackets_pos = tk.position;
				tk = get_token(begin, end);
				return parse_list_expr(begin, end, left_brackets_pos);
			}
			case left_curly_brackets:
			{
				auto left_curly_brackets_pos = tk.position;
				tk = get_token(begin, end);
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

			for (bool with_comma{};  tk.code != pdn_token_code::right_brackets; )
			{
				if (tk.code == pdn_token_code::eof)
				{
					post_err(left_brackets_pos, syn_ec::missing_right_brackets, {});
					return make_proxy<lst>(::std::move(result));
				}
				result.push_back(parse_list_element(begin, end, with_comma));
				if (!with_comma && tk.code != pdn_token_code::right_brackets)
				{
					post_err(tk.position, syn_ec::expect_comma, token_code_to_error_msg_string(tk.code));
					tk = get_token(begin, end);
				}
			}

			// to ... [ ... ] CURRPOS
			tk = get_token(begin, end);

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
					 tk = get_token(begin, end);
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
				tk = get_token(begin, end);
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
						auto msg = make_slashes_string<error_msg_string>(unicode::code_convert<unicode::code_point_string>(*iden_p));
						parse_decl(begin, end);
						post_err(pos, syn_ec::entity_redefine, ::std::move(msg));
					}
				}
				else
				{
					if (tk.code == pdn_token_code::semicolon)
					{
						tk = get_token(begin, end);
					}
					else if (is_expr_first(tk.code))
					{
						auto pos = tk.position;
						parse_expr(begin, end);
						post_err(pos, syn_ec::expect_entity_name, {});
					}
					else if (tk.code == pdn_token_code::eof)
					{
						post_err(left_curly_brackets_pos, syn_ec::missing_right_curly_brackets, {});
						return make_proxy<obj>(::std::move(result));
					}
					else
					{
						post_err(tk.position, syn_ec::unexpected_token, token_code_to_error_msg_string(tk.code));
						tk = get_token(begin, end);
					}
				}
			}

			// to ... { ... } CURRPOS
			tk = get_token(begin, end);

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
			
			return ::std::visit([&]<typename arg_t>(arg_t&& arg) -> entity
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

		entity token_value_to_entity(token_value_variant<char_t> v, bool neg)
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
						post_err(tk.position, syn_ec::invalid_operation, ::std::move(msg));
						return ::std::forward<arg_fwd_t>(arg);
					}
				}
				else
				{
					return ::std::forward<arg_fwd_t>(arg);
				}
				return 0;
			}, ::std::move(v));
		}

	protected:
		template <typename iter_t>
		auto get_token(iter_t&& begin, auto end)
		{
			return this->base_type::get_token(::std::forward<iter_t>(begin), end);
		}
		void post_err(source_position pos, auto error_code, error_msg_string&& str_for_msg_gen)
		{
			this->base_type::post_err(pos, error_code, ::std::move(str_for_msg_gen));
		}
		void reset_position_recorder()
		{
			this->base_type::reset_position_recorder();
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
		entity parse_file(::std::ifstream& source_file)
		{
			auto bom_type = unicode::read_bom(source_file);
			auto source_encode_type = unicode::utility::to_encode_type(bom_type);
			auto source_swap_chain = make_swap_chain(source_file);
			using enum unicode::encode_type;

			switch (source_encode_type)
			{
			case utf_8:     return this->parse(make_code_unit_iterator<utf_8>    (source_swap_chain.current(), source_swap_chain.end()), source_swap_chain.end());
			case utf_16_le: return this->parse(make_code_unit_iterator<utf_16_le>(source_swap_chain.current(), source_swap_chain.end()), source_swap_chain.end());
			case utf_16_be: return this->parse(make_code_unit_iterator<utf_16_be>(source_swap_chain.current(), source_swap_chain.end()), source_swap_chain.end());
			case utf_32_le: return this->parse(make_code_unit_iterator<utf_32_le>(source_swap_chain.current(), source_swap_chain.end()), source_swap_chain.end());
			case utf_32_be: return this->parse(make_code_unit_iterator<utf_32_be>(source_swap_chain.current(), source_swap_chain.end()), source_swap_chain.end());
			default:        throw  inner_error{ "[pdn] inner error when process bom_type" };
			}
			return 0;
		}
	};
}

#endif
