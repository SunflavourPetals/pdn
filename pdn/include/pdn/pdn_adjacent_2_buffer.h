#ifndef PDN_Header_pdn_adjacent_2_buffer
#define PDN_Header_pdn_adjacent_2_buffer

#include <cstddef>
#include <utility>
#include <memory>
#include <limits>
#include <concepts>
#include <type_traits>

namespace pdn::detail
{
	// When non-empty, owns a single allocation of 2 * chunk_size elements and exposes it as two
	// adjacent chunks of chunk_size elements each: [get_buffer_1(), get_buffer_2()) and
	// [get_buffer_2(), get_buffer_2() + chunk_size()).
	//
	// The object is move-only: a move transfers ownership of the allocation and leaves the
	// moved-from object empty, so has_buffer() is false and chunk_size() is 0 afterwards.
	// The pointer accessors are only meaningful while has_buffer()/operator bool() is true.
	template <typename value_t, typename buffer_deleter_t = ::std::default_delete<value_t[]>>
	class adjacent_2_buffer
	{
	private:
		using buffer_manager = ::std::unique_ptr<value_t[], buffer_deleter_t>;
	public:
		using element_type = typename buffer_manager::element_type;
		using deleter_type = typename buffer_manager::deleter_type;
		using pointer = typename buffer_manager::pointer;
		using const_pointer = typename ::std::pointer_traits<pointer>::template rebind<::std::add_const_t<value_t>>;
		using size_type = ::std::size_t;

		bool has_buffer() const noexcept
		{
			return static_cast<bool>(buffer);
		}
		auto chunk_size() const noexcept -> size_type
		{
			return size;
		}
		auto get_buffer_1() const noexcept -> const_pointer
		{
			return buffer.get();
		}
		auto get_buffer_1() noexcept -> pointer
		{
			return buffer.get();
		}
		auto get_buffer_2() const noexcept -> const_pointer
		{
			return buffer.get() + size;
		}
		auto get_buffer_2() noexcept -> pointer
		{
			return buffer.get() + size;
		}

		void swap(adjacent_2_buffer& other) noexcept
		{
			buffer.swap(other.buffer);
			::std::swap(size, other.size);
		}

		friend void swap(adjacent_2_buffer& lhs, adjacent_2_buffer& rhs) noexcept
		{
			lhs.swap(rhs);
		}

		explicit operator bool() const noexcept
		{
			return has_buffer();
		}

		// Largest chunk size such that the allocation, 2 * chunk_size() elements, cannot overflow size_type.
		static constexpr auto truncate_chunk_size(size_type size_per_chunk) noexcept -> size_type
		{
			constexpr auto half_max_size = ::std::numeric_limits<size_type>::max() / 2;
			return size_per_chunk > half_max_size ? half_max_size : size_per_chunk;
		}

		adjacent_2_buffer() = default;

		// Allocates 2 * chunk_size() elements. A requested size of 0 leaves the buffer empty
		// (has_buffer() is false); oversized requests are truncated by truncate_chunk_size.
		explicit adjacent_2_buffer(size_type size_per_chunk)
			requires ::std::same_as<buffer_deleter_t, ::std::default_delete<value_t[]>> :
			buffer{ size_per_chunk ? ::std::make_unique<value_t[]>(truncate_chunk_size(size_per_chunk) * 2) : nullptr },
			size{ truncate_chunk_size(size_per_chunk) } {}

		// Takes ownership of p, which must point to at least 2 * size_per_chunk elements that deleter
		// can release, and which must have been obtained in a way deleter can undo. The caller
		// guarantees both arguments describe the same allocation. A null p is stored as an empty
		// buffer with chunk_size() == 0; a non-null p is kept even when size_per_chunk is 0.
		adjacent_2_buffer(size_type size_per_chunk, pointer p, const deleter_type& deleter) :
			buffer{ p, deleter },
			size{ p ? size_per_chunk : 0 } {}
		// Same as above, moving the deleter into the buffer.
		adjacent_2_buffer(size_type size_per_chunk, pointer p, deleter_type&& deleter) :
			buffer{ p, ::std::move(deleter) },
			size{ p ? size_per_chunk : 0 } {}

		adjacent_2_buffer(adjacent_2_buffer&& other) noexcept :
			buffer{ ::std::move(other.buffer) },
			size{ ::std::exchange(other.size, size_type{}) } {}

		auto operator=(adjacent_2_buffer&& other) noexcept -> adjacent_2_buffer&
		{
			adjacent_2_buffer temporary{ ::std::move(other) };
			swap(temporary);
			return *this;
		}

		adjacent_2_buffer(const adjacent_2_buffer&) = delete;
		auto operator=(const adjacent_2_buffer&) -> adjacent_2_buffer& = delete;

	private:
		buffer_manager buffer{};
		size_type size{};
	};
}

#endif
