#ifndef PDN_Header_pdn_byte_swap_chain
#define PDN_Header_pdn_byte_swap_chain

#include <cstddef>
#include <utility>
#include <cassert>

#include "pdn_adjacent_2_buffer.h"
#include "pdn_stop_reason.h"

//    byte input stream (provide: read byte...) such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |
//     v
// >> byte iterator (provide: get byte, move backward(++it), check EOF)
//     |
//     v
//    code unit iterator (provide: get code unit, move backward(++it), check EOF)
//     |
//     +---- utf-8 decoder ---+
//     |                      |
//     +---- utf-16 decoder --+---- (provide: decode to get unicode code-point)
//     |                      |
//     +---- utf-32 decoder --+
//     v
//    code point iterator (provide: get code point, move backward(++it), check EOF)
//     |
//     v
//    lexer (provide: get token{ token code, value, token position })
//     |
//     v
//    parser (provide: parse) --> pdn document object model


namespace pdn
{
	template <typename istream_t, typename buffer_deleter_t>
	class byte_swap_chain;

	struct byte_iterator_end_from_byte_swap_chain {};

	template <typename istream_t, typename buffer_deleter_t>
	class byte_iterator_from_byte_swap_chain
	{
	private:
		using swap_chain_type = byte_swap_chain<istream_t, buffer_deleter_t>;
	public:
		using iterator_concept = void;
		using iterator_category = void;
		using size_type = typename swap_chain_type::size_type;
		using value_type = typename swap_chain_type::value_type;
		bool stop_requested() const noexcept
		{
			return swap_chain.stop_requested();
		}
		auto stop_reason() const noexcept -> stop_reason_code
		{
			return swap_chain.stop_reason();
		}
		bool eof() const noexcept
		{
			return swap_chain.eof();
		}
		auto operator*() const noexcept -> ::std::add_lvalue_reference_t<::std::add_const_t<value_type>>
		{
			return swap_chain.get();
		}
		byte_iterator_from_byte_swap_chain& operator++ () // cannot ++end
		{
			swap_chain.to_next();
			return *this;
		}
		// just for checking eof, returns is lhs eof
		friend bool operator==(const byte_iterator_from_byte_swap_chain& lhs, const byte_iterator_end_from_byte_swap_chain&)
		{
			return lhs.eof();
		}
		explicit byte_iterator_from_byte_swap_chain(swap_chain_type&& swap) : swap_chain{::std::move(swap)} {}
	private:
		swap_chain_type swap_chain{}; // swap_ptr == nullptr means it is end
	};

	template <typename istream_t, typename deleter_t>
	auto make_byte_iterator(byte_swap_chain<istream_t, deleter_t>&& rval)
	{
		return byte_iterator_from_byte_swap_chain{ ::std::move(rval) };
	}

	constexpr auto make_byte_iterator_end() noexcept
	{
		return byte_iterator_end_from_byte_swap_chain{};
	}

