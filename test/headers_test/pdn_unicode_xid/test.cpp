// Tests for pdn/include/pdn/pdn_unicode_xid.h.
//
// The header exposes the XID_Start / XID_Continue properties as generated range tables. The data
// comes from DerivedCoreProperties.txt (Unicode 18.0.0); the totals checked here pin that data
// set, so regenerating the tables from another Unicode version has to update this file too. The
// rest of the checks cover the binary search, the XID_Start is a subset of XID_Continue invariant, the lexer
// predicates built on top of the tables, and one parse of a non-ASCII identifier.

#include <pdn/pdn_lexer_utility.h>
#include <pdn/pdn_parse.h>
#include <pdn/pdn_unicode_xid.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace
{
	using pdn::lexer_utility::is_allowed_as_first_char_of_identifier;
	using pdn::lexer_utility::is_allowed_in_identifier;
	using pdn::unicode::is_xid_continue;
	using pdn::unicode::is_xid_start;

	struct sample
	{
		char32_t cp;
		bool is_start;
		bool is_continue;
	};

	// Code points around range edges and around the XID corner cases: U+00B7 and U+0387 are
	// XID_Continue but not XID_Start (their NFKC forms are XID_Continue), U+037E is neither (its
	// NFKC form is ';'), and the emoji, surrogate and out-of-range code points are in no table.
	constexpr sample samples[] =
	{
		{ 0x00, false, false },
		{ 0x20, false, false },
		{ 0x2B, false, false },
		{ 0x30, false, true },
		{ 0x39, false, true },
		{ 0x3A, false, false },
		{ 0x41, true, true },
		{ 0x5A, true, true },
		{ 0x5F, false, true },
		{ 0x61, true, true },
		{ 0x7A, true, true },
		{ 0x7B, false, false },
		{ 0xAA, true, true },
		{ 0xB5, true, true },
		{ 0xB7, false, true },
		{ 0xBA, true, true },
		{ 0x301, false, true },
		{ 0x37E, false, false },
		{ 0x387, false, true },
		{ 0x386, true, true },
		{ 0x3A3, true, true },
		{ 0x4E00, true, true },
		{ 0x9FFF, true, true },
		{ 0x1F600, false, false },
		{ 0xD800, false, false },
		{ 0x10FFFF, false, false },
		{ 0x110000, false, false },
	};
}

int main()
{
	for (const auto& s : samples)
	{
		if (is_xid_start(s.cp) != s.is_start) return 1;
		if (is_xid_continue(s.cp) != s.is_continue) return 2;
	}

	// XID_Start is a subset of XID_Continue, and the totals pin the Unicode version. The scan
	// walks the whole code space (surrogates included) because the tables never reject a value
	// by range alone.
	auto start_total = ::std::size_t{};
	auto continue_total = ::std::size_t{};
	for (char32_t cp = 0; cp <= 0x10FFFFu; ++cp)
	{
		const bool start = is_xid_start(cp);
		const bool cont = is_xid_continue(cp);
		if (start && !cont) return 3;
		start_total += start;
		continue_total += cont;
	}
	if (start_total != 158716) return 4;
	if (continue_total != 162081) return 5;

	// The lexer fast-paths letters, '_' and digits before the table, so its answer differs from
	// the table exactly for '_' (a start character for the lexer, not for Unicode).
	for (const auto& s : samples)
	{
		const bool first = is_allowed_as_first_char_of_identifier(s.cp);
		const bool inner = is_allowed_in_identifier(s.cp);
		if (first != (s.is_start || s.cp == U'_')) return 6;
		if (inner != s.is_continue) return 7;
	}

	// A character with XID_Start is a valid identifier, so a definition keyed by it parses; a
	// combining mark (XID_Continue only) cannot start an identifier, so the same shape fails.
	auto accepted = pdn::parse(::std::u8string_view{ u8"\u4F60\u597D: 1" }, pdn::utf8_tag);
	if (!accepted) return 8;
	if (accepted.value().as_object().size() != 1) return 9;
	if (accepted.value().as_object().at(::std::u8string{ u8"\u4F60\u597D" }).as_int() != 1) return 10;

	auto rejected = pdn::parse(::std::u8string_view{ u8"\u0301: 1" }, pdn::utf8_tag);
	if (rejected) return 11;

	return 0;
}
