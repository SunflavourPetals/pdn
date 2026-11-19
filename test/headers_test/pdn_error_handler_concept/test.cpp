#include <cstddef>
#include <cstdio>

#include <pdn/pdn_error_handler.h>
#include <pdn/pdn_error_handler_concept.h>

namespace
{
	// accepted: the shapes the function package can actually drive
	struct good_member
	{
		void handle_error(const pdn::error_message&) {}
		auto error_count() const -> ::std::size_t { return 0; }
	};

	struct good_by_value
	{
		void handle_error(pdn::error_message) {}
		auto error_count() const noexcept -> ::std::size_t { return 0; }
	};

	struct good_noexcept
	{
		void handle_error(const pdn::error_message&) noexcept {}
		auto error_count() const noexcept -> ::std::size_t { return 0; }
	};

	// rejected: error_count() is read from a const function package
	struct bad_count_nonconst
	{
		void handle_error(const pdn::error_message&) {}
		auto error_count() -> ::std::size_t { return 0; }
	};

	struct bad_count_int
	{
		void handle_error(const pdn::error_message&) {}
		auto error_count() const -> int { return 0; }
	};

	struct bad_count_ulong
	{
		void handle_error(const pdn::error_message&) {}
		auto error_count() const -> unsigned long { return 0; }
	};

	// rejected: the package forwards a const message
	struct bad_arg_mutable
	{
		void handle_error(pdn::error_message&) {}
		auto error_count() const -> ::std::size_t { return 0; }
	};

	struct bad_arg_both_nonconst
	{
		void handle_error(pdn::error_message&) {}
		void handle_error(pdn::error_message&&) {}
		auto error_count() const -> ::std::size_t { return 0; }
	};

	// rejected: a member is missing
	struct bad_no_handle
	{
		auto error_count() const -> ::std::size_t { return 0; }
	};

	struct bad_no_count
	{
		void handle_error(const pdn::error_message&) {}
	};

	int fails = 0;
}
#define CHECK(cond) do { if (!(cond)) { std::printf("FAILED: %s (line %d)\n", #cond, __LINE__); ++fails; } } while (0)

namespace
{
	static_assert(pdn::concepts::error_handler<good_member>);
	static_assert(pdn::concepts::error_handler<good_by_value>);
	static_assert(pdn::concepts::error_handler<good_noexcept>);
	static_assert(pdn::concepts::error_handler<good_member&>);
	static_assert(pdn::concepts::error_handler<good_member&&>);

	// a const handler cannot call the non-const handle_error
	static_assert(!pdn::concepts::error_handler<const good_member&>);

	static_assert(!pdn::concepts::error_handler<bad_count_nonconst>);
	static_assert(!pdn::concepts::error_handler<bad_count_int>);
	static_assert(!pdn::concepts::error_handler<bad_count_ulong>);
	static_assert(!pdn::concepts::error_handler<bad_arg_mutable>);
	static_assert(!pdn::concepts::error_handler<bad_arg_both_nonconst>);
	static_assert(!pdn::concepts::error_handler<bad_no_handle>);
	static_assert(!pdn::concepts::error_handler<bad_no_count>);

	static_assert(!pdn::concepts::error_handler<int>);
	static_assert(!pdn::concepts::error_handler<void>);
	static_assert(!pdn::concepts::error_handler<int[]>);
	static_assert(!pdn::concepts::error_handler<void()>);

	// error_counter alone covers only the counting half
	struct counter_only
	{
		auto error_count() const -> ::std::size_t { return 7; }
	};

	static_assert(pdn::concepts::error_counter<counter_only>);
	static_assert(pdn::concepts::error_counter<good_member>);
	static_assert(!pdn::concepts::error_counter<bad_count_nonconst>);
	static_assert(!pdn::concepts::error_counter<bad_count_int>);
	static_assert(!pdn::concepts::error_counter<bad_no_count>);
	static_assert(!pdn::concepts::error_counter<int>);

	// counting does not imply handling
	static_assert(!pdn::concepts::error_handler<counter_only>);

	template <pdn::concepts::error_counter c_t>
	auto counted(const c_t& counter) -> ::std::size_t { return counter.error_count(); }

	// the handlers shipped with the library satisfy the tightened concept
	static_assert(pdn::concepts::error_handler<pdn::default_error_handler>);
	static_assert(pdn::concepts::error_handler<pdn::default_threshold_error_handler>);

	// a constrained helper, mirroring how pdn_function_package body compiles
	template <pdn::concepts::error_handler h_t>
	auto feed(h_t& handler) -> ::std::size_t
	{
		const pdn::error_message msg{};
		handler.handle_error(msg);               // const message, as forwarded by the package
		handler.handle_error(pdn::error_message{}); // temporary, as after prepending the filename
		const h_t& const_handler = handler;
		return const_handler.error_count();      // read from a const package member
	}

	struct counting_handler
	{
		void handle_error(const pdn::error_message&) { ++count; }
		auto error_count() const -> ::std::size_t { return count; }
		::std::size_t count = 0;
	};

	void test_runtime()
	{
		counting_handler handler{};
		CHECK(feed(handler) == 2);
		CHECK(handler.count == 2);
		CHECK(counted(counter_only{}) == 7);

		pdn::default_error_handler quiet{};
		CHECK(quiet.error_count() == 0);
	}
}

int main()
{
	test_runtime();
	if (fails != 0)
	{
		std::printf("%d check(s) failed\n", fails);
	}
	return fails == 0 ? 0 : 1;
}
