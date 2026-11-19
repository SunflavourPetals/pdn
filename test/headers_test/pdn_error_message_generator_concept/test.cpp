#include <cstdio>
#include <type_traits>

#include <pdn/pdn_error_message_generator_concept.h>

namespace
{
	int fails = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			++fails;
			std::printf("FAILED: %s\n", what);
		}
	}

	using pdn::error_msg_string;
	using pdn::raw_error_message;
	using pdn::concepts::error_message_generator;

	// The guide only requires a generate_error_message for the function package, so both a static
	// function and a member function must satisfy the concept: the requires-expression calls the
	// function through an object, which works for either.
	struct static_generator
	{
		static auto generate_error_message(raw_error_message src) -> error_msg_string;
	};
	struct member_generator
	{
		auto generate_error_message(raw_error_message src) -> error_msg_string;
	};
	struct const_member_generator
	{
		auto generate_error_message(raw_error_message src) const -> error_msg_string;
	};

	static_assert(error_message_generator<static_generator>);
	static_assert(error_message_generator<member_generator>);
	static_assert(error_message_generator<const_member_generator>);

	// Both value categories are required: the default generator consumes the payload while composing
	// the text, so a generator that only accepts an rvalue (or only an lvalue) is rejected.
	struct rvalue_only_generator
	{
		auto generate_error_message(raw_error_message&& src) -> error_msg_string;
	};
	struct lvalue_only_generator
	{
		auto generate_error_message(raw_error_message& src) -> error_msg_string;
	};

	static_assert(!error_message_generator<rvalue_only_generator>);
	static_assert(!error_message_generator<lvalue_only_generator>);

	// The result must be usable where the handler expects an error_msg_string.
	struct wrong_result_generator
	{
		auto generate_error_message(raw_error_message src) -> int;
	};
	struct no_generator
	{
	};

	static_assert(!error_message_generator<wrong_result_generator>);
	static_assert(!error_message_generator<no_generator>);
	static_assert(!error_message_generator<int>);

	// The concept only names a hypothetical object: the generator itself need not be copyable or
	// default constructible.
	struct non_copyable_generator
	{
		non_copyable_generator()                        = default;
		non_copyable_generator(const non_copyable_generator&) = delete;
		auto generate_error_message(raw_error_message src) -> error_msg_string;
	};

	static_assert(error_message_generator<non_copyable_generator>);

	void test_error_msg_string_is_the_result_type()
	{
		check(::std::is_same_v<decltype(static_generator::generate_error_message(::std::declval<raw_error_message>())), error_msg_string>,
			"the example generator returns error_msg_string");
	}
}

int main()
{
	test_error_msg_string_is_the_result_type();
	return fails == 0 ? 0 : 1;
}
