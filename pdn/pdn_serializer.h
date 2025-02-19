#ifndef PDN_Header_pdn_serializer
#define PDN_Header_pdn_serializer

#include <type_traits>
#include <concepts>
#include <utility>
#include <string>
#include <format>
#include <limits>
#include <cstddef>
#include <variant>
#include <cmath>

#include "pdn_types.h"
#include "pdn_data_entity.h"

#include "pdn_make_slashes_string.h"
#include "pdn_type_to_type_code.h"
#include "pdn_type_code_to_error_msg_string.h"

#include "pdn_unicode.h"
#include "pdn_code_convert.h"

#include "pdn_proxy.h"

#include "pdn_lexer_utility.h"

namespace pdn::dev_util
{
	template <typename t>
	concept can_to_string = types::concepts::pdn_integral<t> || types::concepts::pdn_fp<t>;
	template <typename t, typename char_t>
	concept nonlist_nonobj = types::concepts::basic_types<t, char_t> || ::std::same_as<t, types::string<char_t>>;

	template <typename char_t>
	auto to_pdn_format(types::concepts::pdn_integral auto val) -> types::string<char_t>
	{
		auto result = types::string<char_t>{};
		
		for (const auto c : ::std::format("{}", val))
		{
			result += char_t(c);
		}
		return result;
	}

	template <typename char_t>
	auto to_pdn_format(types::concepts::pdn_fp auto val) -> types::string<char_t>
	{
		auto result = types::string<char_t>{};
		{
			using c = char_t;
			if (::std::isinf(val))
			{
				return (::std::signbit(val)) ?
					types::string<char_t>{ c('-'), c('@'), c('i'), c('n'), c('f') } :
					types::string<char_t>{ c('@'), c('i'), c('n'), c('f') };
			}
			else if (::std::isnan(val))
			{
				return types::string<char_t>{ c('@'), c('N'), c('a'), c('N') };
			}
		}
		bool has_e{};
		bool has_dot{};
		for (const auto c : ::std::format("{}", val))
		{
			result += char_t(c);
			if (c == 'e') has_e = true;
			if (c == '.') has_dot = true;
		}
		if (!has_e && !has_dot)
		{
			result.push_back(char_t('.')); // make sure that it is a floating-point format
			result.push_back(char_t('0'));
		}
		return result;
	}

	template <typename char_t>
	auto to_pdn_format(types::boolean val) -> types::string<char_t>
	{
		using c = char_t;
		auto result = val ?
			types::string<c>{ c('@'), c('t'), c('r'), c('u'), c('e') } :
			types::string<c>{ c('@'), c('f'), c('a'), c('l'), c('s'), c('e') };
		return result;
	}

	template <typename char_t, typename src_char_t>
	auto to_pdn_format(types::character<src_char_t> val) -> types::string<char_t>
	{
		auto get_quoted = [](::std::basic_string_view<char_t> sv) -> types::string<char_t>
		{
			auto result = types::string<char_t>{};
			result.push_back(char_t('\''));
			result += sv;
			result.push_back(char_t('\''));
			return result;
		};
		auto src_sv = val.to_string_view();
		auto decode_result = unicode::decode<true>(src_sv.cbegin(), src_sv.cend());
		if (!decode_result)
		{
			return get_quoted(unicode::utility::get_rep_char<char_t>());
		}
		return get_quoted(make_slashes_string<types::string<char_t>>(decode_result.value()));
	}

	template <typename char_t, typename src_char_t>
	auto to_pdn_format(const types::string<src_char_t>& val) -> types::string<char_t>
	{
		auto get_quoted = [](::std::basic_string_view<char_t> sv) -> types::string<char_t>
		{
			auto result = types::string<char_t>{};
			result.push_back(char_t('\"'));
			result += sv;
			result.push_back(char_t('\"'));
			return result;
		};
		auto cp_s = unicode::code_convert<unicode::code_point_string>(val);
		return get_quoted(make_slashes_string<types::string<char_t>>(cp_s));
	}
}

