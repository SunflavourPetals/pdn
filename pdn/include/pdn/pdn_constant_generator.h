#ifndef PDN_Header_pdn_constant_generator
#define PDN_Header_pdn_constant_generator

#include <optional>

#include "pdn_unicode_base.h"
#include "pdn_entity.h"
#include "pdn_constant_generator_concept.h"
#include "pdn_constant_generator_std.h"

// The built-in at-constant generator. default_function_package forwards the text of an @identifier here,
// and this adapter turns the table in pdn_constant_generator_std.h into the callable form required by the
// constant_generator concept: a static member returning std::optional<entity<char_t>>. Keys stay UTF-8 for
// every char_t; std::nullopt means "no such built-in constant" and the parser reports at_value_not_found.
namespace pdn
{
	template <typename char_t>
	class default_constant_generator
	{
	public:
		static auto generate_constant(const unicode::u8string& iden) -> ::std::optional<entity<char_t>>
		{
			return constant_generator_std_function<char_t>(iden);
		}
	};
	// entity only supports the three UTF character widths, so check the adapter satisfies each of them.
	static_assert(concepts::constant_generator<default_constant_generator<unicode::u8char_t>, unicode::u8char_t>);
	static_assert(concepts::constant_generator<default_constant_generator<unicode::u16char_t>, unicode::u16char_t>);
	static_assert(concepts::constant_generator<default_constant_generator<unicode::u32char_t>, unicode::u32char_t>);
}

#endif
