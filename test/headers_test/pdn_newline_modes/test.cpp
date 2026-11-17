// Review test for pdn_newline_modes.h.
// The header has no run-time behaviour, only a set of bit flags and four bit operators,
// so everything is pinned at compile time: the values, the default, the combine/test
// semantics, and the easy-to-misuse property that == holds only for exactly one kind.

#include <type_traits>

#include <pdn/pdn_newline_modes.h>

using pdn::newline_modes;

// 1. NONE is a clean zero: the position recorder initializes with newline_modes{} and relies on it.
static_assert(static_cast<int>(newline_modes::NONE) == 0);
static_assert(newline_modes{} == newline_modes::NONE);

// 2. Every kind owns one bit; all six bits are distinct and together make 0x3F.
static_assert(static_cast<int>(newline_modes::CRLF) == 0x01);
static_assert(static_cast<int>(newline_modes::LF) == 0x02);
static_assert(static_cast<int>(newline_modes::CR) == 0x04);
static_assert(static_cast<int>(newline_modes::NEL) == 0x08);
static_assert(static_cast<int>(newline_modes::LS) == 0x10);
static_assert(static_cast<int>(newline_modes::PS) == 0x20);
static_assert((newline_modes::CRLF | newline_modes::LF | newline_modes::CR
             | newline_modes::NEL | newline_modes::LS | newline_modes::PS)
              == static_cast<newline_modes>(0x3F));

// 3. Combine and test: | and & behave like sets, |= and &= are the in-place versions.
constexpr newline_modes mixed = newline_modes::CRLF | newline_modes::LF;
static_assert(static_cast<int>(mixed) == 0x03);
static_assert((mixed & newline_modes::LF) == newline_modes::LF);
static_assert((mixed & newline_modes::CR) == newline_modes::NONE);
static_assert((mixed & newline_modes::CRLF) == newline_modes::CRLF);

// 4. == holds only for exactly one kind: a mixed value equals no single kind.
//    To ask "does it contain LF", use &; writing == newline_modes::LF silently yields false.
static_assert(mixed != newline_modes::LF);
static_assert(mixed != newline_modes::CRLF);
static_assert((mixed & newline_modes::LF) == newline_modes::LF);

// 5. All four operators work in constant expressions and chain (this is how the recorder builds its result).
constexpr newline_modes chained = [] {
	newline_modes m{};
	m |= newline_modes::NEL;
	m |= newline_modes::PS;
	m &= newline_modes::PS;
	return m;
}();
static_assert(chained == newline_modes::PS);

// 6. Strongly typed: neither implicitly constructible from an integer nor implicitly convertible to one.
static_assert(!std::is_convertible_v<int, newline_modes>);
static_assert(!std::is_convertible_v<newline_modes, int>);

int main()
{
	return 0;
}
