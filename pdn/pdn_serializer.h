#ifndef PDN_Header_pdn_serializer
#define PDN_Header_pdn_serializer

#include <type_traits>
#include <concepts>
#include <utility>
#include <string>
#include <limits>
#include <cstddef>
#include <variant>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <unordered_map>

#ifndef PDN_USER_FORMAT
#include <format>
#endif

#include "pdn_type.h"
#include "pdn_entity.h"

#include "pdn_make_slashes_string.h"
#include "pdn_type_to_type_code.h"
#include "pdn_type_code_to_error_msg_string.h"

#include "pdn_unicode.h"
#include "pdn_utf_code_convert.h"

#include "pdn_proxy.h"

#include "pdn_lexer_utility.h"

#include "pdn_false.h"

namespace pdn::detail
{
	template <typename t>
	concept can_to_string = type::concepts::pdn_integral<t> || type::concepts::pdn_fp<t>;
	template <typename t, typename char_t>
	concept nonlist_nonobj = type::concepts::basic_type<t, char_t> || ::std::same_as<t, type::string<char_t>>;

	// only for standard at values
	template <typename char_t>
	static auto ascii_to_at_constant_string(unicode::u8string_view sv) -> type::string<char_t>
	{
		auto result = type::string<char_t>(sv.size(), char_t{});
		::std::copy(sv.cbegin(), sv.cend(), result.begin());
		return result;
	}

	// inf and nan not support
	template <typename char_t>
	class f64_at_constant_string_table : public ::std::unordered_map<type::f64, type::string<char_t>>
	{
	public:
		f64_at_constant_string_table()
		{
			using namespace unicode_literals;
			using namespace ::std::numbers;

			auto& self = *this;

			self[e]          = ascii_to_at_constant_string<char_t>(u8"@e"_sv);
			self[log2e]      = ascii_to_at_constant_string<char_t>(u8"@log2e"_sv);
			self[log10e]     = ascii_to_at_constant_string<char_t>(u8"@log10e"_sv);
			self[pi]         = ascii_to_at_constant_string<char_t>(u8"@pi"_sv);
			self[inv_pi]     = ascii_to_at_constant_string<char_t>(u8"@inv_pi"_sv);
			self[inv_sqrtpi] = ascii_to_at_constant_string<char_t>(u8"@inv_sqrtpi"_sv);
			self[ln2]        = ascii_to_at_constant_string<char_t>(u8"@ln2"_sv);
			self[ln10]       = ascii_to_at_constant_string<char_t>(u8"@ln10"_sv);
			self[sqrt2]      = ascii_to_at_constant_string<char_t>(u8"@sqrt2"_sv);
			self[sqrt3]      = ascii_to_at_constant_string<char_t>(u8"@sqrt3"_sv);
			self[inv_sqrt3]  = ascii_to_at_constant_string<char_t>(u8"@inv_sqrt3"_sv);
			self[egamma]     = ascii_to_at_constant_string<char_t>(u8"@egamma"_sv);
			self[phi]        = ascii_to_at_constant_string<char_t>(u8"@phi"_sv);
		}
		static auto instance() -> const f64_at_constant_string_table<char_t>&
		{
			static f64_at_constant_string_table<char_t> obj{};
			return obj;
		}
	};

