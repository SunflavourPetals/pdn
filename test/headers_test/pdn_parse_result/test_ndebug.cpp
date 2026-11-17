// The fallback of parse_result's accessors only exists when NDEBUG is defined: the debug build
// asserts success() instead, so a separate target (built with NDEBUG) pins the release behavior.
#include <pdn/pdn_parse_result.h>

#include <cstdio>
#include <string_view>
#include <type_traits>

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

	using char_t   = pdn::unicode::u8char_t;
	using result_t = pdn::parse_result<char_t>;
}

int main()
{
#ifndef NDEBUG
	std::printf("this target must be compiled with NDEBUG\n");
	return 1;
#else
	const result_t failure{ pdn::parse_error_code::parse_error };
	check(failure.failed(), "constructed as a failure");
	// Reading a failed result is a precondition violation, but with NDEBUG nothing stops it: the
	// accessors hand out the default entity instead of asserting, so the program stays well defined.
	check(failure.value().get_if<pdn::type::i32>() != nullptr, "the default entity is an i32");
	check(failure.value().as_int() == 0, "the default entity holds zero");
	check((*failure).as_int() == 0, "operator* exposes the same entity");
	check(failure->as_int() == 0, "operator-> exposes the same entity");
	check(&failure.value() == failure.operator->(), "all accessors reach the same member");

	const result_t success{ pdn::entity<char_t>{ pdn::type::i32{ 7 } } };
	check(success.success() && success.value().as_int() == 7, "a successful result is unaffected");

	if (fails == 0)
	{
		std::printf("all pdn_parse_result release checks passed\n");
	}
	return fails == 0 ? 0 : 1;
#endif
}
