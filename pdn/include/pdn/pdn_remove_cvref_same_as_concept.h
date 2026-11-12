#ifndef PDN_Header_pdn_remove_cvref_same_as_concept
#define PDN_Header_pdn_remove_cvref_same_as_concept

#include <type_traits>

namespace pdn::detail
{
	template <typename type, typename target_type>
	concept remove_cvref_same_as = ::std::is_same_v<::std::remove_cvref_t<type>, ::std::remove_cvref_t<target_type>>;
}

#endif
