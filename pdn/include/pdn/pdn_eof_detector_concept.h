#ifndef PDN_Header_pdn_eof_detector_concept
#define PDN_Header_pdn_eof_detector_concept

#include <concepts>
#include <type_traits>

namespace pdn::concepts
{
	// Models an input source that reports its own exhaustion instead of exposing a begin/end pair.
	// eof() must be callable on a const object, return bool, and be true exactly when nothing is
	// left to read; the library uses it to select the eof() driven overloads and to end its read
	// loops. The reference is stripped first, so that eof_detector<T&>, eof_detector<const T&>
	// and eof_detector<T&&> all mean the same as eof_detector<T>. The check is purely structural,
	// so any type with such an eof() models it.
	template <typename type>
	concept eof_detector = requires(::std::add_const_t<::std::remove_reference_t<type>> o)
	{
		{ o.eof() } -> ::std::same_as<bool>;
	};
}

#endif
