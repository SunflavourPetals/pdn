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
#include "pdn_eof_detector.h"
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
	// requires it_t::operator* and eof never request stop
	// only for mb_code_unit_iterator inner usage
	template <unicode::encode_type encode_type, typename it_t>
	class mb_code_unit_iterator_helper
	{
	public:
		static_assert(encode_type != unicode::encode_type::unknown && encode_type != unicode::encode_type::utf8,
			"[pdn] code_unit_iterator_helper: unsupported encode");
		using code_unit_type = unicode::type_traits::code_unit_t<encode_type>;
		using char_type = code_unit_type;
		using size_type = ::std::size_t;
		using it_value_t = ::std::remove_cvref_t<decltype(*::std::declval<it_t&>())>;
		using unsigned_it_value_t = ::std::make_unsigned_t<it_value_t>;
		static constexpr size_type bits_count_of_byte{ 8 }; // requires 1 Byte = 8 bits
		static constexpr size_type bits_count_of_unit{ sizeof(char_type) * bits_count_of_byte };
		bool stop_requested() const noexcept
		{
			return stop_reason_c != stop_reason_code{};
		}
		auto stop_reason() const noexcept -> stop_reason_code
		{
			return stop_reason_c;
		}
		static constexpr ::std::endian source_endian() noexcept
		{
			return unicode::type_traits::endian_from_encode_type<encode_type>;
		}
		template <typename pred>
		char_type to_next(it_t& begin, pred&& is_eof)
		{
			if constexpr (source_endian() == ::std::endian::little)
			{
				return le_next(begin, ::std::forward<pred>(is_eof));
			}
			else
			{
				return be_next(begin, ::std::forward<pred>(is_eof));
			}
		}
		template <typename pred>
		char_type first_to_next(it_t& begin, pred&& is_eof)
		{
			if constexpr (source_endian() == ::std::endian::little)
			{
				return first_le_next(begin, ::std::forward<pred>(is_eof));
			}
			else
			{
				return first_be_next(begin, ::std::forward<pred>(is_eof));
			}
		}
	private:
		static char_type left_move(it_value_t value, size_type offset) noexcept
		{
			using uct = ::std::make_unsigned_t<char_type>;
			return static_cast<uct>(static_cast<unsigned_it_value_t>(value)) << offset;
		}
		template <typename pred> requires (source_endian() == ::std::endian::little)
		char_type le_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (auto offset = size_type{}; offset != bits_count_of_unit; offset += bits_count_of_byte)
			{
				// 0x12345678 le -> 0:0x78, 1:0x56, 2:0x34, 3:0x12
				++begin;
				if constexpr (concepts::stoppable<it_t>)
				{
					if (begin.stop_requested()) [[unlikely]]
					{
						request_stop(begin.stop_reason());
						break;
					}
				}
				if (is_eof()) [[unlikely]]
				{
					if (offset) [[unlikely]]
					{
						request_stop(stop_reason_code::incomplete_code_unit);
					}
					break;
				}
				curr_value |= left_move(*begin, offset);
			}
			return curr_value;
		}
		template <typename pred> requires (source_endian() == ::std::endian::big)
		char_type be_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			for (auto offset = bits_count_of_unit; offset != 0; )
			{
				// 0x12345678 be -> 0:0x12, 1:0x34, 2:0x56, 3:0x78
				++begin;
				if constexpr (concepts::stoppable<it_t>)
				{
					if (begin.stop_requested()) [[unlikely]]
					{
						request_stop(begin.stop_reason());
						break;
					}
				}
				if (is_eof()) [[unlikely]]
				{
					if (offset != bits_count_of_unit) [[unlikely]]
					{
						request_stop(stop_reason_code::incomplete_code_unit);
					}
					break;
				}
				offset -= bits_count_of_byte;
				curr_value |= left_move(*begin, offset);
			}
			return curr_value;
		}
		template <typename pred> requires (source_endian() == ::std::endian::little)
		char_type first_le_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			if constexpr (concepts::stoppable<it_t>)
			{
				if (begin.stop_requested()) [[unlikely]]
				{
					request_stop(begin.stop_reason());
					return curr_value;
				}
			}
			for (auto offset = size_type{}; ; )
			{
				if (is_eof()) [[unlikely]]
				{
					if (offset) [[unlikely]]
					{
						request_stop(stop_reason_code::incomplete_code_unit);
					}
					break;
				}
				curr_value |= left_move(*begin, offset);
				offset += bits_count_of_byte;
				if (offset == bits_count_of_unit)
				{
					break;
				}
				++begin;
				if constexpr (concepts::stoppable<it_t>)
				{
					if (begin.stop_requested()) [[unlikely]]
					{
						request_stop(begin.stop_reason());
						break;
					}
				}
			}
			return curr_value;
		}
		template <typename pred> requires (source_endian() == ::std::endian::big)
		char_type first_be_next(it_t& begin, pred&& is_eof)
		{
			auto curr_value = char_type{};
			if constexpr (concepts::stoppable<it_t>)
			{
				if (begin.stop_requested()) [[unlikely]]
				{
					request_stop(begin.stop_reason());
					return curr_value;
				}
			}
			for (auto offset = bits_count_of_unit; ; )
			{
				if (is_eof()) [[unlikely]]
				{
					if (offset != bits_count_of_unit) [[unlikely]]
					{
						request_stop(stop_reason_code::incomplete_code_unit);
					}
					break;
				}
				offset -= bits_count_of_byte;
				curr_value |= left_move(*begin, offset);
				if (offset == 0)
				{
					break;
				}
				++begin;
				if constexpr (concepts::stoppable<it_t>)
				{
					if (begin.stop_requested()) [[unlikely]]
					{
						request_stop(begin.stop_reason());
						break;
					}
				}
			}
			return curr_value;
		}
		void request_stop(stop_reason_code reason) noexcept
		{
			if (stop_reason_c == stop_reason_code{})
			{
				stop_reason_c = reason;
			}
		}
		stop_reason_code stop_reason_c{};
	};

	template <typename it_begin_t, typename it_end_t>
	struct code_unit_iterator_compressed final
	{
		it_begin_t begin;
		it_end_t end;
		bool eof() const noexcept(noexcept(::std::declval<const it_begin_t&>() == ::std::declval<const it_end_t&>()))
		{
			return begin == end;
		}
	};

	template <concepts::eof_detector it_begin_t, typename it_end_t>
	struct code_unit_iterator_compressed<it_begin_t, it_end_t> final
	{
		it_begin_t begin;
		bool eof() const noexcept(noexcept(::std::declval<const it_begin_t&>().eof()))
		{
			return begin.eof();
		}
	};

	// for multi-byte code unit type(utf16 or utf32)
	template <unicode::encode_type encode_type, typename it_begin_t, typename it_end_t>
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
		using helper_type       = mb_code_unit_iterator_helper<encode_type, it_begin_t>;
		void first_to_next()
		{
			curr_value = helper.first_to_next(compressed.begin, [this]() { return compressed.eof(); });
		}
	public:
		static constexpr ::std::endian source_endian() noexcept
		{
			return helper_type::source_endian();
		}
		bool stop_requested() const noexcept
		{
			return helper.stop_requested();
		}
		auto stop_reason() const noexcept
		{
			return helper.stop_reason();
		}
		bool eof() const noexcept(noexcept(::std::declval<const code_unit_iterator_compressed<it_begin_t, it_end_t>&>().eof()))
		{
			return compressed.eof();
		}
		const char_type& operator*() const noexcept
		{
			return curr_value;
		}
		mb_code_unit_iterator& operator++ ()
		{
			curr_value = helper.to_next(compressed.begin, [this]() { return compressed.eof(); });
			return *this;
		}
		// just for checking eof, returns is lhs eof
		template <typename original_it_t>
		friend bool operator== (const mb_code_unit_iterator& lhs, const original_it_t& rhs)
			noexcept(noexcept(::std::declval<const code_unit_iterator_compressed<it_begin_t, it_end_t>&>().eof()))
		{
			return lhs.compressed.eof();
		}
		mb_code_unit_iterator(it_begin_t bytes_begin)  requires concepts::eof_detector<it_begin_t> :
			compressed{ ::std::move(bytes_begin) }
		{
			first_to_next();
		}
		mb_code_unit_iterator(it_begin_t bytes_begin, auto&&) requires concepts::eof_detector<it_begin_t> :
			mb_code_unit_iterator(::std::move(bytes_begin)) {}
		mb_code_unit_iterator(it_begin_t bytes_begin, it_end_t bytes_end) requires (!concepts::eof_detector<it_begin_t>) :
			compressed{ ::std::move(bytes_begin), ::std::move(bytes_end) }
		{
			first_to_next();
		}
	private:
		code_unit_iterator_compressed<it_begin_t, it_end_t> compressed{};
		char_type curr_value{};
		helper_type helper{};
	};

	template <typename it_begin_t, typename it_end_t>
	class mb_code_unit_iterator<unicode::encode_type::unknown, it_begin_t, it_end_t>
	{
		static_assert(false_v<it_begin_t>, "[pdn] unknown encode type for mb_code_unit_iterator");
	};

	template <typename it_begin_t, typename it_end_t>
	class mb_code_unit_iterator<unicode::encode_type::utf8, it_begin_t, it_end_t>
	{
		static_assert(false_v<it_begin_t>, "[pdn] unsupported encode utf8 for mb_code_unit_iterator, see u8_code_unit_iterator");
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
		bool eof() const noexcept(noexcept(::std::declval<const it_t&>().eof()))
			requires concepts::eof_detector<it_t>
		{
			return begin.eof();
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
		// just for checking eof
		template <typename original_it_t>
		friend bool operator== (const u8_code_unit_iterator& lhs, const original_it_t& rhs)
			noexcept(noexcept(::std::declval<const it_t&>() == ::std::declval<const original_it_t&>()))
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
	template <unicode::encode_type encode_type, typename it_begin_t, typename it_end_t>
	using code_unit_iterator = ::std::conditional_t<encode_type == unicode::encode_type::utf8,
		detail::u8_code_unit_iterator<it_begin_t>,
		detail::mb_code_unit_iterator<encode_type, it_begin_t, it_end_t>>;

	template <unicode::encode_type encode_type, typename it_begin_t, typename it_end_t>
	 auto make_code_unit_iterator(it_begin_t bytestream_begin, it_end_t bytestream_end)
	{
		return code_unit_iterator<encode_type, it_begin_t, it_end_t>{ ::std::move(bytestream_begin), ::std::move(bytestream_end) };
	}
}

#endif
