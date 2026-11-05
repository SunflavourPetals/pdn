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
#include "pdn_false.h"
#include "pdn_source_position_recorder_concept.h"
#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_stop_reason.h"
#include "pdn_stoppable_concept.h"
#include "pdn_stoppable_utility.h"

// requires 1 Byte = 8 bits

//    byte input stream (provide: get byte) // such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |         extra: requires byte input stream has operation seekg
//     v
//    byte iterator and swap chain (provide: get byte)
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

namespace pdn::detail
{
	static_assert(sizeof(unicode::u8char_t)  == 1, "[pdn] code_unit_iterator: unsupported platform");
	static_assert(sizeof(unicode::u16char_t) == 2, "[pdn] code_unit_iterator: unsupported platform");
	static_assert(sizeof(unicode::u32char_t) == 4, "[pdn] code_unit_iterator: unsupported platform");

	// only for utf16 and utf32
	template <unicode::encode_type encode_type, typename it_t>
	class mb_code_unit_iterator_base
	{
	public:
		static_assert(encode_type != unicode::encode_type::unknown && encode_type != unicode::encode_type::utf8,
			"[pdn] code_unit_iterator_helper: unsupported encode");
		using code_unit_type = unicode::type_traits::code_unit_t<encode_type>;
		using char_type      = code_unit_type;
		using size_type      = ::std::size_t;
		using unsigned_it_value_t = ::std::make_unsigned_t<::std::remove_cvref_t<decltype(*::std::declval<it_t&>())>>;
		static constexpr size_type bits_count_of_byte{ 8 }; // requires 1 Byte = 8 bits
		static constexpr size_type bits_count_of_unit{ sizeof(char_type) * bits_count_of_byte };
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
		template <typename pred> requires (source_endian() == ::std::endian::little)
		static char_type le_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{}; offset < bits_count_of_unit; offset += bits_count_of_byte)
			{
				// 0x12345678 le -> 0:0x78, 1:0x56, 2:0x34, 3:0x12
				++begin;
				if (is_eof()) [[unlikely]]
				{
					break;
				}
				curr_value |= (char_type(unsigned_it_value_t(*begin)) << offset);
			}
			return curr_value;
		}
		template <typename pred> requires (source_endian() == ::std::endian::big)
		static char_type be_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{ bits_count_of_unit }; offset > 0; )
			{
				// 0x12345678 be -> 0:0x12, 1:0x34, 2:0x56, 3:0x78
				offset -= bits_count_of_byte;
				++begin;
				if (is_eof()) [[unlikely]]
				{
					break;
				}
				curr_value |= (char_type(unsigned_it_value_t(*begin)) << offset);
			}
			return curr_value;
		}
		template <typename pred> requires (source_endian() == ::std::endian::little)
		static char_type first_le_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{}; !is_eof(); )
			{
				curr_value |= (char_type(unsigned_it_value_t(*begin)) << offset);
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
		template <typename pred> requires (source_endian() == ::std::endian::big)
		static char_type first_be_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (size_type offset{ bits_count_of_unit }; !is_eof(); )
			{
				offset -= bits_count_of_byte;
				curr_value |= (char_type(unsigned_it_value_t(*begin)) << offset);
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
	
	template <typename type>
	concept eof_checker = requires(const type o)
	{
		{ o.eof() } -> ::std::convertible_to<bool>;
	};

	template <typename it_t>
	struct code_unit_iterator_compressed final
	{
		it_t begin;
		it_t end;
		bool eof() const
		{
			return begin == end;
		}
	};

	template <eof_checker it_t>
	struct code_unit_iterator_compressed<it_t> final
	{
		it_t begin;
		bool eof() const
		{
			return begin.eof();
		}
	};

	// for multi-byte code unit type(utf16 or utf32)
	template <unicode::encode_type encode_type, typename it_t>
	class mb_code_unit_iterator
	{
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = unicode::type_traits::code_unit_t<encode_type>;
		using char_type         = code_unit_type;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
	private:
		using helper            = mb_code_unit_iterator_base<encode_type, it_t>;
		void first_to_next()
		{
			curr_value = helper::first_to_next(compressed.begin, [this]() { return compressed.eof(); });
		}
	public:
		static constexpr ::std::endian source_endian() noexcept
		{
			return helper::source_endian();
		}
		bool stop_requested() const noexcept(noexcept_stop_requested_v<it_t>)
			requires concepts::stoppable<it_t>
		{
			return compressed.begin.stop_requested();
		}
		auto stop_reason() const noexcept(noexcept_stop_reason_v<it_t>) -> stop_reason_code
			requires concepts::stoppable<it_t>
		{
			return compressed.begin.stop_reason();
		}
		const char_type& operator*() const noexcept
		{
			return curr_value;
		}
		mb_code_unit_iterator& operator++ ()
		{
			curr_value = helper::to_next(compressed.begin, [this]() { return compressed.eof(); });
			return *this;
		}
		template <typename original_it_t>
		friend bool operator== (const mb_code_unit_iterator& lhs, const original_it_t& rhs) noexcept
		{
			return lhs.compressed.begin == rhs;
		}
		mb_code_unit_iterator(it_t bytes_begin)  requires eof_checker<it_t> :
			compressed{ ::std::move(bytes_begin) }
		{
			first_to_next();
		}
		mb_code_unit_iterator(it_t bytes_begin, auto&&) requires eof_checker<it_t> :
			mb_code_unit_iterator(::std::move(bytes_begin)) {}
		mb_code_unit_iterator(it_t bytes_begin, it_t bytes_end) requires (!eof_checker<it_t>) :
			compressed{ ::std::move(bytes_begin), ::std::move(bytes_end) }
		{
			first_to_next();
		}
	private:
		code_unit_iterator_compressed<it_t> compressed{};
		char_type curr_value{};
	};

	template <typename it_t>
	class mb_code_unit_iterator<unicode::encode_type::unknown, it_t>
	{
		static_assert(false_v<it_t>, "[pdn] unknown encode type for mb_code_unit_iterator");
	};

	template <typename it_t>
	class mb_code_unit_iterator<unicode::encode_type::utf8, it_t>
	{
		static_assert(false_v<it_t>, "[pdn] unsupported encode utf8 for mb_code_unit_iterator, see u8_code_unit_iterator");
	};

	namespace for_u8_code_unit_iterator
	{
		template <typename it_t>
		using deref_t = decltype(*::std::declval<it_t>());

		template <typename it_t>
		using it_value_t = ::std::remove_const_t<::std::remove_reference_t<deref_t<it_t>>>;

		template <typename it_t, typename t>
		concept can_ret_ref = ::std::same_as<it_value_t<it_t>, t>&& ::std::is_lvalue_reference_v<deref_t<it_t>>;
	}


	template <typename it_t>
	class u8_code_unit_iterator
	{
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = unicode::type_traits::code_unit_t<unicode::encode_type::utf8>;
		using char_type         = code_unit_type;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
		using deref_value_type  = ::std::conditional_t<for_u8_code_unit_iterator::can_ret_ref<it_t, char_type>, const char_type&, char_type>;
		static constexpr ::std::endian source_endian() noexcept
		{
			return ::std::endian::native;
		}
		bool stop_requested() const noexcept(noexcept_stop_requested_v<it_t>)
			requires concepts::stoppable<it_t>
		{
			return begin.stop_requested();
		}
		auto stop_reason() const noexcept(noexcept_stop_reason_v<it_t>) -> stop_reason_code
			requires concepts::stoppable<it_t>
		{
			return begin.stop_reason();
		}
		deref_value_type operator*() const noexcept
		{
			return *begin;
		}
		u8_code_unit_iterator& operator++ ()
		{
			++begin;
			return *this;
		}
		template <typename original_it_t>
		friend bool operator== (const u8_code_unit_iterator& lhs, const original_it_t& rhs) noexcept
		{
			return lhs.begin == rhs;
		}
		explicit u8_code_unit_iterator(it_t bytes_begin) : begin{ ::std::move(bytes_begin) } {}
		u8_code_unit_iterator(it_t bytes_begin, auto&&) : u8_code_unit_iterator(::std::move(bytes_begin)) {}
	private:
		it_t begin;
	};
}

namespace pdn
{
	template <unicode::encode_type encode_type, typename it_t>
	using code_unit_iterator = ::std::conditional_t<encode_type == unicode::encode_type::utf8,
		detail::u8_code_unit_iterator<it_t>,
		detail::mb_code_unit_iterator<encode_type, it_t>>;

	template <unicode::encode_type encode_type, typename it_t>
	inline auto make_code_unit_iterator(it_t bytestream_begin, it_t bytestream_end)
	{
		return code_unit_iterator<encode_type, it_t>{ ::std::move(bytestream_begin), ::std::move(bytestream_end) };
	}
}

#endif