	// inf and nan not support
	template <typename char_t>
	class f32_at_constant_string_table : public ::std::unordered_map<type::f32, type::string<char_t>>
	{
	public:
		f32_at_constant_string_table()
		{
			using namespace unicode_literals;
			using namespace ::std::numbers;

			auto& self = *this;

			self[static_cast<type::f32>(e)]          = ascii_to_at_constant_string<char_t>(u8"@e"_sv);
			self[static_cast<type::f32>(log2e)]      = ascii_to_at_constant_string<char_t>(u8"@log2e"_sv);
			self[static_cast<type::f32>(log10e)]     = ascii_to_at_constant_string<char_t>(u8"@log10e"_sv);
			self[static_cast<type::f32>(pi)]         = ascii_to_at_constant_string<char_t>(u8"@pi"_sv);
			self[static_cast<type::f32>(inv_pi)]     = ascii_to_at_constant_string<char_t>(u8"@inv_pi"_sv);
			self[static_cast<type::f32>(inv_sqrtpi)] = ascii_to_at_constant_string<char_t>(u8"@inv_sqrtpi"_sv);
			self[static_cast<type::f32>(ln2)]        = ascii_to_at_constant_string<char_t>(u8"@ln2"_sv);
			self[static_cast<type::f32>(ln10)]       = ascii_to_at_constant_string<char_t>(u8"@ln10"_sv);
			self[static_cast<type::f32>(sqrt2)]      = ascii_to_at_constant_string<char_t>(u8"@sqrt2"_sv);
			self[static_cast<type::f32>(sqrt3)]      = ascii_to_at_constant_string<char_t>(u8"@sqrt3"_sv);
			self[static_cast<type::f32>(inv_sqrt3)]  = ascii_to_at_constant_string<char_t>(u8"@inv_sqrt3"_sv);
			self[static_cast<type::f32>(egamma)]     = ascii_to_at_constant_string<char_t>(u8"@egamma"_sv);
			self[static_cast<type::f32>(phi)]        = ascii_to_at_constant_string<char_t>(u8"@phi"_sv);
		}
		static auto instance() -> const f32_at_constant_string_table<char_t>&
		{
			static f32_at_constant_string_table<char_t> obj{};
			return obj;
		}
	};

	template <typename char_t, type::concepts::pdn_fp fp>
	auto fp_at_constant_string_table_function(fp val) -> const type::string<char_t>&
	{
		if constexpr (::std::same_as<fp, type::f32>)
		{
			if (auto result = f32_at_constant_string_table<char_t>::instance().find(val);
				result != f32_at_constant_string_table<char_t>::instance().end())
			{
				return result->second;
			}
		}
		else
		{
			if (auto result = f64_at_constant_string_table<char_t>::instance().find(val);
				result != f64_at_constant_string_table<char_t>::instance().end())
			{
				return result->second;
			}
		}
		return as_accessor<char_t>::null_string_val();
	}

	template <typename char_t>
	void to_pdn_format(type::string<char_t>& out, type::concepts::pdn_integral auto val)
	{
		if constexpr (::std::same_as<decltype(val), type::i64>)
		{
			if (val == ::std::numeric_limits<type::i64>::min())
			{
				for (auto c : u8"@i64_min"_sv)
				{
					out += char_t(c);
				}
				return;
			}
		}
#ifdef PDN_USER_FORMAT
		auto formatted = PDN_USER_FORMAT("{}", val);
#else
		auto formatted = ::std::format("{}", val);
#endif
		for (const auto c : formatted)
		{
			out += char_t(c);
		}
	}

	template <typename char_t>
	void to_pdn_format(type::string<char_t>& out, type::concepts::pdn_fp auto val)
	{
		using namespace unicode_literals;
		if (::std::isinf(val))
		{
			auto formatted = ::std::signbit(val) ? u8"-@inf"_sv : u8"@inf"_sv;
			for (auto c : formatted)
			{
				out += char_t(c);
			}
			return;
		}
		else if (::std::isnan(val))
		{
			for (auto c : u8"@NaN"_sv)
			{
				out += char_t(c);
			}
			return;
		}
		const auto& at_constant_string = fp_at_constant_string_table_function<char_t>(val);
		if (!at_constant_string.empty())
		{
			out += at_constant_string;
			return;
		}

		bool has_e{};
		bool has_dot{};
#ifdef PDN_USER_FORMAT
		auto formatted = PDN_USER_FORMAT("{}", val);
#else
		auto formatted = ::std::format("{}", val);
#endif
		for (const auto c : formatted)
		{
			out += char_t(c);
			if (c == 'e') has_e = true;
			if (c == '.') has_dot = true;
		}
		if (!has_e && !has_dot)
		{
			out.push_back(char_t('.')); // make sure that it is a floating-point format
			out.push_back(char_t('0'));
		}
	}

	template <typename char_t>
	void to_pdn_format(type::string<char_t>& out, type::boolean val)
	{
		using namespace unicode_literals;
		auto formatted = val ? u8"@true"_sv : u8"@false"_sv;
		for (auto c : formatted)
		{
			out += char_t(c);
		}
	}

