#ifndef PDN_Header_pdn_false
#define PDN_Header_pdn_false

#include <type_traits>

namespace pdn::detail
{
	template <typename>
	constexpr bool false_v = false;
}

#endif
