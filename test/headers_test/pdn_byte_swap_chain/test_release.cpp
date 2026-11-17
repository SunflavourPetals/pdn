// Release mode guard for an empty buffer.
//
// byte_swap_chain requires a non-empty buffer: make_byte_swap_chain turns a zero sized request into
// default_buffer_size, and code that builds a chain directly must not pass an empty buffer. Debug
// builds assert that precondition; this file is built with NDEBUG on purpose so the release fallback
// can be checked: a zero sized chunk must report eof() at once instead of spinning the consumer loop.
//
// Assertions are compiled out here, so failures are reported through the process exit code.

#include <pdn/pdn_adjacent_2_buffer.h>
#include <pdn/pdn_byte_swap_chain.h>

#include <cstdio>
#include <sstream>
#include <string>

namespace
{
	struct counting_deleter
	{
		int* count{};
		void operator()(char* ptr) const noexcept { if (ptr) { ++*count; delete[] ptr; } }
	};

	using custom_chain_type  = pdn::byte_swap_chain<std::istringstream, counting_deleter>;
	using custom_buffer_type = pdn::detail::adjacent_2_buffer<char, counting_deleter>;

	auto check(bool condition, const char* text) -> bool
	{
		if (!condition)
		{
			std::fprintf(stderr, "FAILED: %s\n", text);
		}
		return condition;
	}

	// The number of to_next() calls a consumer loop would perform before it ends.
	template <typename chain_t>
	auto count_steps(chain_t& chain) -> int
	{
		auto steps = 0;
		while (!chain.eof() && !chain.stop_requested() && steps != 8)
		{
			++steps;
			chain.to_next();
		}
		return steps;
	}
}

auto main() -> int
{
	auto ok = true;

	// Default deleter with a default constructed (empty) buffer.
	{
		auto stream = std::istringstream{ "xyz" };
		auto chain  = pdn::byte_swap_chain<std::istringstream>{ stream, pdn::detail::adjacent_2_buffer<char>{} };
		ok = check(chain.chunk_size() == 0, "empty buffer keeps chunk_size() == 0") && ok;
		ok = check(chain.eof(), "empty buffer reports eof()") && ok;
		ok = check(!chain.stop_requested(), "empty buffer does not request a stop") && ok;
		ok = check(count_steps(chain) == 0, "empty buffer never spins") && ok;
	}
	// Custom deleter with a zero sized buffer whose pointer is not null.
	{
		auto freed  = 0;
		auto raw    = new char[2];
		auto stream = std::istringstream{ "xyz" };
		auto chain  = custom_chain_type{ stream, custom_buffer_type{ 0, raw, counting_deleter{ &freed } } };
		ok = check(chain.chunk_size() == 0, "zero sized buffer keeps chunk_size() == 0") && ok;
		ok = check(chain.eof(), "zero sized buffer reports eof()") && ok;
		ok = check(!chain.stop_requested(), "zero sized buffer does not request a stop") && ok;
		ok = check(count_steps(chain) == 0, "zero sized buffer never spins") && ok;
	}
	return ok ? 0 : 1;
}
