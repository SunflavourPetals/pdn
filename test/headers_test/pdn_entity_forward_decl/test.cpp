// Tests for pdn/include/pdn/pdn_entity_forward_decl.h.
//
// The header exists so that pdn_type.h can name entity<char_t> without pulling in the definition
// (pdn_entity.h includes pdn_type.h, so the reverse direction would be a cycle). The first part
// below uses the three names as incomplete types with nothing but the forward declaration
// included; the second part, after the full definition is visible, checks that the declarations
// name exactly the classes the public API returns.

#include <type_traits>

#include <pdn/pdn_entity_forward_decl.h>

namespace
{
	// Incomplete types are enough for pointers, references and function declarations.
	pdn::entity<char8_t>* entity_ptr = nullptr;
	pdn::refer<char16_t>* refer_ptr = nullptr;
	pdn::const_refer<char32_t>* cref_ptr = nullptr;

	// Member declarations accept the incomplete types without any definition being visible.
	struct api_shape
	{
		auto take_entity(const pdn::entity<char32_t>&) -> void;
		auto take_refer(pdn::refer<char8_t>&) -> void;
		auto take_cref(const pdn::const_refer<char8_t>&) -> void;
	};

	// Each declaration introduces a class template with a single type parameter, and the names are
	// usable before any definition is available.
	static_assert(::std::is_class_v<pdn::entity<char8_t>>);
	static_assert(::std::is_class_v<pdn::refer<char8_t>>);
	static_assert(::std::is_class_v<pdn::const_refer<char8_t>>);
}

#include <pdn/pdn_entity.h>

namespace
{
	// ref() on an lvalue entity returns refer, on a const lvalue returns const_refer; the names
	// declared above must be the same classes, otherwise the forward declaration would be useless.
	static_assert(::std::is_same_v<pdn::refer<char8_t>, decltype(::std::declval<pdn::entity<char8_t>&>().ref())>);
	static_assert(::std::is_same_v<pdn::const_refer<char8_t>, decltype(::std::declval<const pdn::entity<char8_t>&>().ref())>);
	static_assert(::std::is_same_v<pdn::const_refer<char8_t>, decltype(::std::declval<const pdn::entity<char8_t>&>().cref())>);

	// refer is the mutable view, const_refer the read-only one.
	static_assert(::std::is_base_of_v<pdn::const_refer<char8_t>, pdn::refer<char8_t>>);
}

int main()
{
	return (entity_ptr != nullptr) || (refer_ptr != nullptr) || (cref_ptr != nullptr);
}
