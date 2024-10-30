#ifndef PDN_Header_pdn_unicode_decoder
#define PDN_Header_pdn_unicode_decoder

#include <type_traits>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_utf_8_base.h"
#include "pdn_utf_8_decoder.h"
#include "pdn_utf_16_base.h"
#include "pdn_utf_16_decoder.h"
#include "pdn_utf_32_base.h"
#include "pdn_utf_32_decoder.h"

namespace pdn::unicode::dev_util
{
	template <typename type>
	concept utf_8_iterator = requires (type it)
	{
		requires concepts::utf_8_code_unit<::std::remove_reference_t<decltype(*it)>>;
		++it;
	};
	template <typename type>
	concept utf_16_iterator = requires (type it)
	{
		requires concepts::utf_16_code_unit<::std::remove_reference_t<decltype(*it)>>;
		++it;
	};
	template <typename type>
	concept utf_32_iterator = requires (type it)
	{
		requires concepts::utf_32_code_unit<::std::remove_reference_t<decltype(*it)>>;
		++it;
	};
}

namespace pdn::unicode
{
	template <bool reach_next_code_point, dev_util::utf_8_iterator it_begin_t, typename it_end_t>
	auto decode(it_begin_t&& begin, it_end_t end)
	{
		return utf_8::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
	}
	template <bool reach_next_code_point, dev_util::utf_16_iterator it_begin_t, typename it_end_t>
	auto decode(it_begin_t&& begin, it_end_t end)
	{
		return utf_16::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
	}
	template <bool reach_next_code_point, dev_util::utf_32_iterator it_begin_t, typename it_end_t>
	auto decode(it_begin_t&& begin, it_end_t end)
	{
		return utf_32::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
	}

	class decoder // not final for ebo
	{
	public:
		template <bool reach_next_code_point, concepts::code_unit it_begin_t, typename it_end_t>
		static auto decode(it_begin_t&& begin, it_end_t end)
		{
			return ::pdn::unicode::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
		}
	};
}

#endif
