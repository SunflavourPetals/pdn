#include <iostream>
#include <format>
#include <sstream>
#include <functional>
#include <utility>
#include <cstddef>
#include <vector>
#include <variant>
#include <chrono>

#include "pdn_unicode.h"
#include "pdn_code_convert.h"

#include "pdn_bom_reader.h"
#include "pdn_swap_chain.h"
#include "pdn_code_unit_iterator.h"
#include "pdn_code_point_iterator.h"

#include "pdn_source_position_and_newline_recorder.h"
#include "pdn_source_position_recorder.h"

#include "pdn_parser.h"
#include "pdn_make_slashes_string.h"
#include "pdn_error_code_variant_to_error_msg_string.h"

namespace parser_utf_config
{
	using my_char = pdn::unicode::utf_8::code_unit_t;
}

namespace pdn_test
{
	namespace ts = pdn::types;
	using header_t = std::variant<std::string, std::size_t>;
	namespace
	{
		std::vector<header_t> stack{};
		volatile std::size_t g_err_count{};
	}

	template <typename char_t>
	void print(std::ostream& out,
	           const pdn::dom<char_t>& ev,
	           const std::size_t max_layer = 3,
	           const std::size_t layer = 0)
	{
		using dom_t = pdn::dom<char_t>;

		auto print_layer = [&]() { for (std::size_t i = 0; i < layer; ++i) out << "    "; };

		std::visit([&](const auto& arg) {
			using arg_t = std::decay_t<decltype(arg)>;
			using namespace pdn::types::concepts;
			if constexpr (pdn_integral<arg_t> || pdn_fp<arg_t>)
			{
				if constexpr (std::same_as<arg_t, typename ts::i8>)
				{
					out << ", value: " << (int)arg << "\n";
				}
				else if constexpr (std::same_as<arg_t, typename ts::u8>)
				{
					out << ", value: " << (unsigned)arg << "\n";
				}
				else
				{
					out << ", value: " << arg << "\n";
				}
			}
			else if constexpr (pdn_bool<arg_t>)
			{
				out << ", value: " << (arg ? "true" : "false") << "\n";
			}
			else if constexpr (std::same_as<arg_t, typename dom_t::character>)
			{
				auto raw_char_s = pdn::make_slashes_string<pdn::unicode::utf_8_code_unit_string>(
					pdn::unicode::code_convert<pdn::unicode::code_point_string>(arg.to_string_view()));

				out << ", value: " << "\'" << std::string_view{ (const char*)raw_char_s.data(), raw_char_s.size() } << "\'" << "\n";
			}
			else if constexpr (std::same_as<arg_t, typename dom_t::string_proxy>)
			{
				auto s = pdn::make_slashes_string<pdn::unicode::utf_8_code_unit_string>(
					pdn::unicode::code_convert<pdn::unicode::code_point_string>(*arg));

				out << ", value: " << "\"" << std::string_view{ (const char*)s.data(), s.size() } << "\"" << "\n";
			}
			else if constexpr (std::same_as<arg_t, typename dom_t::list_proxy>)
			{
				if (!arg)
				{
					std::cout << "null proxy for list\n";
				}
				if (layer > 0)
				{
					out << ", size: " << arg->size() << "\n";
				}
				if (max_layer == 0)
				{
					return;
				}
				std::size_t index = 0;
				for (const auto& e : *arg)
				{
					auto type_s = std::visit([](auto& e) {
						using arg_t = std::decay_t<decltype(e)>;
						using pdn_e_t = pdn::type_traits::remove_proxy_t<arg_t>;
						constexpr auto type_c = pdn::type_to_type_code_v<pdn_e_t, char_t>;
						auto type_em_s = pdn::type_code_to_error_msg_string(type_c);
						auto type_s = std::string((const char*)type_em_s.data(), type_em_s.size());
						return type_s;
						}, e);

					print_layer();
					out << "index: " << index++ << ", type: " << type_s;
					print<char_t>(out, e, max_layer - 1, layer + 1);
				}
			}
			else if constexpr (std::same_as<arg_t, typename dom_t::object_proxy>)
			{
				if (!arg)
				{
					std::cout << "null proxy for list\n";
				}
				if (layer > 0)
				{
					out << ", size: " << arg->size() << "\n";
				}
				if (max_layer == 0)
				{
					return;
				}
				for (const auto& e : *arg)
				{
					auto id_raw_s = pdn::make_slashes_string<pdn::unicode::utf_8_code_unit_string>(
						pdn::unicode::code_convert<pdn::unicode::code_point_string>(e.first));

					std::string id((const char*)id_raw_s.data(), id_raw_s.size());
					
					auto type_s = std::visit([](auto& e)
					{
						using arg_t = std::decay_t<decltype(e)>;
						using pdn_e_t = pdn::type_traits::remove_proxy_t<arg_t>;
						constexpr auto type_c = pdn::type_to_type_code_v<pdn_e_t, char_t>;
						auto type_em_s = pdn::type_code_to_error_msg_string(type_c);
						auto type_s = std::string((const char*)type_em_s.data(), type_em_s.size());
						return type_s;
					}, e.second);

					print_layer();
					out << "id: \"" << id << "\", type: " << type_s;
					print<char_t>(out, e.second, max_layer - 1, layer + 1);
				}
			}
		}, ev);
	};

