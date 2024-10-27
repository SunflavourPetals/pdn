#ifndef PDN_Header_pdn_eof_checker_concept
#define PDN_Header_pdn_eof_checker_concept

#include <concepts>

namespace pdn::dev_util
{
	template <typename type>
	concept eof_checker = requires(type o) { { o.eof() } -> ::std::convertible_to<bool>; };
}

#endif
