#ifndef PDN_Header_pdn_convert_decision
#define PDN_Header_pdn_convert_decision

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
		static_assert(false, "[pdn] cannot generate convert_decision");
	};

#define PDN_Macro_Temp_make_convert_decision(src_concept, source_ns, des_concept, target_ns) \
	template <src_concept src, des_concept des> \
	struct convert_decision <src, des> \
	{ \
		using decode_result = source_ns::decode_result; \
		using encode_result = target_ns::encode_result; \
		using source_char   = source_ns::code_unit_t; \
		using target_char   = target_ns::code_unit_t; \
		using decoder       = source_ns::decoder; \
		using encoder       = target_ns::encoder; \
		template <bool reach_next_code_point = false> \
		inline static decode_result decode(auto&& begin, auto end) noexcept(noexcept(source_ns::decode<reach_next_code_point>(begin, end))) \
		{ \
			return decoder::decode<reach_next_code_point>(begin, end); \
		} \
		inline static encode_result encode(code_point_t code_point) noexcept(noexcept(target_ns::encode(code_point))) \
		{ \
			return encoder::encode(code_point); \
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
}

#endif
