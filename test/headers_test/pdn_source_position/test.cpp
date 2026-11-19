#include <concepts>
#include <cstddef>
#include <type_traits>

#include <pdn/pdn_source_position.h>
#include <pdn/pdn_source_position_recorder.h>

namespace
{
	using pdn::source_position;

	// 1. Default construction: line and column both start at 1 (the beginning of the text).
	static_assert(source_position{}.line == 1);
	static_assert(source_position{}.column == 1);

	// 2. Still an aggregate: brace initialization in declaration order works.
	static_assert(::std::is_aggregate_v<source_position>);
	static_assert(source_position{ 3, 5 }.line == 3);
	static_assert(source_position{ 3, 5 }.column == 5);

	// 3. == and != (!= is synthesized from == by the C++20 rewrite rules; the header does not define it).
	constexpr bool equality_contract()
	{
		constexpr source_position a{ 2, 7 };
		constexpr source_position b{ 2, 7 };
		constexpr source_position c{ 2, 8 };
		static_assert(a == b);
		static_assert(!(a != b));
		static_assert(a != c);
		return true;
	}
	static_assert(equality_contract());
	static_assert(::std::equality_comparable<source_position>);

	// 4. Value semantics: trivially copyable, standard layout, passed by value as two size_t.
	static_assert(::std::is_trivially_copyable_v<source_position>);
	static_assert(::std::is_standard_layout_v<source_position>);
	static_assert(sizeof(source_position) == 2 * sizeof(::std::size_t));

	// 5. The public scalar type of line and column.
	static_assert(::std::same_as<source_position::text_position_type, ::std::size_t>);

	// 6. Contract check (depends on the not yet reviewed pdn_source_position_recorder.h):
	//    the column counts code points; after a line break the line increments and the column
	//    returns to 1; CRLF counts as a single line break.
	constexpr bool recorder_contract()
	{
		pdn::source_position_recorder r{};
		if (r.position() != source_position{ 1, 1 }) return false;
		r.update(U'a');
		if (r.position() != source_position{ 1, 2 }) return false;
		r.update(U'\r');
		r.update(U'\n');
		if (r.position() != source_position{ 2, 1 }) return false;
		r.update(U'\u2028');
		if (r.position() != source_position{ 3, 1 }) return false;
		return true;
	}
	static_assert(recorder_contract());
}

int main() {}
