#ifndef PDN_Header_pdn_source_position_recorder_concept
#define PDN_Header_pdn_source_position_recorder_concept

#include <concepts>
#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_source_position.h"

namespace pdn::concepts
{
	// A source position recorder is the part of the function package that answers 'where am I now'
	// (position) and consumes the code points after the lexer has read them (update). The lexer only
	// needs the getter; the code point iterator needs both, because it feeds every code point it hands out.
	//
	// The getter probes a const object: reading a position must not mutate the recorder. The updater
	// probes a mutable object, hence only references are stripped there; its two calls pin both value
	// categories, so update has to accept an lvalue and a prvalue alike.

	template <typename type>
	concept source_position_getter = requires(::std::add_const_t<::std::remove_reference_t<type>> pos_recorder)
	{
		{ pos_recorder.position() } -> ::std::convertible_to<source_position>;
	};

	template <typename type>
	concept source_position_updater = requires(::std::remove_reference_t<type> pos_recorder, unicode::code_point_t c)
	{
		pos_recorder.update(c);
		pos_recorder.update(unicode::code_point_t{});
	};

	template <typename type>
	concept source_position_recorder = source_position_getter<type> && source_position_updater<type>;
}

#endif
