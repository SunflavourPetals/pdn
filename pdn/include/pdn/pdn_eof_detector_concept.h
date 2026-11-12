#ifndef PDN_Header_pdn_eof_detector_concept
#define PDN_Header_pdn_eof_detector_concept

#include <concepts>

namespace pdn::concepts
{
	// Models an input source that reports its own exhaustion instead of exposing a begin/end pair.
	// eof() must be callable on a const object, return bool, and be true exactly when nothing is
	// left to read; the library uses it to select the eof() driven overloads and to end its read
	// loops. The check is purely structural, so any type with such an eof() models it.
	template <typename type>
	concept eof_detector = requires(const type o)
	{
		{ o.eof() } -> ::std::same_as<bool>;
	};
}

#endif