	template <typename char_t>
	void print_member(std::ostream& out,
	                  const pdn::dom<char_t>& dom,
	                  const pdn::types::string<char_t>& id,
	                  const std::size_t max_layer = 3)
	{
		auto op = dom.object_ptr();
		if (!op)
		{
			out << "[!] this is not an object\n";
			return;
		}
		auto& o = *op;
		auto it = o.find(id);
		auto id_o = pdn::make_slashes_string<std::u8string>(pdn::unicode::code_convert<pdn::unicode::code_point_string>(id));
		auto id_slashes = std::string_view{ (const char*)id_o.data(), (const char*)(id_o.data() + id_o.size()) };
		if (it == o.end())
		{
			out << "[!] this have no member which named: \"" << id_slashes << "\"\n";
			return;
		}
		auto type_s = std::visit([](auto& e)
		{
			using arg_t = std::decay_t<decltype(e)>;
			using pdn_e_t = pdn::type_traits::remove_proxy_t<arg_t>;
			constexpr auto type_c = pdn::type_to_type_code_v<pdn_e_t, char_t>;
			auto type_em_s = pdn::type_code_to_error_msg_string(type_c);
			auto type_s = std::string((const char*)type_em_s.data(), type_em_s.size());
			return type_s;
		}, it->second);

		out << "id: \"" << id_slashes << "\", type: " << type_s;
		print<char_t>(out, it->second, max_layer, 1);
	}

	template <typename char_t>
	void print_element(std::ostream& out,
	                   const pdn::dom<char_t>& dom,
	                   const std::size_t index,
	                   const std::size_t max_layer = 3)
	{
		auto lp = dom.list_ptr();
		if (!lp)
		{
			out << "[!] this is not a list\n";
			return;
		}
		auto& list_v = *lp;
		if (index >= list_v.size())
		{
			out << "[!] index out of range: [0, " << list_v.size() << ")\n";
			return;
		}
		auto& element = list_v[index];

		auto type_s = std::visit([](auto& e)
		{
			using arg_t = std::decay_t<decltype(e)>;
			using pdn_e_t = pdn::type_traits::remove_proxy_t<arg_t>;
			constexpr auto type_c = pdn::type_to_type_code_v<pdn_e_t, char_t>;
			auto type_em_s = pdn::type_code_to_error_msg_string(type_c);
			auto type_s = std::string((const char*)type_em_s.data(), type_em_s.size());
			return type_s;
		}, element);

		out << "index: " << index << ", type: " << type_s;
		print<char_t>(out, element, max_layer, 1);
	}

	struct error_handler_t
	{
		std::size_t err_count = 0;
		std::ostream& log;
		error_handler_t(std::ostream& l) : log{ l } {}
		void operator()(const pdn::error_message& e)
		{
			++err_count;
			g_err_count = err_count;
			auto error_type_s = pdn::error_code_variant_to_error_msg_string(e.error_code);
			log << std::format("{}({}:{}) {}\n",
			                   (const char*)error_type_s.c_str(),
			                   e.position.line,
			                   e.position.column,
			                   (const char*)e.error_message.c_str());
		}
	};

