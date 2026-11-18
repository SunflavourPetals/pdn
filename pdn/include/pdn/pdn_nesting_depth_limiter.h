#ifndef PDN_Header_pdn_nesting_depth_limiter
#define PDN_Header_pdn_nesting_depth_limiter

#include <cstddef>

#include "pdn_nesting_depth_limiter_concept.h"

// The default implementation of the nesting depth limiter concept. The parser is recursive
// descent, so a deeply nested list or object would recurse until the thread stack runs out; the
// limit turns that into stop_reason_code::nesting_depth_exceed instead. 256 is the single source
// of that number: the default function package forwards to it, and so does the parser's own
// fallback for a package that does not model the concept at all.

namespace pdn
{
	class default_nesting_depth_limiter
	{
	public:
		// The limit reported below, and the number the parser falls back to.
		static constexpr auto default_nesting_depth_limit = ::std::size_t{ 256 };
		// Satisfies concepts::nesting_depth_limiter: callable on a const object, returns size_t.
		static constexpr auto nesting_depth_limit() noexcept -> ::std::size_t
		{
			return default_nesting_depth_limit;
		}
	};
	static_assert(concepts::nesting_depth_limiter<default_nesting_depth_limiter>);
}

#endif
