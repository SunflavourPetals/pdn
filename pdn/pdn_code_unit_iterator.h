#ifndef PDN_Header_pdn_code_unit_iterator
#define PDN_Header_pdn_code_unit_iterator

#include <type_traits>
#include <concepts>
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

namespace pdn::dev_util
{
	template <typename it_t>
	concept eof_tester_iterator = requires(it_t it) { { it.eof() } -> ::std::convertible_to<bool>; };

	template <unicode::encode_type encode_type, typename it_t>
	class code_unit_iterator_helper final
	{
	public:
		using code_unit_type = unicode::type_traits::code_unit_t<encode_type>;
		using char_type      = code_unit_type;
		using size_type      = ::std::size_t;
		static constexpr size_type bits_count_of_byte{ 8 };
		static constexpr size_type bits_count_of_unit{ sizeof(char_type) * bits_count_of_byte };
		static_assert(bits_count_of_unit >= bits_count_of_byte);
		static constexpr ::std::endian source_endian() noexcept
		{
			return unicode::type_traits::endian_from_encode_type<encode_type>;
		}
		template <typename pred>
		static char_type to_next(it_t& begin, pred&& is_eof)
		{
			if constexpr (source_endian() == ::std::endian::little) // little endian
			{
				return le_next(begin, ::std::forward<pred>(is_eof));
			}
			else // big endian
			{
				return be_next(begin, ::std::forward<pred>(is_eof));
			}
		}
		template <typename pred>
		static char_type first_to_next(it_t& begin, pred&& is_eof)
		{
			if constexpr (source_endian() == ::std::endian::little) // little endian
			{
				return first_le_next(begin, ::std::forward<pred>(is_eof));
			}
			else // big endian
			{
				return first_be_next(begin, ::std::forward<pred>(is_eof));
			}
		}
	private:
		template <typename pred>
		static char_type le_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{}; offset < bits_count_of_unit; offset += bits_count_of_byte)
			{
				// 0x12345678 le -> 0:0x78, 1:0x56, 2:0x34, 3:0x12
				++begin;
				if (is_eof())
				{
					break;
				}
				curr_value |= (char_type(::std::make_unsigned_t<typename it_t::value_type>(*begin)) << offset);
			}
			return curr_value;
		}
		template <typename pred>
		static char_type be_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{ bits_count_of_unit }; offset > 0; )
			{
				// 0x12345678 be -> 0:0x12, 1:0x34, 2:0x56, 3:0x78
				offset -= bits_count_of_byte;
				++begin;
				if (is_eof())
				{
					break;
				}
				curr_value |= (char_type(::std::make_unsigned_t<typename it_t::value_type>(*begin)) << offset);
			}
			return curr_value;
		}
		template <typename pred>
		static char_type first_le_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{}; !is_eof(); )
			{
				curr_value |= (char_type(::std::make_unsigned_t<typename it_t::value_type>(*begin)) << offset);
				offset += bits_count_of_byte;
				if (offset < bits_count_of_unit)
				{
					++begin;
					continue;
				}
				break;
			}
			return curr_value;
		}
		template <typename pred>
		static char_type first_be_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{ bits_count_of_unit }; !is_eof(); )
			{
				offset -= bits_count_of_byte;
				curr_value |= (char_type(::std::make_unsigned_t<typename it_t::value_type>(*begin)) << offset);
				if (offset > 0)
				{
					++begin;
					continue;
				}
				break;
			}
			return curr_value;
		}
	};
}

namespace pdn
{
	template <unicode::encode_type encode_type, typename it_t>
	class code_unit_iterator
	{
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = unicode::type_traits::code_unit_t<encode_type>;
		using char_type         = code_unit_type;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
	private:
		using helper            = dev_util::code_unit_iterator_helper<encode_type, it_t>;
		void first_to_next()
		{
			curr_value = helper::first_to_next(begin, [this]() { return begin == end; });
		}
	public:
		static constexpr ::std::endian source_endian() noexcept
		{
			return helper::source_endian();
		}
		const char_type& get() const noexcept
		{
			return curr_value;
		}
		void to_next()
		{
			curr_value = helper::to_next(begin, [this]() { return begin == end; });
		}
		const char_type& operator*() const noexcept
		{
			return get();
		}
		code_unit_iterator& operator++ ()
		{
			to_next();
			return *this;
		}
		template <typename it_origin_t>
		friend bool operator== (const code_unit_iterator& lhs, const it_origin_t& rhs) noexcept
		{
			return lhs.begin == rhs;
		}
		code_unit_iterator(it_t bytestream_begin, it_t bytestream_end) :
			begin{ ::std::move(bytestream_begin) },
			end{ ::std::move(bytestream_end) }
		{
			first_to_next();
		}
	private:
		it_t      begin;
		it_t      end;
		char_type curr_value{};
	};

