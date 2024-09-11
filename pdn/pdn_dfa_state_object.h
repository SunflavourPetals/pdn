#ifndef PDN_Header_pdn_dfa_state_object
#define PDN_Header_pdn_dfa_state_object

#include "pdn_dfa_state_code.h"
#include "pdn_token.h"
#include "pdn_unicode_base.h"

namespace pdn
{
	struct dfa_state_object
	{
		using transformer_ptr = dfa_state_object(*)(unicode::code_point_t) noexcept;
		dfa_state_code state_code{};
		pdn_token_code token_code{};
		transformer_ptr transformer{};
		constexpr bool is_final() const noexcept
		{
			return is_final_dfa_state(state_code);
		}
		constexpr bool is_error() const noexcept
		{
			return is_error_dfa_state(state_code);
		}
		// operator!= synthesis by compiler (C++20)
		friend constexpr bool operator==(dfa_state_object lhs, dfa_state_object rhs) noexcept
		{
			return lhs.state_code == rhs.state_code;
		}
	};
}

#endif