	template <typename char_t, typename src_char_t>
	void to_pdn_format(type::string<char_t>& out, type::character<src_char_t> val)
	{
		auto append_quoted = [](type::string<char_t>& sink_out, ::std::basic_string_view<char_t> sv)
		{
			sink_out.push_back(char_t('\''));
			sink_out += sv;
			sink_out.push_back(char_t('\''));
		};
		auto src_sv = val.to_string_view();
		auto decode_result = unicode::decode<true>(src_sv.cbegin(), src_sv.cend());
		if (decode_result)
		{
			append_quoted(out, make_slashes_string<type::string<char_t>>(decode_result.value()));
		}
		else
		{
			append_quoted(out, unicode::get_replace<char_t>());
		}
	}

	template <typename char_t, typename src_char_t>
	void to_pdn_format(type::string<char_t>& out, const type::string<src_char_t>& val)
	{
		auto append_quoted = [](type::string<char_t>& sink_out, ::std::basic_string_view<char_t> sv)
		{
			sink_out.push_back(char_t('\"'));
			sink_out += sv;
			sink_out.push_back(char_t('\"'));
		};
		auto cp_s = unicode::code_convert<unicode::ucpstring>(val);
		append_quoted(out, make_slashes_string<type::string<char_t>>(cp_s));
	}
}

