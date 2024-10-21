#ifndef PDN_Header_pdn_code_unit_iterator
#define PDN_Header_pdn_code_unit_iterator

#include <type_traits>
#include <utility>
#include <bit>
#include <limits>
#include <memory>
#include <iterator>

#include "pdn_unicode_base.h"

//    byte input stream (provide: get byte) // such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |         extra: requires byte input stream has operation seekg
//     v
//    swap chain (provide: get byte)
//     |
//     v
// >> code unit iterator (provide: get code unit, move backward(++it), check EOF)
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
	template <unicode::encode_type encode_type, typename swap_chain_t>
	class code_unit_iterator
	{
	public:
		using iterator_category = void; // it does not satisfy the requirements of any legacy iterator
		using code_unit_type    = unicode::type_traits::code_unit_t<encode_type>;
		using char_type         = code_unit_type;
		using size_type         = ::std::size_t;
		using swap_chain_type   = swap_chain_t;
		static constexpr size_type bits_count_of_byte{ 8 };
		static constexpr size_type bits_count_of_unit{ sizeof(char_type) * bits_count_of_byte };
		static_assert(bits_count_of_unit >= bits_count_of_byte);
	public:
		static constexpr ::std::endian source_endian() noexcept
		{
			return unicode::type_traits::endian_from_encode_type<encode_type>;
		}
	public:
		char_type get() const noexcept
		{
			return curr_value;
		}
		bool eof() const noexcept
		{
			return swap_chain.eof();
		}
		void goto_next()
		{
			if constexpr (source_endian() == ::std::endian::little) // little endian
			{
				le_next();
			}
			else // big endian
			{
				be_next();
			}
		}
	private:
		void le_next()
		{
			curr_value = char_type{};
			for (size_type offset{}; offset < bits_count_of_unit && !swap_chain.eof(); offset += bits_count_of_byte)
			{
				// 0x12345678 le -> 0:0x78, 1:0x56, 2:0x34, 3:0x12
				++swap_chain;
				curr_value |= (char_type(::std::make_unsigned_t<typename swap_chain_type::char_type>(swap_chain.get())) << offset);
			}
		}
		void be_next()
		{
			curr_value = char_type{};
			for (size_type offset{ bits_count_of_unit }; offset > 0 && !swap_chain.eof(); )
			{
				// 0x12345678 be -> 0:0x12, 1:0x34, 2:0x56, 3:0x78
				offset -= bits_count_of_byte;
				++swap_chain;
				curr_value |= (char_type(::std::make_unsigned_t<typename swap_chain_type::char_type>(swap_chain.get())) << offset);
			}
		}
		void first_le_next()
		{
			curr_value = char_type{};
			for (size_type offset{}; ; )
			{
				curr_value |= (char_type(::std::make_unsigned_t<typename swap_chain_type::char_type>(swap_chain.get())) << offset);
				offset += bits_count_of_byte;
				if (offset < bits_count_of_unit && !swap_chain.eof())
				{
					++swap_chain;
					continue;
				}
				break;
			}
		}
		void first_be_next()
		{
			curr_value = char_type{};
			for (size_type offset{ bits_count_of_unit }; ; )
			{
				offset -= bits_count_of_byte;
				curr_value |= (char_type(::std::make_unsigned_t<typename swap_chain_type::char_type>(swap_chain.get())) << offset);
				if (offset > 0 && !swap_chain.eof())
				{
					++swap_chain;
					continue;
				}
				break;
			}
		}
		void first_goto_next()
		{
			if constexpr (source_endian() == ::std::endian::little) // little endian
			{
				first_le_next();
			}
			else // big endian
			{
				first_be_next();
			}
		}
	public:
		char_type operator*() const noexcept
		{
			return get();
		}
		code_unit_iterator& operator++ ()
		{
			goto_next();
			return *this;
		}
		template <typename it_t>
		friend bool operator== (const code_unit_iterator& rhs, const it_t& lhs) noexcept
		{
			return rhs.swap_chain == lhs;
		}
	public:
		explicit code_unit_iterator(swap_chain_t swap_chain) : swap_chain{ ::std::move(swap_chain) }
		{
			first_goto_next();
		}
		code_unit_iterator(const code_unit_iterator&) = delete;
		code_unit_iterator(code_unit_iterator&& o) noexcept
		{
			*this = ::std::move(o);
		}
		code_unit_iterator& operator=(const code_unit_iterator&) = delete;
		code_unit_iterator& operator=(code_unit_iterator&& o) noexcept
		{
			::std::swap(swap_chain, o.swap_chain);
			::std::swap(curr_value, o.curr_value);
			return *this;
		}
	private:
		swap_chain_type swap_chain;
		char_type curr_value{};
	};

	template <unicode::encode_type encode_type, typename swap_chain_t> // pdn::swap_chain like
	inline auto make_code_unit_iterator(swap_chain_t swap_chain)
	{
		return code_unit_iterator<encode_type, swap_chain_t>{ ::std::move(swap_chain) };
	}
}

#endif
