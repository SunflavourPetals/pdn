#ifndef PDN_Header_pdn_eof_detector
#define PDN_Header_pdn_eof_detector

#include <concepts>

namespace pdn
{
	template <typename type>
	concept eof_detector = requires(const type o)
	{
		{ o.eof() } -> ::std::convertible_to<bool>;
	};
}

#endif