namespace pdn
{
	template <unicode::concepts::code_unit my_char_t = char8_t>
	class serializer
	{
	public:
		using char_t = my_char_t; // target char type
		using string_t = types::string<char_t>;
		using string_view_t = ::std::basic_string_view<char_t>;
		// dom to .spdn format
		template <typename src_char_t>
		auto serialize(
			const types::object<src_char_t>& dom,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			string_t result{};
			if (dom.empty())
			{
				return result;
			}
			for (const auto& e : dom)
			{
				result += serialize_object_member(e.first, e.second, 0, use_line_feed, use_last_sep_for_lo);
				if (use_line_feed) result += char_t('\n');
				else               result += char_t(' ');
			}
			return result;
		}
		// to "[`]slashed_iden[`] [: [type]] pdn_form[;]"
		template <typename src_char_t, dev_util::nonlist_nonobj<src_char_t> value_t>
		auto serialize(const types::string<src_char_t>& iden, const value_t& val) const -> string_t
		{
			using val_t = ::std::decay_t<decltype(val)>;
			string_t result = serialize_iden(iden);
			constexpr auto no_type_spec
				 = ::std::same_as<val_t, types::auto_int>
				|| ::std::same_as<val_t, types::f64>
				|| ::std::same_as<val_t, types::boolean>
				|| ::std::same_as<val_t, types::character<src_char_t>>
				|| ::std::same_as<val_t, types::string<src_char_t>>;
			if constexpr (no_type_spec)
			{
				result += separator;
				result += serialize_value<src_char_t>(val);
			}
			else
			{
				result += separator_for_ts;
				constexpr auto type_c = type_to_type_code_v<val_t, src_char_t>;
				result += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
				result += char_t(' ');
				result += serialize_value<src_char_t>(val);
			}
			result += last_sep;
			return result;
		}
		// to "[`]slashed_iden[`] [: [type]] { pdn_form_of_members }[;]"
		template <typename src_char_t>
		auto serialize(
			const types::string<src_char_t>& iden,
			const types::object<src_char_t>& val,
			const ::std::size_t layer = 0,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			string_t result = serialize_iden(iden);
			result += object_separator;
			result += serialize_object(val, layer, use_line_feed, use_last_sep_for_lo);
			if (use_last_sep_for_lo) result += last_sep;
			return result;
		}
		// to "[`]slashed_iden[`] [: [type]] "[" pdn_form_of_elements "]"[;]"
		template <typename src_char_t>
		auto serialize(
			const types::string<src_char_t>& iden,
			const types::list<src_char_t>& val,
			const ::std::size_t layer = 0,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			string_t result = serialize_iden(iden);
			result += list_separator;
			result += serialize_list(val, layer, use_line_feed, use_last_sep_for_lo);
			if (use_last_sep_for_lo) result += last_sep;
			return result;
		}
		// identifier to "[`]slashed[`]"
		template <typename src_char_t>
		auto serialize_iden(const types::string<src_char_t>& iden) const -> string_t
		{
			string_t result{};
			auto cp_s = unicode::code_convert<unicode::code_point_string>(iden);
			bool need_quote{};
			{
				if (cp_s.empty())
				{
					need_quote = true;
				}
				else if (!lexer_utility::is_allowed_as_first_char_of_identifier(cp_s[0]))
				{
					need_quote = true;
				}
				else
				{
					for (auto c : unicode::code_point_string_view{ cp_s.cbegin() + 1, cp_s.cend() })
					{
						if (!lexer_utility::is_allowed_in_identifier(c))
						{
							need_quote = true;
							break;
						}
					}
				}
			}
			auto slashes = make_slashes_id_string<string_t>(cp_s);
			
			if (need_quote)
			{
				result.push_back(char_t('`'));
				result += slashes;
				result.push_back(char_t('`'));
			}
			else
			{
				result = ::std::move(slashes);
			}
			return result;
		}
		// integral/floating/boolean/character/string to pdn_form
		template <typename src_char_t>
		auto serialize_value(dev_util::nonlist_nonobj<src_char_t> auto const& val) const -> string_t
		{
			return dev_util::to_pdn_format<char_t>(val);
		}
		// object to "{ ... }"
		template <typename src_char_t>
		auto serialize_object(
			const types::object<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			string_t result{ char_t('{') };
			if (val.empty())
			{
				result += char_t('}');
				return result;
			}
			const auto inner_layer = layer + 1;
			for (const auto& e : val)
			{
				if (use_line_feed)
				{
					result += char_t('\n');
					result += gen_tabs(inner_layer);
				}
				result += serialize_object_member(e.first, e.second, inner_layer, use_line_feed, use_last_sep_for_lo);
				if (!use_line_feed) result += char_t(' ');
			}
			if (use_line_feed)
			{
				result += char_t('\n');
				result += gen_tabs(layer);
			}
			result += char_t('}');
			return result;
		}
		// object member to "name [:[type]] value [;]..."
		template <typename src_char_t>
		auto serialize_object_member(
			const types::string<src_char_t>& iden,
			const data_entity<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			return ::std::visit([&]<typename arg_pr_t>(const arg_pr_t& arg) -> string_t
			{
				using arg_t = remove_proxy_t<arg_pr_t>;
				if constexpr (dev_util::nonlist_nonobj<arg_t, src_char_t>)
				{
					constexpr bool has_proxy = !::std::same_as<arg_t, arg_pr_t>;
					if constexpr (has_proxy) return serialize(iden, *arg);
					else                     return serialize(iden,  arg);
				}
				else
				{
					return serialize(iden, *arg, layer, use_line_feed, use_last_sep_for_lo);
				}
			}, val);
		}
		// list to "[ ..., ]"
		template <typename src_char_t>
		auto serialize_list(
			const types::list<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			string_t result{ char_t('[') };
			if (val.empty())
			{
				result += char_t(']');
				return result;
			}
			const auto inner_layer = layer + 1;
			for (const auto& e : val)
			{
				if (use_line_feed)
				{
					result += char_t('\n');
					result += gen_tabs(inner_layer);
				}
				result += serialize_list_element(e, inner_layer, use_line_feed, use_last_sep_for_lo);
				result += char_t(',');
				if (!use_line_feed) result += char_t(' ');
			}
			if (use_line_feed)
			{
				result += char_t('\n');
				result += gen_tabs(layer);
			}
			result += char_t(']');
			return result;
		}
		// list element to "[type:] pdn_form,"
		template <typename src_char_t>
		auto serialize_list_element(
			const data_entity<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const -> string_t
		{
			return ::std::visit([&]<typename arg_pr_t>(const arg_pr_t& arg) -> string_t
			{
				using arg_t = remove_proxy_t<arg_pr_t>;
				if constexpr (::std::same_as<arg_t, types::object<src_char_t>>)
				{
					return serialize_object(*arg, layer, use_line_feed, use_last_sep_for_lo);
				}
				else if constexpr (::std::same_as<arg_t, types::list<src_char_t>>)
				{
					return serialize_list(*arg, layer, use_line_feed, use_last_sep_for_lo);
				}
				else if constexpr (::std::same_as<arg_t, types::string<src_char_t>>)
				{
					return serialize_value<src_char_t>(*arg);
				}
				else
				{
					string_t result{};
					constexpr auto no_type_spec
						 = ::std::same_as<arg_t, types::auto_int>
						|| ::std::same_as<arg_t, types::f64>
						|| ::std::same_as<arg_t, types::boolean>
						|| ::std::same_as<arg_t, types::character<src_char_t>>
						|| ::std::same_as<arg_t, types::string<src_char_t>>;
					if constexpr (no_type_spec)
					{
						result += serialize_value<src_char_t>(arg);
					}
					else
					{
						constexpr auto type_c = type_to_type_code_v<arg_t, src_char_t>;
						result += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
						result += separator_for_ts;
						result += serialize_value<src_char_t>(arg);
					}
					return result;
				}
			}, val);
		}
	private:
		auto gen_tabs(::std::size_t layer) const -> string_t
		{
			string_t result{};
			for (::std::size_t i{}; i != layer; ++i) result += tab;
			return result;
		}
	public:
		string_t tab             { char_t('\t') };
		string_t last_sep        { char_t(';') };              // ...;
		string_t separator       { char_t(':'), char_t(' ') }; // -> "name: data"
		string_t separator_for_ts{ char_t(':'), char_t(' ') }; // -> "name: type data" or "type: data" in list
		string_t list_separator  { char_t(' ') };              // list list_separator [...] -> "list [...]"
		string_t object_separator{ char_t(' ') };              // object object_separator {...} -> "object {...}"
	};

