// Review test for pdn_adjacent_2_buffer.h.
// The class is a move-only owner of one allocation exposed as two adjacent chunks, so the
// tests cover: the empty state, allocation and chunk adjacency, the zero-size branch, move
// and swap (including self-move and self-swap), size truncation, a custom deleter, and the
// type-level guarantees the library relies on.

#include <pdn/pdn_adjacent_2_buffer.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{
	using pdn::detail::adjacent_2_buffer;

	struct counting_deleter
	{
		int* count;
		void operator()(char* p) const noexcept
		{
			if (p) ++*count;
			delete[] p;
		}
	};

	struct no_default_deleter
	{
		no_default_deleter() = delete;
		explicit no_default_deleter(int) {}
		void operator()(char* p) const noexcept { delete[] p; }
	};

	template <typename t>
	void self_move_assign(t& value)
	{
		value = ::std::move(value);
	}

	void test_empty()
	{
		adjacent_2_buffer<char> b{};
		assert(!b.has_buffer());
		assert(!static_cast<bool>(b));
		assert(b.chunk_size() == 0);
		assert(b.get_buffer_1() == nullptr);
		assert(b.get_buffer_2() == nullptr);
	}

	void test_allocation()
	{
		adjacent_2_buffer<char> b{ 8 };
		assert(b.has_buffer());
		assert(static_cast<bool>(b));
		assert(b.chunk_size() == 8);
		assert(b.get_buffer_2() == b.get_buffer_1() + 8);

		for (::std::size_t i = 0; i != 8; ++i)
		{
			b.get_buffer_1()[i] = static_cast<char>('a' + i);
			b.get_buffer_2()[i] = static_cast<char>('A' + i);
		}
		for (::std::size_t i = 0; i != 8; ++i) // the two chunks do not overlap
		{
			assert(b.get_buffer_1()[i] == static_cast<char>('a' + i));
			assert(b.get_buffer_2()[i] == static_cast<char>('A' + i));
		}
	}

	void test_zero_size()
	{
		// The allocating constructor treats 0 as "no buffer", not as a valid empty chunk.
		adjacent_2_buffer<char> b{ 0 };
		assert(!b.has_buffer());
		assert(b.chunk_size() == 0);
	}

	void test_move()
	{
		adjacent_2_buffer<char> a{ 4 };
		a.get_buffer_1()[0] = 'x';

		adjacent_2_buffer<char> b{ ::std::move(a) };
		assert(!a.has_buffer() && a.chunk_size() == 0);
		assert(b.has_buffer() && b.chunk_size() == 4 && b.get_buffer_1()[0] == 'x');

		adjacent_2_buffer<char> c{};
		c = ::std::move(b);
		assert(!b.has_buffer() && b.chunk_size() == 0);
		assert(c.has_buffer() && c.chunk_size() == 4 && c.get_buffer_1()[0] == 'x');

		// Move assignment is implemented as move-and-swap, so a self move keeps the buffer.
		auto* before = c.get_buffer_1();
		self_move_assign(c);
		assert(c.has_buffer() && c.chunk_size() == 4 && c.get_buffer_1() == before);
	}

	void test_swap()
	{
		adjacent_2_buffer<char> a{ 2 };
		adjacent_2_buffer<char> b{ 6 };
		auto* pa = a.get_buffer_1();
		auto* pb = b.get_buffer_1();

		swap(a, b);
		assert(a.chunk_size() == 6 && a.get_buffer_1() == pb);
		assert(b.chunk_size() == 2 && b.get_buffer_1() == pa);

		a.swap(a); // self swap is a no-op
		assert(a.chunk_size() == 6 && a.get_buffer_1() == pb);
	}

	void test_truncate()
	{
		constexpr auto half = ::std::numeric_limits<::std::size_t>::max() / 2;
		static_assert(adjacent_2_buffer<char>::truncate_chunk_size(0) == 0);
		static_assert(adjacent_2_buffer<char>::truncate_chunk_size(7) == 7);
		static_assert(adjacent_2_buffer<char>::truncate_chunk_size(half) == half);
		static_assert(adjacent_2_buffer<char>::truncate_chunk_size(half + 1) == half);
		static_assert(adjacent_2_buffer<char>::truncate_chunk_size(::std::numeric_limits<::std::size_t>::max())
			== half);
	}

	void test_custom_deleter()
	{
		int released = 0;
		{
			adjacent_2_buffer<char, counting_deleter> b{ 8, new char[16], counting_deleter{ &released } };
			assert(b.has_buffer() && b.chunk_size() == 8);
			assert(b.get_buffer_2() == b.get_buffer_1() + 8);
		}
		assert(released == 1);

		// Move assignment releases the buffer it replaces.
		released = 0;
		adjacent_2_buffer<char, counting_deleter> a{ 8, new char[16], counting_deleter{ &released } };
		adjacent_2_buffer<char, counting_deleter> b{ 4, new char[8], counting_deleter{ &released } };
		a = ::std::move(b);
		assert(released == 1);
		assert(a.chunk_size() == 4 && a.has_buffer());
		assert(!b.has_buffer() && b.chunk_size() == 0);
	}

	void test_null_pointer_with_size()
	{
		// A null pointer is stored as an empty buffer with chunk_size() == 0, so the stored size
		// never disagrees with has_buffer(); a nonzero size used to be kept and made
		// get_buffer_2() compute nullptr + size (undefined behavior).
		int released = 0;
		{
			adjacent_2_buffer<char, counting_deleter> b{ 16, nullptr, counting_deleter{ &released } };
			assert(!b.has_buffer());
			assert(b.chunk_size() == 0);
			assert(b.get_buffer_1() == nullptr);
			assert(b.get_buffer_2() == nullptr);
		}
		assert(released == 0); // a null pointer is never handed to the deleter

		// A non-null pointer with size 0 is kept: it still owns memory, the chunk just has no room.
		int released_2 = 0;
		{
			adjacent_2_buffer<char, counting_deleter> b{ 0, new char[2], counting_deleter{ &released_2 } };
			assert(b.has_buffer());
			assert(b.chunk_size() == 0);
			assert(b.get_buffer_2() == b.get_buffer_1());
		}
		assert(released_2 == 1);
	}

	void test_type_properties()
	{
		using buffer_t = adjacent_2_buffer<char>;
		static_assert(!::std::is_copy_constructible_v<buffer_t>);
		static_assert(!::std::is_copy_assignable_v<buffer_t>);
		static_assert(::std::is_nothrow_move_constructible_v<buffer_t>);
		static_assert(::std::is_nothrow_move_assignable_v<buffer_t>);
		static_assert(::std::is_default_constructible_v<buffer_t>);
		static_assert(::std::is_nothrow_swappable_v<buffer_t>);

		// With a non default constructible deleter the class is still usable, just not default constructible.
		using no_default_t = adjacent_2_buffer<char, no_default_deleter>;
		static_assert(!::std::is_default_constructible_v<no_default_t>);
		static_assert(::std::is_constructible_v<no_default_t, ::std::size_t, char*, no_default_deleter&&>);
		static_assert(::std::is_constructible_v<no_default_t, ::std::size_t, char*, const no_default_deleter&>);

		// Const accessors hand out pointers to const elements, non-const ones plain pointers.
		static_assert(::std::is_same_v<decltype(::std::declval<const buffer_t&>().get_buffer_1()), const char*>);
		static_assert(::std::is_same_v<decltype(::std::declval<const buffer_t&>().get_buffer_2()), const char*>);
		static_assert(::std::is_same_v<decltype(::std::declval<buffer_t&>().get_buffer_1()), char*>);
	}
}

int main()
{
	test_empty();
	test_allocation();
	test_zero_size();
	test_move();
	test_swap();
	test_truncate();
	test_custom_deleter();
	test_null_pointer_with_size();
	test_type_properties();
	::std::puts("pdn_adjacent_2_buffer: all checks passed");
	return 0;
}