	using namespace pdn::unicode_literals;
	template <typename char_t>
	struct cmd_table
	{
		inline static auto clear = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"clear"_ucusv);
		inline static auto cls   = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"cls"_ucusv);
		inline static auto pos   = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"pos"_ucusv);
		inline static auto p     = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"p"_ucusv);
		inline static auto exit  = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"exit"_ucusv);
		inline static auto quit  = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"quit"_ucusv);
		inline static auto ls    = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"ls"_ucusv);
		inline static auto bk    = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"bk"_ucusv);
		inline static auto cd    = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"cd"_ucusv);
		inline static auto help  = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"help"_ucusv);
		inline static auto mb    = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"mb"_ucusv);
		inline static auto el    = pdn::unicode::code_convert<pdn::types::string<char_t>>(u8"el"_ucusv);
	};

	namespace
	{
		template <typename char_t>
		pdn::lexer<char_t> get_lex(std::ostream& o)
		{
			pdn::lexer<char_t> lex{ error_handler_t{ o } };
			return lex;
		}
		void print_prompt(std::ostream& out)
		{
			if (stack.empty())
			{
				out << "<root>: ";
			}
			else
			{
				std::visit([&](const auto& arg)
				{
					using arg_t = std::decay_t<decltype(arg)>;
					if constexpr (std::same_as<arg_t, std::string>)
					{
						out << "[" << arg << "]: ";
					}
					else if constexpr (std::same_as<arg_t, std::size_t>)
					{
						out << arg << ": ";
					}
					else
					{
						static_assert(false);
					}
				}, stack.back());
			}
		}
		void print_stack(std::ostream& out)
		{
			out << "    <root>";
			for (auto& e : stack)
			{
				std::visit([&](const auto& arg)
				{
					using arg_t = std::decay_t<decltype(arg)>;
					if constexpr (std::same_as<arg_t, std::string>)
					{
						out << "." << arg;
					}
					else if constexpr (std::same_as<arg_t, std::size_t>)
					{
						out << "[" << arg << "]";
					}
					else
					{
						static_assert(false);
					}
				}, e);
			}
			out << "\n";
		}
		std::u8string_view reinterpret_to_u8sv(std::string& s)
		{
			return { (const char8_t*)s.data(), (const char8_t*)(s.data() + s.size()) };
		}
		std::string_view reinterpret_to_sv(std::u8string& s)
		{
			return { (const char*)s.data(), (const char*)(s.data() + s.size()) };
		}
		template <typename char_t>
		ts::string<char8_t> convert_to_u8s(std::basic_string_view<char_t> sv)
		{
			return pdn::unicode::code_convert<ts::string<char8_t>>(sv);
		}
		template <typename char_t>
		ts::string<char8_t> convert_to_u8s(const ts::string<char_t>& s)
		{
			return pdn::unicode::code_convert<ts::string<char8_t>>(s);
		}
		template <typename char_t>
		bool is_iden(const pdn::token<char_t>& tk)
		{
			return tk.code == pdn::pdn_token_code::identifier;
		}
		template <typename char_t>
		bool is_integral(const pdn::token<char_t>& tk)
		{
			return tk.code == pdn::pdn_token_code::literal_integer;
		}
		template <typename char_t>
		bool is_end(const pdn::token<char_t>& tk)
		{
			return tk.code == pdn::pdn_token_code::eof;
		}
		template <typename char_t>
		bool is_not_end(const pdn::token<char_t>& tk)
		{
			return !is_end(tk);
		}
		template <typename char_t>
		bool get_string(const pdn::token<char_t>& tk, ts::string<char_t>& dest)
		{
			auto prp = ::std::get_if<pdn::proxy<ts::string<char_t>>>(&tk.value);
			if (!prp || !(*prp))
			{
				return false;
			}
			dest = **prp;
			return true;
		}
		template <typename char_t>
		bool get_int(const pdn::token<char_t>& tk, std::size_t& dest)
		{
			return std::visit([&](const auto& arg) -> bool
			{
				using arg_t = std::decay_t<decltype(arg)>;
				if constexpr (pdn::types::concepts::pdn_integral<arg_t>)
				{
					dest = arg;
					return true;
				}
				return false;
			}, tk.value);
		}
		template <typename char_t>
		void print_self_shortly(std::ostream& out, const pdn::dom<char_t>& dom)
		{
			if (auto p = dom.object_ptr())
			{
				out << "this type: object";
			}
			else if (auto p = dom.list_ptr())
			{
				out << "this type: list";
			}
			else
			{
				out << "[!] inner error this is not object or list\n";
			}
		}
	}

	template <typename char_t>
	void play(std::ostream& out, pdn::dom<char_t>& dom, std::size_t& exit_layer, std::size_t layer = 0)
	{
		for (; ; )
		{
			if (exit_layer > 0)
			{
				--exit_layer;
				return;
			}
			std::string cmd{};
			print_prompt(out);
			std::getline(std::cin, cmd);
			auto cmd_v = reinterpret_to_u8sv(cmd);
			auto lex = get_lex<char_t>(out);
			auto cp_it = pdn::make_code_point_iterator(cmd_v.begin(),
			                                           cmd_v.end(),
													   [&]() { return lex.position(); },
													   error_handler_t{ out });
			auto tk = lex.get_token(cp_it, cmd_v.end());

			if (is_iden(tk))
			{
				ts::string<char_t> cmd_str;
				if (!get_string(tk, cmd_str))
				{
					out << "[!] inner error cmd val is null\n";
					continue;
				}

				using ct = cmd_table<char_t>;

				if (cmd_str == ct::mb)
				{
					auto arg_tk = lex.get_token(cp_it, cmd_v.end());
					if (is_iden(arg_tk))
					{
						ts::string<char_t> arg_str;
						if (!get_string(arg_tk, arg_str))
						{
							out << "[!] inner error arg val is null\n";
							continue;
						}
						std::size_t max_layer{ 3 };
						auto layer_tk = lex.get_token(cp_it, cmd_v.end());
						if (is_not_end(layer_tk))
						{
							if (is_integral(layer_tk))
							{
								if (!get_int(layer_tk, max_layer))
								{
									out << "[!] inner error arg val is not integer\n";
									max_layer = 3;
								}
								auto end_tk = lex.get_token(cp_it, cmd_v.end());
								if (is_not_end(end_tk))
								{
									goto mb_redundant;
								}
							}
							else
							{
							mb_redundant:
								auto cmd_u8s = convert_to_u8s(cmd_str);
								out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
								out << "    usage of mb: mb <identifier> [depth optional default=3]\n";
							}
						}
						
						print_member(out, dom, arg_str, max_layer);
					}
					else
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						out << "[!] missing identifier arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						continue;
					}
				}
				else if (cmd_str == ct::el)
				{
					auto arg_tk = lex.get_token(cp_it, cmd_v.end());
					if (is_integral(arg_tk))
					{
						std::size_t index;
						if (!get_int(arg_tk, index))
						{
							out << "[!] inner error arg val is not integer\n";
							continue;
						}
						std::size_t max_layer{ 3 };
						auto layer_tk = lex.get_token(cp_it, cmd_v.end());
						if (is_not_end(layer_tk))
						{
							if (is_integral(layer_tk))
							{
								if (!get_int(layer_tk, max_layer))
								{
									out << "[!] inner error arg val is not integer\n";
									max_layer = 3;
								}
								auto end_tk = lex.get_token(cp_it, cmd_v.end());
								if (is_not_end(end_tk))
								{
									goto el_redundant;
								}
							}
							else
							{
							el_redundant:
								auto cmd_u8s = convert_to_u8s(cmd_str);
								out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
								out << "    usage of el: el <index> [depth optional default=3]\n";
							}
						}

						print_element(out, dom, index, max_layer);
					}
					else
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						out << "[!] missing index arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						continue;
					}
				}
				else if (cmd_str == ct::ls)
				{
					auto arg_tk = lex.get_token(cp_it, cmd_v.end());
					if (is_end(arg_tk))
					{
						print_self_shortly(out, dom);
						print(out, dom, 3, 1);
						continue;
					}
					else if (is_integral(arg_tk))
					{
						std::size_t i{};
						if (!get_int(arg_tk, i))
						{
							out << "[!] inner error arg val is not integer\n";
							continue;
						}
						print_self_shortly(out, dom);
						print(out, dom, i, 1);
						if (is_end(lex.get_token(cp_it, cmd_v.end())))
						{
							continue;
						}
					}
					auto cmd_u8s = convert_to_u8s(cmd_str);
					out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
					out << "    usage of ls: ls [depth optional default=3]\n";
					continue;
				}
				else if (cmd_str == ct::cd)
				{
					auto arg_tk = lex.get_token(cp_it, cmd_v.end());
					if (is_iden(arg_tk))
					{
						ts::string<char_t> arg_str;
						if (!get_string(arg_tk, arg_str))
						{
							out << "[!] inner error arg val is null\n";
							continue;
						}
						auto op = dom.object_ptr();
						if (!op)
						{
							out << "[!] this is not an object\n";
							continue;
						}
						auto& o = *op;
						auto& id = arg_str;
						auto it = o.find(id);
						auto id_o = pdn::make_slashes_string<std::u8string>(pdn::unicode::code_convert<pdn::unicode::code_point_string>(id));
						auto id_slashes = reinterpret_to_sv(id_o);
						if (it == o.end())
						{
							out << "[!] this have no member which named: \"" << id_slashes << "\"\n";
							continue;
						}
						bool is_object_or_list = std::visit([&](auto& e)
						{
							using arg_t = std::decay_t<decltype(e)>;
							using pdn_e_t = pdn::type_traits::remove_proxy_t<arg_t>;
							if constexpr (std::same_as<pdn_e_t, pdn::types::object<char_t>>)
							{
								return true;
							}
							else if constexpr (std::same_as<pdn_e_t, pdn::types::list<char_t>>)
							{
								return true;
							}
							return false;
						}, it->second);

						if (!is_object_or_list)
						{
							out << "[!] target entity: \"" << id_slashes << "\" is not an object or list\n";
							continue;
						}

						if (is_not_end(lex.get_token(cp_it, cmd_v.end())))
						{
							auto cmd_u8s = convert_to_u8s(cmd_str);
							out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						}

						stack.push_back(std::string(id_slashes.begin(), id_slashes.end()));
						play(out, it->second, exit_layer, layer + 1);
						stack.pop_back();
					}
					else if (is_integral(arg_tk))
					{
						std::size_t index{};
						if (!get_int(arg_tk, index))
						{
							out << "[!] inner error arg val is not integer\n";
							continue;
						}
						auto lp = dom.list_ptr();
						if (!lp)
						{
							out << "[!] this is not a list\n";
							continue;
						}
						auto& list_v = *lp;
						if (index >= list_v.size())
						{
							out << "[!] index out of range: [0, " << list_v.size() << ")\n";
							continue;
						}
						auto& element = list_v[index];
						bool is_object_or_list = std::visit([&](auto& e)
						{
							using arg_t = std::decay_t<decltype(e)>;
							using pdn_e_t = pdn::type_traits::remove_proxy_t<arg_t>;
							if constexpr (std::same_as<pdn_e_t, pdn::types::object<char_t>>)
							{
								return true;
							}
							else if constexpr (std::same_as<pdn_e_t, pdn::types::list<char_t>>)
							{
								return true;
							}
							return false;
						}, element);

						if (!is_object_or_list)
						{
							out << "[!] target entity: [" << index << "] is not an object or list\n";
							continue;
						}

						if (is_not_end(lex.get_token(cp_it, cmd_v.end())))
						{
							auto cmd_u8s = convert_to_u8s(cmd_str);
							out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						}

						stack.push_back(index);
						play(out, element, exit_layer, layer + 1);
						stack.pop_back();
					}
					else
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						if (auto p = dom.object_ptr())
						{
							out << "[!] missing identifier arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						}
						else if (auto p = dom.list_ptr())
						{
							out << "[!] missing index arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						}
						else
						{
							out << "[!] missing arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						}
						continue;
					}
				}
				else if (cmd_str == ct::p || cmd_str == ct::pos)
				{
					if (auto ntk = lex.get_token(cp_it, cmd_v.end()); is_not_end(ntk))
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						// do not continue
					}
					print_stack(out);
				}
				else if (cmd_str == ct::bk)
				{
					++exit_layer;
					for (auto ntk = lex.get_token(cp_it, cmd_v.end()); is_not_end(ntk); ntk = lex.get_token(cp_it, cmd_v.end()))
					{
						if (is_iden(ntk))
						{
							ts::string<char_t> next_str;
							if (!get_string(tk, next_str))
							{
								out << "[!] inner error cmd val is null\n";
								break;
							}

							if (next_str == ct::bk)
							{
								++exit_layer;
							}
							else
							{
								auto cmd_u8s = convert_to_u8s(next_str);
								out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
								break;
							}
						}
					}
					continue;
				}
				else if (cmd_str == ct::cls || cmd_str == ct::clear)
				{
					if (auto ntk = lex.get_token(cp_it, cmd_v.end()); is_not_end(ntk))
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
					}
#ifdef _WIN32
					std::system("cls");
#else
					std::system("clear");
#endif
				}
				else if (cmd_str == ct::exit)
				{
					exit_layer = std::numeric_limits<std::size_t>::max();
					if (auto ntk = lex.get_token(cp_it, cmd_v.end()); is_not_end(ntk))
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						out << "    usage of exit: exit\n";
					}
					return;
				}
				else if (cmd_str == ct::quit)
				{
					exit_layer = std::numeric_limits<std::size_t>::max();
					if (auto ntk = lex.get_token(cp_it, cmd_v.end()); is_not_end(ntk))
					{
						auto cmd_u8s = convert_to_u8s(cmd_str);
						out << "[!] redundant arguments after: \"" << reinterpret_to_sv(cmd_u8s) << "\"\n";
						out << "    usage of quit: quit\n";
					}
					return;
				}
				else if (cmd_str == ct::help)
				{
					out << "Help:\n"
						<< "commands and usages:\n"
						<< "\thelp : ------------------------------ get help\n"
						<< "\texit | quit : ----------------------- exit | quit\n"
						<< "\tclear | cls : ----------------------- clear screen\n"
						<< "\tpos | p : --------------------------- get index of current object\n"
						<< "\tls [depth opt default=3] : ---------- list members | elements for this pos\n"
						<< "\tmb <iden> [depth opt default=3] : --- list members | elements for this.iden\n"
						<< "\tel <index> [depth opt default=3] : -- list members | elements for this[index]\n"
						<< "\tcd <iden>|<index> : ----------------- move to this.iden|this[index]\n"
						<< "\tbk (bk)* : -------------------------- move back...\n";
				}
				else
				{
					auto cmd_u8s = convert_to_u8s(cmd_str);
					out << "[!] \"" << reinterpret_to_sv(cmd_u8s) << "\" is not a command\n";
				}
			}
			else if (tk.code == pdn::pdn_token_code::eof)
			{
				continue;
			}
			else
			{
				out << "[!] requires command is an identifier\n";
			}
		}
	}

	template <typename char_t>
	void test(pdn::parser<char_t>& parser, const std::string& filename, std::ostream& out, bool to_play = false)
	{
		auto prev_parse = std::chrono::high_resolution_clock::now();
		auto dom = parser.parse(filename);
		auto after_parse = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> time_cost = after_parse - prev_parse;

		std::cout << "parse over with " << g_err_count << " error(s)\n";
		std::cout << "time cost: " << time_cost << "\n";
		
		if (to_play)
		{
			std::cout << "Type help and press enter to get help.\n";
			std::size_t exit_layer{};
			play(std::cout, dom, exit_layer);
		}
		else
		{
			print<char_t>(out, dom);
		}
	}
}