	enum class serialize_tab
	{
		table,   // "\t"
		space_1, // " "
		space_2, // "  "
		space_3, // "   "
		space_4, // "    "
		space_8, // "        "
		no_tab,  // ""
	};
	enum class serialize_last_semi
	{
		semi_yes, // ";"
		no_semi,  // ""
	};
	enum class serialize_separator
	{
		sep_semi_with_space, // ": "
		sep_semi, // ":"
		sep_space_around_semi, // " : "
		sep_space, // " "
	};
	enum class serialize_separator_ts
	{
		sts_semi_with_space, // ": "
		sts_semi, // ":"
		sts_space_around_semi, // " : "
	};

	namespace dev_util
	{
		template <typename c, typename e>
		auto to_string(e serialize_e) -> types::string<c>
		{
			using s = types::string<c>;
			if constexpr (::std::same_as<e, serialize_tab>)
			{
				using enum serialize_tab;
				switch (serialize_e)
				{
				case table:   return s{ c('\t') };
				case space_1: return s{ c(' ') };
				case space_2: return s{ c(' '), c(' ') };
				case space_3: return s{ c(' '), c(' '), c(' ') };
				case space_4: return s{ c(' '), c(' '), c(' '), c(' ') };
				case space_8: return s{ c(' '), c(' '), c(' '), c(' '), c(' '), c(' '), c(' '), c(' ') };
				case no_tab:  return s{};
				default:      return s{ c('\t') };
				}
			}
			else if constexpr (::std::same_as<e, serialize_last_semi>)
			{
				using enum serialize_last_semi;
				switch (serialize_e)
				{
				case semi_yes: return s{ c(';') };
				case no_semi:  return s{};
				default:       return s{ c(';') };
				}
			}
			else if constexpr (::std::same_as<e, serialize_separator>)
			{
				using enum serialize_separator;
				switch (serialize_e)
				{
				case sep_semi_with_space:   return s{ c(':'), c(' ') };
				case sep_semi:              return s{ c(':') };
				case sep_space_around_semi: return s{ c(' '), c(':'), c(' ') };
				case sep_space:             return s{ c(' ') };
				default:                    return s{ c(':'), c(' ') };
				}
			}
			else if constexpr (::std::same_as<e, serialize_separator_ts>)
			{
				using enum serialize_separator_ts;
				switch (serialize_e)
				{
				case sts_semi_with_space:   return s{ c(':'), c(' ') };
				case sts_semi:              return s{ c(':') };
				case sts_space_around_semi: return s{ c(' '), c(':'), c(' ') };
				default:                    return s{ c(':'), c(' ') };
				}
			}
			else
			{
				static_assert(false, "param type not match");
			}
		}
	}

