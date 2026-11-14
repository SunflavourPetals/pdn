#ifndef PDN_Header_pdn_type_generator_concept
#define PDN_Header_pdn_type_generator_concept

#include <concepts>
#include <utility>

#include "pdn_type.h"
#include "pdn_type_code.h"

namespace pdn::concepts
{
	// The parser-function-package hook that turns a written type name into a type_code.
	// type_code::unknown is how "this is not a PDN type name" is reported: the parser then
	// posts syntax_error_code::unknown_type with the identifier (pdn_parser.h:210-214), so
	// an implementation must answer for every name instead of rejecting the unknown ones.
	// Both an lvalue and an rvalue identifier must be accepted; implementations normally
	// take const&. The generator is passed by value here, so unlike the other package
	// concepts it is not required to be callable on a const object and may carry state.
	template <typename type_gen_t, typename char_t>
	concept type_generator = requires (type_gen_t type_gen, type::string<char_t> iden)
	{
		{ type_gen.generate_type(iden) } -> ::std::convertible_to<type_code>;
		{ type_gen.generate_type(::std::move(iden)) } -> ::std::convertible_to<type_code>;
	};
}

#endif
