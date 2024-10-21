#ifndef PDN_Header_pdn_swap_chain
#define PDN_Header_pdn_swap_chain

#include <type_traits>
#include <stdexcept>
#include <istream>
#include <fstream>
#include <cstdint>
#include <utility>
#include <limits>
#include <memory>

//    byte input stream (provide: get byte) // such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |         extra: requires byte input stream has operation seekg
//     v
// >> swap chain (provide: get byte)
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
//    parser (provide: parse) ----> pdn document object model

namespace pdn
{
	template <typename istream_t = ::std::istream,
	          typename istream_deleter_t = ::std::default_delete<istream_t>,
	          typename buffer_deleter_t = ::std::default_delete<typename istream_t::char_type[]>>
	class swap_chain
	{
	public:
		using istream_type = istream_t;
		using istream_deleter_type = istream_deleter_t;
		using buffer_deleter_type = buffer_deleter_t;
		using size_type = ::std::size_t;
		using char_type = typename istream_type::char_type;
		struct eof_type final {};
		static constexpr eof_type eof_object{};
	private:
		using istream_manager = ::std::unique_ptr<istream_type, istream_deleter_type>;
		using buffer_manager = ::std::unique_ptr<char_type[], buffer_deleter_type>;
	public:
		char_type get() const
		{
			return *pos;
		}
		bool eof() const noexcept
		{
			return pos == eof_pos;
		}
		void goto_next()
		{
			if (pos != eof_pos)
			{
				if (pos == buffer_1_mid_pos)
				{
					fill_buffer_2();
				}
				else if (pos == buffer_2_mid_pos)
				{
					fill_buffer_1();
				}
				++pos;
				// if value type is byte and buffer size = 16, and buffer at = 0x8000'0000
				// then
				// buff 1 at 0x8000'0000, buff 1 mid = 0x8000'0008
				// buff 1 at 0x8000'0010, buff 1 mid = 0x8000'0018
				// buff end is 0x8000'0020
				// buff 1 mid, buff 2 mid, buffer end never equal to each other
				if (pos >= buffer_end_pos)
				{
					pos = buffer_ptr.get();
				}
			}
		}
		size_type size() const noexcept
		{
			return buffer_size;
		}
		static constexpr eof_type eof_obj() noexcept
		{
			return eof_object;
		}
	private:
		void fill_buffer_1()
		{
			fill_to(buffer_ptr.get());
		}
		void fill_buffer_2()
		{
			fill_to(buffer_ptr.get() + size());
		}
		void fill_to(char_type* selected_buffer)
		{
			// if input_stream not in good state then return
			if (*istream_ptr)
			{
				istream_ptr->read(selected_buffer, size());
				if (istream_ptr->eof())
				{
					eof_pos = selected_buffer + istream_ptr->gcount();
				}
			}
		}
	public:
		char_type operator*() const
		{
			return get();
		}
		swap_chain& operator++ ()
		{
			goto_next();
			return *this;
		}
		friend bool operator== (const swap_chain& lhs, eof_type) noexcept
		{
			return lhs.eof();
		}
	public:
		swap_chain() = default;
		swap_chain(::std::unique_ptr<istream_t, istream_deleter_t>&&  istream,
		           size_type                                          buf_size = 1024,
		           ::std::unique_ptr<char_type[], buffer_deleter_t>&& adjacent_2_buf = nullptr) :
			buffer_size{ buf_size },
			buffer_ptr{ ::std::move(adjacent_2_buf) },
			istream_ptr{ ::std::move(istream) }
		{
			// buffer size in range [16, ::std::numeric_limits<size_type>::max() / 2]
			if (buffer_size <= 0)
			{
				throw ::std::invalid_argument("argument buffer size can not less than or equal to 0");
			}
			else if (buffer_size > (::std::numeric_limits<size_type>::max() / 2))
			{
				throw ::std::invalid_argument("argument buffer size can not greater than numeric_limits<size_type>::max() / 2");
			}
			// istream_ptr cannot be nullptr and cannot have bad state
			// and if it is ifstream then it cannot unopened.
			if (istream_ptr == nullptr)
			{
				throw ::std::invalid_argument("istream_ptr can not be nullptr!");
			}
			else if (istream_ptr->bad())
			{
				throw ::std::invalid_argument("input_stream badbit can not be true!");
			}
			else if (auto ifstream_ptr = dynamic_cast<::std::basic_ifstream<char_type>*>(istream_ptr.get()))
			{
				if (!ifstream_ptr->is_open())
				{
					throw ::std::invalid_argument("istream_ptr is ifstream ptr and file was not opened!");
				}
			}
			// clear failbit when it in fail state
			istream_ptr->clear(istream_ptr->rdstate() & ::std::ios::eofbit);
			// create buffer
			if (!buffer_ptr)
			{
				buffer_ptr = ::std::make_unique<char_type[]>(size() * 2);
			}
			// initialization
			pos = buffer_ptr.get();
			buffer_end_pos = buffer_ptr.get() + (2 * size());
			buffer_1_mid_pos = buffer_ptr.get() + (size() / 2);
			buffer_2_mid_pos = buffer_ptr.get() + size() + (size() / 2);
			if (istream_ptr->eof())
			{
				eof_pos = pos;
				*eof_pos = 0;
			}
			else
			{
				fill_buffer_1();
			}
		}
		swap_chain(const swap_chain&) = delete;
		swap_chain(swap_chain&& o) noexcept
		{
			*this = std::move(o);
		}
		swap_chain& operator=(const swap_chain&) = delete;
		swap_chain& operator=(swap_chain&& rhs) noexcept
		{
			::std::swap(buffer_size, rhs.buffer_size);
			::std::swap(eof_pos, rhs.eof_pos);
			::std::swap(buffer_end_pos, rhs.buffer_end_pos);
			::std::swap(buffer_1_mid_pos, rhs.buffer_1_mid_pos);
			::std::swap(buffer_2_mid_pos, rhs.buffer_2_mid_pos);
			::std::swap(pos, rhs.pos);
			::std::swap(buffer_ptr, rhs.buffer_ptr);
			::std::swap(istream_ptr, rhs.istream_ptr);
			return *this;
		}
	private:
		size_type       buffer_size{};      // size of buffer
		char_type*      eof_pos{};          // eof position is mapped to buffer position
		char_type*      buffer_end_pos{};   // buffer offset "size"*2
		char_type*      buffer_1_mid_pos{}; // buffer 1 offset "size"/2
		char_type*      buffer_2_mid_pos{}; // buffer 2 offset "size"/2
		char_type*      pos{};              // current input position
		buffer_manager  buffer_ptr;         // size = 2*"size"; buffer 1 at buffer, buffer 2 at (buffer offset "size")
		istream_manager istream_ptr;
	};

	template <typename istream_t         = ::std::istream,
	          typename istream_deleter_t = ::std::default_delete<istream_t>,
	          typename buffer_deleter_t  = ::std::default_delete<typename istream_t::char_type[]>>
	inline auto make_swap_chain(::std::unique_ptr<istream_t, istream_deleter_t>&& istream,
	                            ::std::size_t                                     buf_size = 1024,
	                            ::std::unique_ptr<typename istream_t::char_type[], buffer_deleter_t>&& adjacent_2_buf = nullptr)
	{
		return swap_chain{ ::std::move(istream), buf_size,::std::move(adjacent_2_buf) };
	}
}

#endif
