#include <pdn/pdn_source_position_recorder_concept.h>

#include <cassert>
#include <cstddef>

namespace
{
	using pdn::source_position;
	using pdn::unicode::code_point_t;

	// A full recorder: a const getter plus an update that consumes one code point.
	struct good_recorder
	{
		source_position pos{};
		constexpr source_position position() const noexcept { return pos; }
		constexpr void update(code_point_t c) noexcept
		{
			if (c == U'\n')
			{
				++pos.line;
				pos.column = 1;
			}
			else
			{
				++pos.column;
			}
		}
	};
	struct getter_only
	{
		constexpr source_position position() const noexcept { return {}; }
	};
	struct updater_only
	{
		constexpr void update(code_point_t) noexcept {}
	};
	struct position_returns_int
	{
		constexpr int  position() const noexcept { return 0; }
		constexpr void update(code_point_t) noexcept {}
	};
	struct position_is_not_const
	{
		constexpr source_position position() noexcept { return {}; }
		constexpr void update(code_point_t) noexcept {}
	};
	// Each variant accepts only one value category, so the two probes of the updater concept bite.
	struct update_takes_lvalue_ref
	{
		constexpr source_position position() const noexcept { return {}; }
		constexpr void update(code_point_t&) noexcept {}
	};
	struct update_takes_rvalue_ref
	{
		constexpr source_position position() const noexcept { return {}; }
		constexpr void update(code_point_t&&) noexcept {}
	};
	struct update_takes_int
	{
		constexpr source_position position() const noexcept { return {}; }
		constexpr void update(int) noexcept {}
	};
	// The concept asks for a callable update, not for a mutating one.
	struct update_is_const
	{
		constexpr source_position position() const noexcept { return {}; }
		constexpr void update(code_point_t) const noexcept {}
	};

	// Mirrors the real consumer of the composite concept: the code point iterator feeds every code point.
	template <pdn::concepts::source_position_recorder type>
	constexpr source_position feed(type& rec, code_point_t c)
	{
		rec.update(c);
		return rec.position();
	}
	// Mirrors the lexer, which only asks where it is.
	template <pdn::concepts::source_position_getter type>
	constexpr source_position where(const type& rec)
	{
		return rec.position();
	}
}

// 1. A recorder with both members models all three concepts.
static_assert(pdn::concepts::source_position_getter<good_recorder>);
static_assert(pdn::concepts::source_position_updater<good_recorder>);
static_assert(pdn::concepts::source_position_recorder<good_recorder>);

// 2. The two halves are independent, and the composite requires both.
static_assert(pdn::concepts::source_position_getter<getter_only>);
static_assert(!pdn::concepts::source_position_updater<getter_only>);
static_assert(!pdn::concepts::source_position_recorder<getter_only>);
static_assert(!pdn::concepts::source_position_getter<updater_only>);
static_assert(pdn::concepts::source_position_updater<updater_only>);
static_assert(!pdn::concepts::source_position_recorder<updater_only>);

// 3. The getter is probed on a const object, so the position has to be readable from a const recorder.
static_assert(!pdn::concepts::source_position_getter<position_returns_int>);
static_assert(!pdn::concepts::source_position_getter<position_is_not_const>);
static_assert(pdn::concepts::source_position_updater<position_is_not_const>);
static_assert(!pdn::concepts::source_position_recorder<position_is_not_const>);

// 4. The updater is probed with an lvalue and a prvalue, so both value categories have to be accepted.
static_assert(!pdn::concepts::source_position_updater<update_takes_lvalue_ref>);
static_assert(!pdn::concepts::source_position_updater<update_takes_rvalue_ref>);
// An implicit conversion is enough: the concept asks whether the call is well formed, not which parameter type is declared.
static_assert(pdn::concepts::source_position_updater<update_takes_int>);
static_assert(pdn::concepts::source_position_updater<update_is_const>);
static_assert(pdn::concepts::source_position_recorder<update_is_const>);

// 5. References are normalized: <T>, <T&> and <T&&> name the same recorder, a const one loses update.
static_assert(pdn::concepts::source_position_recorder<good_recorder&>);
static_assert(pdn::concepts::source_position_recorder<good_recorder&&>);
static_assert(pdn::concepts::source_position_getter<const good_recorder&>);
static_assert(!pdn::concepts::source_position_updater<const good_recorder&>);
static_assert(!pdn::concepts::source_position_recorder<const good_recorder&>);

// 6. Non-class and incomplete argument types are rejected without a hard error.
static_assert(!pdn::concepts::source_position_recorder<int>);
static_assert(!pdn::concepts::source_position_recorder<int[]>);
static_assert(!pdn::concepts::source_position_recorder<int[3]>);
static_assert(!pdn::concepts::source_position_recorder<void()>);

// 7. The constrained helpers accept the recorder and produce the expected positions.
static_assert([] { good_recorder r{}; return feed(r, U'a').column; }() == 2);
static_assert(where(good_recorder{}).line == 1);

int main()
{
	// 8. Positions advance by code point and reset the column on a line break.
	good_recorder rec{};
	assert(rec.position() == (source_position{ 1, 1 }));
	assert(feed(rec, U'a') == (source_position{ 1, 2 }));
	assert(feed(rec, U'\n') == (source_position{ 2, 1 }));
	assert(feed(rec, U'\u4e2d') == (source_position{ 2, 2 }));
	assert(where(rec) == (source_position{ 2, 2 }));
}
