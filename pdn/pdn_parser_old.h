#ifndef PDN_Header_pdn_parser
#define PDN_Header_pdn_parser

#include <utility>
#include <variant>
#include <limits>
#include <type_traits>
#include <concepts>

#include "pdn_type_code.h"
#include "pdn_type_code_to_type.h"
#include "pdn_type_generator_base.h"
#include "pdn_type_generator.h"
#include "pdn_unicode_base.h"
#include "pdn_lexer.h"
#include "pdn_types.h"
#include "pdn_token.h"
#include "pdn_token_code.h"
#include "pdn_type_config.h"

#include <iostream>

namespace pdn
{
	template <
		typename char_t = unicode::utf_8_code_unit_t,
		concepts::source_position_recorder source_position_recorder_t = source_position_recorder,
		typename type_config = default_type_config<char_t>>
	class parser : public lexer<char_t, source_position_recorder_t>
	{
		using base_type = lexer<char_t, source_position_recorder_t>;
		using tkn = token<char_t>;
		using ent = typename type_config::entity;
		using obj = typename type_config::object;
		using str = typename type_config::string;
		using list = typename type_config::list;
		using pobj = proxy<obj>;
		using pstr = proxy<str>;
		using plist = proxy<list>;
		using character = types::character<char_t>;

	private:
		type_generator<char_t> type_gen{ type_generator_std<char_t> };
		token<char_t> tk{};
	public:
		void test(auto&& begin, auto end)
		{
			obj o;
			parse(begin, end, o);
			std::cout << "END OF PARSER TEST\n";
		}
		template <bool f_t_c = false>
		auto get_token(auto&& begin, auto end)
		{
			return this->base_type::template get_token<f_t_c>(begin, end);
		}
		void parse(auto&& begin, auto end, obj& o)
		{
			using enum pdn_token_code;

			

			for (; ; )
			{
				parse_start(begin, end, o);
				if (tk.code == eof)
				{
					break;
				}
				// post error
			}
			
		}
	public:
		parser(
			error_handler err_handler,
			error_message_generator err_msg_gen = error_message_generator_en,
			constants_generator<char_t> constants_gen = constants_generator_std<char_t>,
			type_generator<char_t> type_gen = type_generator_std<char_t>) :
			lexer<char_t, source_position_recorder_t>(::std::move(err_handler), ::std::move(err_msg_gen), ::std::move(constants_gen)),
			type_gen{ ::std::move(type_gen) }
		{}
	private:
		void parse_start(auto&& begin, auto end, obj& o)
		{
			using enum pdn_token_code;
			
			for (tk = get_token(begin, end); ; )
			{
				if (tk.code == identifier)
				{
					auto iden_p = ::std::get<proxy<types::string<char_t>>>(::std::move(tk.value));
					o[*iden_p] = parse_decl(begin, end, o);
				}
				else
				{
					// post expect identifier
					break;
				}
			}
			return;
		}

		ent parse_decl(auto&& begin, auto end, obj& o)
		{
			// ... iden CURRPOS ...
			using enum pdn_token_code;

			tk = get_token(begin, end);
			type_code type_c = type_code::unknown;

			if (tk.code == colon)
			{
				type_c = parse_type_spec(begin, end, o);
				// ... iden : iden CURRPOS ...
			}

			// tips expect express
			ent e = parse_expr(begin, end, o);
			tk = get_token(begin, end); // to ... iden <type_spec> <expr> CURRPOS ...
			
			if (type_c != type_code::unknown)
			{
				e = entity_cast(::std::move(e), type_c);
			}

			return e;
		}

		type_code parse_type_spec(auto&& begin, auto end, obj& o)
		{
			// ... iden : CURRPOS ...
			using enum pdn_token_code;

			tk = get_token(begin, end);

			if (tk.code == identifier)
			{
				auto type_c = type_gen(*::std::get<proxy<str>>(tk.value));

				if (type_c == type_code::unknown)
				{
					// post no type named get<proxy<str>>(tk.value)
				}

				tk = get_token(begin, end); // to ... iden : iden POS ...

				return type_c;
			}

			// post expect identifier

			return type_code::unknown;
		}

		ent parse_expr(auto&& begin, auto end, obj& o)
		{
			//    ... iden : iden CURRPOS ...
			// or ... iden CURRPOS ...

			using enum pdn_token_code;

			// tips expect express

			::std::size_t sign_count{ 0 };
			bool negative_sign{ false };

			for (; tk.code == minus || tk.code == plus; tk = get_token(begin, end))
			{
				if (tk.code == minus)
				{
					negative_sign = !negative_sign;
				}
				++sign_count;
			}

			switch (tk.code)
			{
			case literal_boolean:
			case literal_character:
			case literal_string:
				if (sign_count > 0)
				{
					// TODO post no +|- operation to operand bool|character|string
				}
				[[fallthrough]];
			case literal_floating_point:
			case literal_integer:
				return token_value_to_entity(tk.value, negative_sign);
				break;
			case left_brackets:
				
				break;
			case left_curly_brackets:

				break;
			default:
				break;
			}

			return 0;
		}
		