	template <unicode::encode_type encode_type, dev_util::eof_tester_iterator it_t>
	class code_unit_iterator<encode_type, it_t>
	{
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = unicode::type_traits::code_unit_t<encode_type>;
		using char_type         = code_unit_type;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
	private:
		using helper            = dev_util::code_unit_iterator_helper<encode_type, it_t>;
		void first_to_next()
		{
			curr_value = helper::first_to_next(begin, [this]() { return begin.eof(); });
		}
	public:
		static constexpr ::std::endian source_endian() noexcept
		{
			return helper::source_endian();
		}
		const char_type& get() const noexcept
		{
			return curr_value;
		}
		void to_next()
		{
			curr_value = helper::to_next(begin, [this]() { return begin.eof(); });
		}
		const char_type& operator*() const noexcept
		{
			return get();
		}
		code_unit_iterator& operator++ ()
		{
			to_next();
			return *this;
		}
		template <typename it_other_t>
		friend bool operator== (const code_unit_iterator& lhs, const it_other_t& rhs) noexcept
		{
			return lhs.begin == rhs;
		}
		explicit code_unit_iterator(it_t bytestream_begin) : begin{ ::std::move(bytestream_begin) } { first_to_next(); }
		code_unit_iterator(it_t bytestream_begin, it_t) : code_unit_iterator(::std::move(bytestream_begin)) {}
	private:
		it_t      begin;
		char_type curr_value{};
	};

	namespace dev_util::for_code_unit_iterator_u8
	{
		template <typename it_t>
		using deref_t = decltype(*::std::declval<it_t>());

		template <typename it_t>
		using it_value_t = ::std::remove_reference_t<deref_t<it_t>>;

		template <typename it_t, typename t>
		concept can_ret_ref =
			   ::std::is_trivial_v<t>
			&& ::std::is_reference_v<deref_t<it_t>>
			&& ::std::is_trivial_v<it_value_t<it_t>>
			&& sizeof(it_value_t<it_t>) == sizeof(t)
			&& requires(it_t it) { reinterpret_cast<::std::add_const_t<t>&>(*it); };
	}

	template <typename it_t>
	class code_unit_iterator<unicode::encode_type::utf_8, it_t>
	{
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = unicode::type_traits::code_unit_t<unicode::encode_type::utf_8>;
		using char_type         = code_unit_type;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
		using deref_value_type  = ::std::conditional_t<dev_util::for_code_unit_iterator_u8::can_ret_ref<it_t, char_type>, ::std::add_const_t<char_type>&, char_type>;
		deref_value_type operator*() const noexcept
		{
			if constexpr (dev_util::for_code_unit_iterator_u8::can_ret_ref<it_t, char_type>)
			{
				return reinterpret_cast<::std::add_const_t<char_type>&>(*begin);
			}
			else
			{
				return *begin; // implicit conversation to code_unit_type
			}
		}
		code_unit_iterator& operator++ ()
		{
			++begin;
			return *this;
		}
		template <typename it_other_t>
		friend bool operator== (const code_unit_iterator& lhs, const it_other_t& rhs) noexcept
		{
			return lhs.begin == rhs;
		}
		explicit code_unit_iterator(it_t bytestream_begin) : begin{ ::std::move(bytestream_begin) } {}
		code_unit_iterator(it_t bytestream_begin, it_t) : code_unit_iterator(::std::move(bytestream_begin)) {}
	private:
		it_t begin;
	};

	template <unicode::encode_type encode_type, typename it_t>
	inline auto make_code_unit_iterator(it_t bytestream_begin, it_t bytestream_end)
	{
		return code_unit_iterator<encode_type, it_t>{ ::std::move(bytestream_begin), ::std::move(bytestream_end) };
	}
}

#endif
