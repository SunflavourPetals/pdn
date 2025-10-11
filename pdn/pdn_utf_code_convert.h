#ifndef PDN_Header_pdn_utf_code_convert
#define PDN_Header_pdn_utf_code_convert

#include <string>
#include <utility>
#include <type_traits>

#include "pdn_unicode.h"
#include "pdn_convert_decision.h"

namespace pdn::unicode::detail
{
	template <typename cvt_src, typename cvt_des>
	using dec_res = typename convert_decision<cvt_src, cvt_des>::decode_result_type;

	template <typename cvt_src, typename cvt_des>
	using enc_res = typename convert_decision<cvt_src, cvt_des>::encode_result_type;

	template <typename cvt_src, typename cvt_des>
	using dec_char = typename convert_decision<cvt_src, cvt_des>::target_char;

	template <typename cvt_src, typename cvt_des>
	inline bool default_decode_error_handler(cvt_des& des, dec_res<cvt_src, cvt_des>, ::std::size_t)
	{
		// replace with Replacement Character
		auto rep = get_replace<dec_char<cvt_src, cvt_des>>();
		des.append(rep.cbegin(), rep.cend());
		// return true to finish converting, return false to continue
		return false;
	}

	template <typename cvt_src, typename cvt_des>
	inline bool default_encode_error_handler(cvt_des& des, enc_res<cvt_src, cvt_des>, ::std::size_t)
	{
		// replace with Replacement Character
		auto rep = get_replace<dec_char<cvt_src, cvt_des>>();
		des.append(rep.cbegin(), rep.cend());
		// return true to finish converting, return false to continue
		return false;
	}
}

namespace pdn::unicode
{
	// param decode_error_handler: see detail::default_decode_error_handler
	// param encode_error_handler: see detail::default_encode_error_handler
	template <typename target_string,
	          typename source_string_view,
	          typename decode_error_handler_t,
	          typename encode_error_handler_t>
	inline auto code_convert(const source_string_view& source,
	                         decode_error_handler_t    decode_error_handler,
	                         encode_error_handler_t    encode_error_handler) -> target_string
	{
		using decision = convert_decision<source_string_view, target_string>;

		target_string convert_result{};

		for (auto begin = ::std::cbegin(source); begin != ::std::cend(source); )
		{
			auto curr = begin;
			auto decode_result = decision::template decode<true>(begin, ::std::cend(source));
			if (!decode_result)
			{
				if (decode_error_handler(convert_result, decode_result, curr - ::std::cbegin(source)))
				{
					break;
				}
				continue;
			}
			auto encode_result = decision::encode(decode_result.value());
			if (!encode_result)
			{
				if (encode_error_handler(convert_result, encode_result, curr - ::std::cbegin(source)))
				{
					break;
				}
				continue;
			}
			convert_result.append(encode_result.cbegin(), encode_result.cend());
		}
		return convert_result;
	}

	template <typename target_string, typename source_string_view>
	inline auto code_convert(const source_string_view& source) -> target_string
	{
		return code_convert<target_string>(
			source,
			&detail::default_decode_error_handler<source_string_view, target_string>,
			&detail::default_encode_error_handler<source_string_view, target_string>);
	}
}

#endif
