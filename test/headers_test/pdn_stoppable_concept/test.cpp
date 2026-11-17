#include <pdn/pdn_function_package.h>
#include <pdn/pdn_stoppable_concept.h>

#include <cassert>
#include <cstdio>

namespace
{
	struct stopping
	{
		bool stop_requested() const { return true; }
		auto stop_reason() const -> pdn::stop_reason_code { return pdn::stop_reason_code::user_cancelled; }
	};

	struct not_stopping
	{
		bool stop_requested() const { return false; }
		auto stop_reason() const -> pdn::stop_reason_code { return {}; }
	};

	struct noexcept_members
	{
		bool stop_requested() const noexcept { return false; }
		auto stop_reason() const noexcept -> pdn::stop_reason_code { return {}; }
	};

	// The library polls a const object, so non-const members must not satisfy the concept.
	struct non_const_members
	{
		bool stop_requested() { return true; }
		auto stop_reason() -> pdn::stop_reason_code { return {}; }
	};

	struct missing_stop_reason
	{
		bool stop_requested() const { return false; }
	};

	// The next two satisfy convertible_to<bool> but not same_as<bool>.
	struct returns_int
	{
		int stop_requested() const { return 0; }
		auto stop_reason() const -> pdn::stop_reason_code { return {}; }
	};

	struct returns_pointer
	{
		void* stop_requested() const { return nullptr; }
		auto stop_reason() const -> pdn::stop_reason_code { return {}; }
	};

	// Satisfies convertible_to<stop_reason_code> through an implicit conversion, but not same_as.
	struct returns_convertible_reason
	{
		struct to_reason
		{
			operator pdn::stop_reason_code() const { return {}; }
		};
		bool stop_requested() const { return false; }
		auto stop_reason() const -> to_reason { return {}; }
	};

	static_assert(pdn::concepts::stoppable<stopping>);
	static_assert(pdn::concepts::stoppable<not_stopping>);
	static_assert(pdn::concepts::stoppable<noexcept_members>);
	static_assert(pdn::concepts::stoppable<const stopping>);
	static_assert(pdn::concepts::stoppable<stopping&>);
	static_assert(pdn::concepts::stoppable<const stopping&>);
	static_assert(pdn::concepts::stoppable<stopping&&>);

	static_assert(!pdn::concepts::stoppable<non_const_members>);
	static_assert(!pdn::concepts::stoppable<non_const_members&>); // references are stripped, so T& keeps the const check
	static_assert(!pdn::concepts::stoppable<missing_stop_reason>);
	static_assert(!pdn::concepts::stoppable<returns_int>);
	static_assert(!pdn::concepts::stoppable<returns_pointer>);
	static_assert(!pdn::concepts::stoppable<returns_convertible_reason>);
	static_assert(!pdn::concepts::stoppable<int>);

	// The library's own stoppables model the concept.
	static_assert(pdn::concepts::stoppable<pdn::default_function_package<char8_t>>);
	static_assert(pdn::concepts::stoppable<pdn::default_function_package<char16_t>>);
	static_assert(pdn::concepts::stoppable<pdn::default_function_package<char32_t>>);

	template <pdn::concepts::stoppable type>
	auto reason_after_polling(const type& source) -> pdn::stop_reason_code
	{
		return source.stop_requested() ? source.stop_reason() : pdn::stop_reason_code{};
	}

	struct stops_on_third_poll
	{
		mutable int poll_count = 0;
		bool stop_requested() const { return ++poll_count >= 3; }
		auto stop_reason() const -> pdn::stop_reason_code { return pdn::stop_reason_code::user_cancelled; }
	};

	void test_runtime()
	{
		assert(reason_after_polling(stopping{}) == pdn::stop_reason_code::user_cancelled);
		assert(reason_after_polling(not_stopping{}) == pdn::stop_reason_code{});

		stops_on_third_poll source{};
		int polls = 0;
		while (!source.stop_requested() && polls < 10)
		{
			++polls;
		}
		assert(polls == 2);
		assert(source.poll_count == 3);

		std::printf("test_pdn_stoppable_concept passed\n");
	}
}

int main()
{
	test_runtime();
}