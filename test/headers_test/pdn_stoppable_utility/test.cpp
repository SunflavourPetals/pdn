#include <cassert>
#include <type_traits>
#include <utility>

#include <pdn/pdn_stoppable_utility.h>

namespace
{
	namespace d = pdn::detail;

	struct all_noexcept
	{
		bool              stop_requested() const noexcept { return false; }
		pdn::stop_reason_code stop_reason() const noexcept { return {}; }
	};
	struct requested_throws
	{
		bool              stop_requested() const { return false; }
		pdn::stop_reason_code stop_reason() const noexcept { return {}; }
	};
	struct reason_throws
	{
		bool              stop_requested() const noexcept { return false; }
		pdn::stop_reason_code stop_reason() const { return {}; }
	};
	struct nothing_noexcept
	{
		bool              stop_requested() const { return false; }
		pdn::stop_reason_code stop_reason() const { return {}; }
	};

	// Mirrors the real use: a forwarding layer repeats the noexcept-ness of the type it wraps.
	template <typename wrapped_type>
	struct forwarder
	{
		wrapped_type wrapped;

		bool stop_requested() const noexcept(d::noexcept_stop_requested_v<wrapped_type>)
		{
			return wrapped.stop_requested();
		}
		pdn::stop_reason_code stop_reason() const noexcept(d::noexcept_stop_reason_v<wrapped_type>)
		{
			return wrapped.stop_reason();
		}
	};
}

// 1. The traits read the noexcept-ness of the two members independently.
static_assert(d::noexcept_stop_requested_v<all_noexcept>);
static_assert(d::noexcept_stop_reason_v<all_noexcept>);
static_assert(!d::noexcept_stop_requested_v<requested_throws>);
static_assert(d::noexcept_stop_reason_v<requested_throws>);
static_assert(d::noexcept_stop_requested_v<reason_throws>);
static_assert(!d::noexcept_stop_reason_v<reason_throws>);
static_assert(!d::noexcept_stop_requested_v<nothing_noexcept>);
static_assert(!d::noexcept_stop_reason_v<nothing_noexcept>);

// 2. References are irrelevant: the probe is always on a const lvalue, so <T>, <T&>, <const T&> and <T&&> agree.
static_assert(d::noexcept_stop_requested_v<all_noexcept&>);
static_assert(d::noexcept_stop_requested_v<const all_noexcept&>);
static_assert(d::noexcept_stop_requested_v<all_noexcept&&>);
static_assert(!d::noexcept_stop_requested_v<requested_throws&>);

// 3. The struct form agrees with the variable template (it is what the variable template is built from).
static_assert(d::noexcept_stop_requested<all_noexcept>::value == d::noexcept_stop_requested_v<all_noexcept>);
static_assert(d::noexcept_stop_reason<requested_throws>::value == d::noexcept_stop_reason_v<requested_throws>);

// 4. A forwarder's noexcept-ness follows the wrapped type instead of being unconditionally noexcept.
static_assert(noexcept(::std::declval<const forwarder<all_noexcept>&>().stop_requested()));
static_assert(noexcept(::std::declval<const forwarder<all_noexcept>&>().stop_reason()));
static_assert(!noexcept(::std::declval<const forwarder<requested_throws>&>().stop_requested()));
static_assert(noexcept(::std::declval<const forwarder<requested_throws>&>().stop_reason()));
static_assert(!noexcept(::std::declval<const forwarder<reason_throws>&>().stop_reason()));

// 5. The traits are constrained: a type that does not model stoppable cannot even be named.
template <typename t>
concept has_requested_trait = requires { d::noexcept_stop_requested_v<t>; };
static_assert(!has_requested_trait<int>);
static_assert(has_requested_trait<all_noexcept>);

int main()
{
	// The traits are compile time only; a runtime pass proves the forwarder keeps working values unchanged.
	forwarder<all_noexcept> f{};
	assert(!f.stop_requested());
	assert(f.stop_reason() == pdn::stop_reason_code{});
}