int main(int argc, const char* argv[])
{
	std::string filename{};
	std::string out_filename{};
	std::string log_filename{};
	
	bool is_using_stdout = false;
	bool to_play = true;

	const char* my_argv[]{ "./pdn", "-p", R"(D:\Works\VisualStudio2022\ProjectsUsingGit\Maze\Maze\assets\pdn\level_1.pdn)" };
	{
		std::string answer{};
		std::cout << "do debug test[yes/y = yes, else = no]: ";
		std::cin >> answer;

		for (auto& c : answer)
		{
			c = std::tolower(c);
		}
		if (answer == "y" || answer == "yes")
		{
			argc = 3;
			argv = my_argv;
		}
	}

	if (argc <= 1)
	{
		std::cout << "no arguments.\n" << "using \"./pdn -help\" for help.";
		return 0;
		std::cout << "please input filename [default=\"source.pdn\"]: ";
		std::getline(std::cin, filename);
		if (filename == std::string{})
		{
			filename = "source.pdn";
		}
		out_filename = filename + ".pdn-parser-out.txt";
		log_filename = filename + ".pdn-log.txt";
	}
	else if (argc == 2 && argv[1][0] != '-')
	{
		to_play = true;
		filename = argv[1];
		out_filename = filename + ".pdn-parser-out.txt";
		log_filename = filename + ".pdn-log.txt";
	}
	else
	{
		using namespace std::string_literals;
		int curr_cmd = 1;
		bool showed_ver = false;
		bool showed_help = false;
		while (argc >= curr_cmd + 1)
		{
			std::string cmd = argv[curr_cmd++];
			if (cmd == "-o"s)
			{
				if (argc >= curr_cmd + 1)
				{
					filename = argv[curr_cmd++];
					out_filename = filename + ".pdn-parser-out.txt";
					log_filename = filename + ".pdn-log.txt";
				}
				else
				{
					std::cout << "command -o requires following argument <filename>\n";
				}
			}
			else if (cmd == "-p"s)
			{
				to_play = true;
				if (argc >= curr_cmd + 1)
				{
					filename = argv[curr_cmd++];
					out_filename = filename + ".pdn-parser-out.txt";
					log_filename = filename + ".pdn-log.txt";
				}
				else
				{
					std::cout << "command -p requires following argument <filename>\n";
				}
			}
			else if (cmd == "-v"s)
			{
				if (!showed_ver)
				{
					std::cout << "Pdn Parser Lib Version:  Beta   1.0\n";
					std::cout << "PdnShell Version:        Alpha  0.3\n";
					showed_ver = true;
				}
			}
			else if (cmd == "-stdout")
			{
				is_using_stdout = true;
			}
			else if (cmd == "-help")
			{
				if (!showed_help)
				{
					std::cout
						<< "Help\n"
						<< "-o <filename> : parse <filename> and output to <filename>.pdn-parser-out.txt\n"
						<< "-p <filename> : parse <filename> and play\n"
						<< "-stdout       : output to terminal when <filename> parsed by -o command\n"
						<< "-v            : show pdn version\n"
						<< "--------------+\n"
						<< "for example:\n"
						<< "    ./pdn -o source.pdn\n"
						<< "    ./pdn -o data.pdn -stdout\n"
						<< "    ./pdn -v\n";
					showed_help = true;
				}
			}
			else
			{
				std::cout << "unknown command: \"" << cmd << "\"\n";
			}
		}
	}
	if (filename == "") return 0;
	
	std::ofstream out_file;
	std::ofstream log_file;

	if (!is_using_stdout && !to_play)
	{
		out_file.open(out_filename);
		if (!out_file.is_open())
		{
			std::cout << "failed in open file: " << out_filename << "\"\n";
			std::terminate();
		}
		log_file.open(log_filename);
		if (!log_file.is_open())
		{
			std::cout << "failed in open file: " << log_filename << "\"\n";
			std::terminate();
		}
	}
	std::ostream& out = (is_using_stdout || to_play) ? std::cout : out_file;
	std::ostream& log = (is_using_stdout || to_play) ? std::cout : log_file;

	pdn_test::error_handler_t err_handler{ log };

	pdn::parser<parser_utf_config::my_char> parser(err_handler);

	try
	{
		pdn_test::test(parser, filename, out, to_play);
	}
	catch (const pdn::runtime_error& e)
	{
		std::cout << "pdn runtime_error: " << e.what() << "\n";
	}
	catch (const std::exception& e)
	{
		std::cout << "std exception: " << e.what() << "\n";
	}
	return 0;
}