	// to_next maybe requests stop; get and eof is safe
	template <typename istream_t, typename buffer_deleter_t = ::std::default_delete<typename istream_t::char_type[]>>
	class byte_swap_chain
	{
	public:
		using istream_type = istream_t;
		using deleter_type = buffer_deleter_t;
		using size_type = ::std::size_t;
		using char_type = typename istream_type::char_type;
		using value_type = char_type;
		using const_iterator = byte_iterator_from_byte_swap_chain<istream_t, buffer_deleter_t>;
		static_assert(sizeof(value_type) == 1, "[pdn] unsupported istream");
	private:
		using buffers_manager = detail::adjacent_2_buffer<char_type, deleter_type>;
		static constexpr auto default_buffer_size = size_type{ 16 * 1024 };
	private:
		void request_stop(stop_reason_code reason) noexcept
		{
			if (stop_reason_c == stop_reason_code{})
			{
				stop_reason_c = reason;
			}
		}
	public:
		bool stop_requested() const noexcept
		{
			return stop_reason_c != stop_reason_code{};
		}
		auto stop_reason() const noexcept -> stop_reason_code
		{
			return stop_reason_c;
		}
		auto get() const noexcept -> ::std::add_lvalue_reference_t<::std::add_const_t<value_type>>
		{
			return *current_pos;
		}
		bool eof() const noexcept
		{
			return current_pos == eof_pos;
		}
		void to_next()
		{
			if (current_pos != eof_pos) // if eof was reached, call fill_to or move current_pos is awful!
			{
				++current_pos;
				// if value type is byte and buffer size = 16, and buffer at = 0x8000'0000
				// then
				// buff 1 at 0x8000'0000
				// buff 2 at 0x8000'0010
				// first byte after buff is 0x8000'0020
				// buff 1, buff 2, buffer end never equal to each other
				if (current_pos >= current_end)
				{
					fill_next_buffer(); // fill next buffer

					auto next_buffer = get_next_buffer();
					current_buffer = next_buffer; // change active buffer

					current_end = calc_end_of_current_buffer(); // update end flag

					if (current_pos >= calc_end_of_whole_buffers()) // reset current pos
					{
						current_pos = buffers_mng.get_buffer_1();
					}
				}
			}
		}
		size_type chunk_size() const noexcept
		{
			return buffers_mng.chunk_size();
		}
	private:
		void fill_to(value_type* target_buffer)
		{
			// if eof was reached or stop requested, then no need to call fill_to anytime
			assert(!istream_ptr->fail() && "[pdn] istream fail or bad in byte_swap_chain::fill_to");
			istream_ptr->read(target_buffer, chunk_size() * sizeof(value_type));
			if (istream_ptr->bad()) [[unlikely]]
			{
				request_stop(stop_reason_code::bad_stream);
				return;
			}
			if (istream_ptr->fail() && !istream_ptr->eof()) [[unlikely]]
			{
				request_stop(stop_reason_code::stream_operation_failed);
				return;
			}
			if (istream_ptr->eof())
			{
				eof_pos = target_buffer + istream_ptr->gcount();
			}
		}
		bool is_working_on_buffer_1() const noexcept
		{
			return current_buffer == buffers_mng.get_buffer_1();
		}
		value_type* get_next_buffer() noexcept
		{
			return is_working_on_buffer_1() ? buffers_mng.get_buffer_2() : buffers_mng.get_buffer_1();
		}
		void fill_next_buffer()
		{
			fill_to(get_next_buffer());
		}
		const value_type* calc_end_of_current_buffer() const noexcept
		{
			return current_buffer + chunk_size();
		}
		const value_type* calc_end_of_whole_buffers() const noexcept
		{
			return buffers_mng.get_buffer_2() + chunk_size();
		}
	public:
		byte_swap_chain(
			istream_type& istream, // should be opened(if is ifstream) and not bad or fail but not eof
			buffers_manager&& adjacent_2_buf = {}) : // adjacent_2_buf have (2 * buf_size) char_type object
			buffers_mng{ adjacent_2_buf ? ::std::move(adjacent_2_buf) : buffers_manager{ default_buffer_size } },
			istream_ptr{ &istream }
		{
			if (istream_ptr->bad()) [[unlikely]]
			{
				request_stop(stop_reason_code::bad_stream);
				return;
			}
			else if (istream_ptr->fail() && !istream_ptr->eof()) [[unlikely]]
			{
				request_stop(stop_reason_code::stream_operation_failed);
				return;
			}
			current_buffer = buffers_mng.get_buffer_1();
			current_pos = current_buffer;
			current_end = calc_end_of_current_buffer();
			eof_pos = nullptr;
			if (istream_ptr->eof())
			{
				eof_pos = current_pos;
			}
			else
			{
				fill_to(current_buffer);
			}
		}
	private:
		buffers_manager buffers_mng{};
		istream_type* istream_ptr{};
		value_type* current_buffer{};
		value_type* current_pos{};
		const value_type* current_end{}; // end of current buffer
		const value_type* eof_pos{};
		stop_reason_code stop_reason_c{};
	};

	template <typename istream_t>
	auto make_byte_swap_chain(istream_t& istream)
	{
		return byte_swap_chain{ istream };
	}

	template <typename istream_t>
	auto make_byte_swap_chain(istream_t& istream, ::std::size_t buf_size)
	{
		return byte_swap_chain{ istream, detail::adjacent_2_buffer<typename istream_t::char_type>{ buf_size } };
	}
}

#endif
