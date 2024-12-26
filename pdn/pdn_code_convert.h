#ifndef PDN_Header_pdn_code_convert
#define PDN_Header_pdn_code_convert

#include <string>
#include <utility>
#include <concepts>
#include <functional>
#include <type_traits>

#include "pdn_unicode.h"

namespace pdn::unicode
{
	namespace concepts
	{
#define PDN_Macro_Temp_make_string_src_des_concepts(src_name, des_name, charx_t) \
		template <typename string> \
		concept src_name = requires(string source) \
		{ \
			typename string::value_type; \
			typename string::iterator; \
			requires ::std::same_as<typename string::value_type, charx_t>; \
			requires ::std::random_access_iterator<typename string::iterator>; \
		}; \
		template <typename string> \
		concept des_name = requires(string destination, ::std::basic_string<charx_t> s) \
		{ \
			typename string::value_type; \
			requires ::std::same_as<typename string::value_type, charx_t>; \
			destination.append(s.cbegin(), s.cend()); \
		};

		PDN_Macro_Temp_make_string_src_des_concepts(utf_8_convert_src,  utf_8_convert_des,  char8_t)
		PDN_Macro_Temp_make_string_src_des_concepts(utf_16_convert_src, utf_16_convert_des, char16_t)
		PDN_Macro_Temp_make_string_src_des_concepts(utf_32_convert_src, utf_32_convert_des, char32_t)

#undef PDN_Macro_Temp_make_string_src_des_concepts
	}
	
	template <typename source_string_view, typename target_string>
	struct convert_decision
	{
		static_assert(false, "[pdn] cannot generate convert_decision for template arguments");
	};

#define PDN_Macro_Temp_make_convert_decision(src_concept, source_ns, des_concept, target_ns) \
	template <src_concept src, des_concept des> \
	struct convert_decision <src, des> \
	{ \
		using decode_result = source_ns::decode_result; \
		using encode_result = target_ns::encode_result; \
		using source_char = source_ns::code_unit_t; \
		using target_char = target_ns::code_unit_t; \
		template <bool reach_next_code_point = false> \
		inline static decode_result decode(auto&& begin, auto end) noexcept(noexcept(source_ns::decode<reach_next_code_point>(begin, end))) \
		{ \
			return source_ns::decode<reach_next_code_point>(begin, end); \
		} \
		inline static encode_result encode(code_point_t code_point) noexcept(noexcept(target_ns::encode(code_point))) \
		{ \
			return target_ns::encode(code_point); \
		} \
	};

	PDN_Macro_Temp_make_convert_decision(concepts::utf_8_convert_src,  utf_8,  concepts::utf_8_convert_des,  utf_8)
	PDN_Macro_Temp_make_convert_decision(concepts::utf_8_convert_src,  utf_8,  concepts::utf_16_convert_des, utf_16)
	PDN_Macro_Temp_make_convert_decision(concepts::utf_8_convert_src,  utf_8,  concepts::utf_32_convert_des, utf_32)

	PDN_Macro_Temp_make_convert_decision(concepts::utf_16_convert_src, utf_16, concepts::utf_8_convert_des,  utf_8)
	PDN_Macro_Temp_make_convert_decision(concepts::utf_16_convert_src, utf_16, concepts::utf_16_convert_des, utf_16)
	PDN_Macro_Temp_make_convert_decision(concepts::utf_16_convert_src, utf_16, concepts::utf_32_convert_des, utf_32)

	PDN_Macro_Temp_make_convert_decision(concepts::utf_32_convert_src, utf_32, concepts::utf_8_convert_des,  utf_8)
	PDN_Macro_Temp_make_convert_decision(concepts::utf_32_convert_src, utf_32, concepts::utf_16_convert_des, utf_16)
	PDN_Macro_Temp_make_convert_decision(concepts::utf_32_convert_src, utf_32, concepts::utf_32_convert_des, utf_32)

#undef PDN_Macro_Temp_make_convert_decision
	
	namespace utility
	{
		template <typename convert_src, typename convert_des>
		inline auto default_decode_error_handler(typename convert_decision<convert_src, convert_des>::decode_result, ::std::size_t)
			-> ::std::pair<bool, bool>
		{
			// pair.first  -> true to finish converting, false to continue
			// pair.second -> true to replace with Replacement Character when failed, false to do nothing
			return ::std::make_pair(false, false);
		}
		template <typename convert_src, typename convert_des>
		inline auto default_encode_error_handler(typename convert_decision<convert_src, convert_des>::encode_result, ::std::size_t)
			-> ::std::pair<bool, bool>
		{
			// pair.first  -> true to finish converting, false to continue
			// pair.second -> true to replace with Replacement Character when failed, false to do nothing
			return ::std::make_pair(false, false);
		}
		template <typename char_t>
		auto get_rep_char() -> ::std::basic_string_view<::std::decay_t<char_t>>
		{
			using namespace ::std::literals::string_view_literals;
			using c = ::std::decay_t<char_t>;
			if constexpr (unicode::concepts::utf_8_code_unit<c>)
			{
				return u8"\uFFFD"sv;
			}
			else if constexpr (unicode::concepts::utf_16_code_unit<c>)
			{
				return u"\uFFFD"sv;
			}
			else if constexpr (unicode::concepts::utf_32_code_unit<c>)
			{
				return U"\uFFFD"sv;
			}
			else
			{
				static_assert(false, "bad call");
			}
		}
	}

	// param decode_error_handler: see utility::default_decode_error_handler
	// param encode_error_handler: see utility::default_encode_error_handler
	template <typename target_string,
	          typename source_string_view,
	          typename decode_error_handler_t,
	          typename encode_error_handler_t>
	inline target_string code_convert(const source_string_view& source,
	                                  decode_error_handler_t    decode_error_handler,
	                                  encode_error_handler_t    encode_error_handler)
	{
		using decision = convert_decision<source_string_view, target_string>;
		target_string convert_result{};
		auto begin = ::std::cbegin(source);
		while (begin != ::std::cend(source))
		{
			auto curr = begin;
			auto decode_result = decision::template decode<true>(begin, ::std::cend(source));
			if (!decode_result)
			{
				auto r = decode_error_handler(decode_result, curr - ::std::cbegin(source));
				
				if (r.first)
				{
					break;
				}
				if (r.second)
				{
					convert_result.append(utility::get_rep_char<typename decision::target_char>());
				}
				continue;
			}
			auto encode_result = decision::encode(decode_result.value());
			if (!encode_result)
			{
				auto r = encode_error_handler(encode_result, curr - ::std::cbegin(source));

				if (r.first)
				{
					break;
				}
				if (r.second)
				{
					convert_result.append(utility::get_rep_char<typename decision::target_char>());
				}
				continue;
			}
			convert_result.append(encode_result.cbegin(), encode_result.cend());
		}
		return convert_result;
	}
	template <typename target_string, typename source_string_view>
	inline target_string code_convert(const source_string_view& source)
	{
		return code_convert<target_string>(
			source,
			&utility::default_decode_error_handler<source_string_view, target_string>,
			&utility::default_encode_error_handler<source_string_view, target_string>);
	}
	template <typename target_string, typename source_string_view>
	inline target_string replace_code_convert(const source_string_view& source)
	{
		return code_convert<target_string>(
			source,
			[](auto, auto) { return ::std::make_pair(false, true); },
			[](auto, auto) { return ::std::make_pair(false, true); });
	}
}

#endif