		ent parse_list_expr(auto&& begin, auto end, obj& o)
		{
			// ... iden <type_spec> <expr> [ CURRPOS ...

			ent result = ::std::make_unique<list>();
			for (; ; )
			{
				
			}
		}

		ent entity_cast(ent src, type_code target_type_c)
		{
			switch (target_type_c)
			{
			case type_code::i8:
				break;
			default:
				break;
			}
			return 0;
		}

		template <type_code target_type_c>
		ent entity_cast_impl(ent src)
		{
			using target_t = type_code_to_type_t<target_type_c, type_config>;
			// 需要一个 type_code to error_msg_string
			return ::std::visit([&](auto&& arg) -> ent
			{
				using src_t = ::std::decay_t<decltype(arg)>;
				using tar_t = target_t;

				if constexpr (types::concepts::pdn_int<src_t>)
				{
					if constexpr (types::concepts::pdn_int<tar_t>)
					{
						if (arg > ::std::numeric_limits<tar_t>::max())
						{
							// TODO post error
						}
						else if (arg < ::std::numeric_limits<tar_t>::min())
						{
							// TODO post error
						}
						return tar_t(arg);
					}
					else if constexpr (types::concepts::pdn_uint<tar_t>)
					{
						using u_src_t = ::std::make_unsigned_t<src_t>;
						if (src < 0)
						{
							// system error uint::min < 0 is impossible, if pdn::types::concept::pdn_uint works
							static_assert(::std::numeric_limits<tar_t>::min() < 0, "[pdn] error");
							// TODO post error
						}
						else if (u_src_t(arg) > ::std::numeric_limits<tar_t>::max())
						{
							// TODO post error
						}
						return tar_t(arg);
						// sint to uint, check src >= 0, cast src to unsigned (make_unsigned)
						// cast
						// return
					}
					else if constexpr (types::concepts::pdn_fp<tar_t>)
					{
						// cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, types::boolean>)
					{
						// cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, character>)
					{
						// sint to char, bad cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, pstr>)
					{
						// sint to string, bad cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, pobj>)
					{
						// sint to object, bad cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, plist>)
					{
						// sint to list, bad cast
						// return
					}
					else
					{
						// post inner error
						// return
					}
					return arg;
				}
				else if constexpr (types::concepts::pdn_uint<src_t>)
				{
					if constexpr (types::concepts::pdn_int<tar_t>)
					{
						using u_tar_t = ::std::make_unsigned_t<tar_t>;
						// system error uint::min < 0 or sint::min >= 0 is impossible,
						// if pdn::types::concept::pdn_uint|pdn_int works
						static_assert(::std::numeric_limits<src_t>::min() >= 0, "");
						static_assert(::std::numeric_limits<tar_t>::min() < 0, "");
						if (arg > u_tar_t(::std::numeric_limits<tar_t>::max()))
						{
							// TODO post error
						}

						// uint to sint, cast numeric_limits<tar_t>::max() to unsigned (make_unsigned)
						// cast
						// return
					}
					else if constexpr (types::concepts::pdn_uint<tar_t>)
					{
						if (arg > ::std::numeric_limits<tar_t>::max())
						{
							// TODO post error
						}
						else if (arg < ::std::numeric_limits<tar_t>::min())
						{
							// TODO post error
						}
						return tar_t(arg);
						// uint to uint
						// cast
						// return
					}
					else if constexpr (types::concepts::pdn_fp<tar_t>)
					{
						// cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, types::boolean>)
					{
						// cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, character>)
					{
						// uint to char, bad cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, pstr>)
					{
						// uint to string, bad cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, pobj>)
					{
						// uint to object, bad cast
						// return
					}
					else if constexpr (::std::same_as<tar_t, plist>)
					{
						// uint to list, bad cast
						// return
					}
					else
					{
						// post inner error
						// return
					}
					return arg;
				}
				// src=sint			done
				// src=uint			done
				// src=fp
				// src=boolean
				// src=character
				// src=pstr
				// src=plist
				// src=pobj

				return arg;
			}, src);
		}


		ent token_value_to_entity(token_value_variant<char_t> v, bool neg)
		{
			return std::visit([&]<typename arg_t>(arg_t&& arg) -> ent
			{
				using decay_arg_t = ::std::decay_t<decltype(arg)>;
				if constexpr (
					   types::concepts::pdn_fp<decay_arg_t>
					|| types::concepts::pdn_int<decay_arg_t>)
				{
					return neg ? ent{ -arg } : ent{ ::std::forward<arg_t>(arg) };
				}
				else if constexpr (types::concepts::pdn_uint<decay_arg_t>)
				{
					if (neg)
					{
						// TODO post error
					}
					return arg;
				}
				else if constexpr (::std::is_same_v<decay_arg_t, ::std::monostate>)
				{
					// TODO post inner error
					return 0;
				}
				else
				{
					// TODO string type cast types::string<char_t> to str
					return arg;
				}
			}, ::std::move(v));
		}
	};
}

#endif
