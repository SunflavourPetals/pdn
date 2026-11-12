#ifndef PDN_Header_pdn_decayed_same_as_concept
#define PDN_Header_pdn_decayed_same_as_concept

#include <type_traits>
#include <concepts>

namespace pdn::detail
{
	template <typename from_t, typename to_t>
	concept decayed_same_as = ::std::same_as<::std::remove_cvref_t<from_t>, to_t>;
}

#endif