	template <typename char_t>
	auto make_serializer(
		serialize_tab          tab      = {},
		serialize_last_semi    last     = {},
		serialize_separator    sep      = {},
		serialize_separator_ts sep_ts   = {},
		serialize_separator    list_sep = {},
		serialize_separator    obj_sep  = {}) -> serializer<char_t>
	{
		return serializer
		{
			.tab              = dev_util::to_string<char_t>(tab),
			.last_sep         = dev_util::to_string<char_t>(last),
			.separator        = dev_util::to_string<char_t>(sep),
			.separator_for_ts = dev_util::to_string<char_t>(sep_ts),
			.list_separator   = dev_util::to_string<char_t>(list_sep),
			.object_separator = dev_util::to_string<char_t>(obj_sep),
		};
	}

	auto make_u8serializer(
		serialize_tab          tab      = {},
		serialize_last_semi    last     = {},
		serialize_separator    sep      = {},
		serialize_separator_ts sep_ts   = {},
		serialize_separator    list_sep = {},
		serialize_separator    obj_sep  = {}) -> serializer<char8_t>
	{
		return make_serializer<char8_t>(tab, last, sep, sep_ts, list_sep, obj_sep);
	}

	auto make_u16serializer(
		serialize_tab          tab      = {},
		serialize_last_semi    last     = {},
		serialize_separator    sep      = {},
		serialize_separator_ts sep_ts   = {},
		serialize_separator    list_sep = {},
		serialize_separator    obj_sep  = {}) -> serializer<char16_t>
	{
		return make_serializer<char16_t>(tab, last, sep, sep_ts, list_sep, obj_sep);
	}

	auto make_u32serializer(
		serialize_tab          tab      = {},
		serialize_last_semi    last     = {},
		serialize_separator    sep      = {},
		serialize_separator_ts sep_ts   = {},
		serialize_separator    list_sep = {},
		serialize_separator    obj_sep  = {}) -> serializer<char32_t>
	{
		return make_serializer<char32_t>(tab, last, sep, sep_ts, list_sep, obj_sep);
	}
}

#endif