namespace pdn
{
	template <unicode::concepts::code_unit my_char_t = char8_t>
	class serializer
	{
	public:
		using char_t = my_char_t; // target char type
		using string_t = type::string<char_t>;
		using string_view_t = ::std::basic_string_view<char_t>;
		// dom to .spdn format
		template <typename src_char_t>
		auto serialize(
			const type::object<src_char_t>& dom, // only object is allowed(spdn text is an object)
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
				serialize_object_member(result, e.first, e.second, 0, use_line_feed, use_last_sep_for_lo);
				if (use_line_feed) result += char_t('\n');
				else               result += char_t(' ');
			}
			return result;
		}
	private:
		// to "[`]slashed_iden[`] [: [type]] pdn_form[;]"
		// out argument cannot be an alias of val
		template <typename src_char_t, detail::nonlist_nonobj<src_char_t> value_t>
		void serialize(string_t& out, const type::string<src_char_t>& iden, const value_t& val) const
		{
			using val_t = ::std::decay_t<decltype(val)>;
			serialize_iden(out, iden);
			constexpr auto no_type_spec
				 = ::std::same_as<val_t, type::f64>
				|| ::std::same_as<val_t, type::boolean>
				|| ::std::same_as<val_t, type::character<src_char_t>>
				|| ::std::same_as<val_t, type::string<src_char_t>>;
			if constexpr (::std::same_as<val_t, type::auto_int>)
			{
				if (val == ::std::numeric_limits<type::auto_int>::min()) [[unlikely]]
				{
					out += separator_for_ts;
					constexpr auto type_c = type_to_type_code_v<val_t, src_char_t>;
					out += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
					out += char_t(' ');
					serialize_value<src_char_t>(out, val);
				}
				else
				{
					out += separator;
					serialize_value<src_char_t>(out, val);
				}
			}
			else if constexpr (::std::same_as<val_t, type::i64>)
			{
				if (val == ::std::numeric_limits<type::i64>::min()) [[unlikely]]
				{
					out += separator;
					serialize_value<src_char_t>(out, val);
				}
				else
				{
					out += separator_for_ts;
					constexpr auto type_c = type_to_type_code_v<val_t, src_char_t>;
					out += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
					out += char_t(' ');
					serialize_value<src_char_t>(out, val);
				}
			}
			else if constexpr (no_type_spec)
			{
				out += separator;
				serialize_value<src_char_t>(out, val);
			}
			else
			{
				out += separator_for_ts;
				constexpr auto type_c = type_to_type_code_v<val_t, src_char_t>;
				out += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
				out += char_t(' ');
				serialize_value<src_char_t>(out, val);
			}
			out += last_sep;
		}
		// to "[`]slashed_iden[`] [: [type]] { pdn_form_of_members }[;]"
		template <typename src_char_t>
		void serialize(
			string_t& out,
			const type::string<src_char_t>& iden,
			const type::object<src_char_t>& val,
			const ::std::size_t layer = 0,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const
		{
			serialize_iden(out, iden);
			out += object_separator;
			serialize_object(out, val, layer, use_line_feed, use_last_sep_for_lo);
			if (use_last_sep_for_lo) out += last_sep;
		}
		// to "[`]slashed_iden[`] [: [type]] "[" pdn_form_of_elements "]"[;]"
		template <typename src_char_t>
		void serialize(
			string_t& out,
			const type::string<src_char_t>& iden,
			const type::list<src_char_t>& val,
			const ::std::size_t layer = 0,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const
		{
			serialize_iden(out, iden);
			out += list_separator;
			serialize_list(out, val, layer, use_line_feed, use_last_sep_for_lo);
			if (use_last_sep_for_lo) out += last_sep;
		}
		// identifier to "[`]slashed[`]"
		template <typename src_char_t>
		void serialize_iden(string_t& out, const type::string<src_char_t>& iden) const
		{
			auto cp_s = unicode::code_convert<unicode::ucpstring>(iden);
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
					for (auto c : unicode::ucpstring_view{ cp_s.cbegin() + 1, cp_s.cend() })
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
				out.push_back(char_t('`'));
				out += slashes;
				out.push_back(char_t('`'));
			}
			else
			{
				out += ::std::move(slashes);
			}
		}
		// integral/floating/boolean/character/string to pdn_form
		template <typename src_char_t>
		void serialize_value(string_t& out, detail::nonlist_nonobj<src_char_t> auto const& val) const
		{
			detail::to_pdn_format<char_t>(out, val);
		}
		// object to "{ ... }"
		template <typename src_char_t>
		void serialize_object(
			string_t& out,
			const type::object<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const
		{
			out += char_t('{');
			if (val.empty())
			{
				out += char_t('}');
				return;
			}
			const auto inner_layer = layer + 1;
			for (const auto& e : val)
			{
				if (use_line_feed)
				{
					out += char_t('\n');
					append_tabs(out, inner_layer);
				}
				serialize_object_member(out, e.first, e.second, inner_layer, use_line_feed, use_last_sep_for_lo);
				if (!use_line_feed) out += char_t(' ');
			}
			if (use_line_feed)
			{
				out += char_t('\n');
				append_tabs(out, layer);
			}
			out += char_t('}');
		}
		// object member to "name [:[type]] value [;]..."
		template <typename src_char_t>
		void serialize_object_member(
			string_t& out,
			const type::string<src_char_t>& iden,
			const entity<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const
		{
			::std::visit([&]<typename arg_pr_t>(const arg_pr_t& arg)
			{
				using arg_t = remove_proxy_t<arg_pr_t>;
				if constexpr (detail::nonlist_nonobj<arg_t, src_char_t>)
				{
					constexpr bool has_proxy = !::std::same_as<arg_t, arg_pr_t>;
					if constexpr (has_proxy) serialize(out, iden, *arg);
					else                     serialize(out, iden,  arg);
				}
				else
				{
					serialize(out, iden, *arg, layer, use_line_feed, use_last_sep_for_lo);
				}
			}, val);
		}
		// list to "[ ..., ]"
		template <typename src_char_t>
		void serialize_list(
			string_t& out,
			const type::list<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const
		{
			out += char_t('[');
			if (val.empty())
			{
				out += char_t(']');
				return;
			}
			const auto inner_layer = layer + 1;
			for (const auto& e : val)
			{
				if (use_line_feed)
				{
					out += char_t('\n');
					append_tabs(out, inner_layer);
				}
				serialize_list_element(out, e, inner_layer, use_line_feed, use_last_sep_for_lo);
				out += char_t(',');
				if (!use_line_feed) out += char_t(' ');
			}
			if (use_line_feed)
			{
				out += char_t('\n');
				append_tabs(out, layer);
			}
			out += char_t(']');
		}
		// list element to "[type:] pdn_form,"
		template <typename src_char_t>
		void serialize_list_element(
			string_t& out,
			const entity<src_char_t>& val,
			const ::std::size_t layer,
			const bool use_line_feed = true,
			const bool use_last_sep_for_lo = true) const
		{
			::std::visit([&]<typename arg_pr_t>(const arg_pr_t& arg)
			{
				using arg_t = remove_proxy_t<arg_pr_t>;
				if constexpr (::std::same_as<arg_t, type::object<src_char_t>>)
				{
					serialize_object(out, *arg, layer, use_line_feed, use_last_sep_for_lo);
				}
				else if constexpr (::std::same_as<arg_t, type::list<src_char_t>>)
				{
					serialize_list(out, *arg, layer, use_line_feed, use_last_sep_for_lo);
				}
				else if constexpr (::std::same_as<arg_t, type::string<src_char_t>>)
				{
					serialize_value<src_char_t>(out, *arg);
				}
				else
				{
					constexpr auto no_type_spec
						 = ::std::same_as<arg_t, type::f64>
						|| ::std::same_as<arg_t, type::boolean>
						|| ::std::same_as<arg_t, type::character<src_char_t>>
						|| ::std::same_as<arg_t, type::string<src_char_t>>;
					if constexpr (::std::same_as<arg_t, type::auto_int>)
					{
						if (arg == ::std::numeric_limits<type::auto_int>::min()) [[unlikely]]
						{
							constexpr auto type_c = type_to_type_code_v<arg_t, src_char_t>;
							out += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
							out += separator_for_ts;
							serialize_value<src_char_t>(out, arg);
						}
						else
						{
							serialize_value<src_char_t>(out, arg);
						}
					}
					else if constexpr (::std::same_as<arg_t, type::i64>)
					{
						if (arg == ::std::numeric_limits<type::i64>::min()) [[unlikely]]
						{
							serialize_value<src_char_t>(out, arg);
						}
						else
						{
							constexpr auto type_c = type_to_type_code_v<arg_t, src_char_t>;
							out += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
							out += separator_for_ts;
							serialize_value<src_char_t>(out, arg);
						}
					}
					else if constexpr (no_type_spec)
					{
						serialize_value<src_char_t>(out, arg);
					}
					else
					{
						constexpr auto type_c = type_to_type_code_v<arg_t, src_char_t>;
						out += unicode::code_convert<string_t>(type_code_to_error_msg_string(type_c));
						out += separator_for_ts;
						serialize_value<src_char_t>(out, arg);
					}
				}
			}, val);
		}
		void append_tabs(string_t& out, ::std::size_t layer) const
		{
			for (::std::size_t i{}; i != layer; ++i)
			{
				out += tab;
			}
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

	namespace detail
	{
		template <typename c, typename e>
		auto to_string(e serialize_e) -> type::string<c>
		{
			using s = type::string<c>;
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
				static_assert(detail::false_v<e>, "[pdn] param type not match");
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
		return serializer<char_t>
		{
			.tab              = detail::to_string<char_t>(tab),
			.last_sep         = detail::to_string<char_t>(last),
			.separator        = detail::to_string<char_t>(sep),
			.separator_for_ts = detail::to_string<char_t>(sep_ts),
			.list_separator   = detail::to_string<char_t>(list_sep),
			.object_separator = detail::to_string<char_t>(obj_sep),
		};
	}

	inline auto make_u8serializer(
		serialize_tab          tab      = {},
		serialize_last_semi    last     = {},
		serialize_separator    sep      = {},
		serialize_separator_ts sep_ts   = {},
		serialize_separator    list_sep = {},
		serialize_separator    obj_sep  = {}) -> serializer<char8_t>
	{
		return make_serializer<char8_t>(tab, last, sep, sep_ts, list_sep, obj_sep);
	}

	inline auto make_u16serializer(
		serialize_tab          tab      = {},
		serialize_last_semi    last     = {},
		serialize_separator    sep      = {},
		serialize_separator_ts sep_ts   = {},
		serialize_separator    list_sep = {},
		serialize_separator    obj_sep  = {}) -> serializer<char16_t>
	{
		return make_serializer<char16_t>(tab, last, sep, sep_ts, list_sep, obj_sep);
	}

	inline auto make_u32serializer(
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
