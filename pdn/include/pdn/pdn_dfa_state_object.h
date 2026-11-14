#ifndef PDN_Header_pdn_dfa_state_object
#define PDN_Header_pdn_dfa_state_object

#include "pdn_dfa_state_code.h"
#include "pdn_token_code.h"
#include "pdn_unicode_base.h"

namespace pdn
{
	// One node of the lexer's state machine: the state itself, the token this state tries to emit, and
	// the transition function that maps a code point to the next state. The state objects themselves are
	// the constexpr constants in pdn_dfa_state_objects.h; the lexer only copies them around.
	//
	// A default constructed object has an unmatched state code and a null transformer: it exists so that
	// members can be value initialized, and calling its transformer is a programming error.
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
		// Identity is the state code: the token code and the transformer are properties of that state, so
		// two objects built for the same state compare equal even if those fields were filled in differently.
		// operator!= synthesis by compiler (C++20)
		friend constexpr bool operator==(dfa_state_object lhs, dfa_state_object rhs) noexcept
		{
			return lhs.state_code == rhs.state_code;
		}
	};
}

#endif
