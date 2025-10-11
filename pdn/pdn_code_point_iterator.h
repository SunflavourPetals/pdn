#ifndef PDN_Header_pdn_code_point_iterator
#define PDN_Header_pdn_code_point_iterator

#include <type_traits>
#include <iterator>
#include <concepts>
#include <utility>
#include <string>
#include <memory>
#include <format>
#include <cstdint>

#include "pdn_unicode.h"
#include "pdn_convert_decision.h"
#include "pdn_raw_error_message.h"
#include "pdn_error_message.h"
#include "pdn_source_position_recorder_concept.h"
#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator_concept.h"

//    byte input stream (provide: get byte) // such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |         extra: requires byte input stream has operation seekg
//     v
//    swap chain (provide: get byte)
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
// >> code point iterator (provide: get code point, move backward(++it), check EOF)
//     |
//     v
//    lexer (provide: get token{ token code, value, token position })
//     |
//     v
//    parser (provide: parse) ----> pdn document object model

namespace pdn::concepts
{
	template <typename type>
	concept function_package_for_code_point_iterator
		 = concepts::source_position_recorder<type>
		&& concepts::error_handler<type>
		&& concepts::error_message_generator<type>;
}

namespace pdn::detail
{
	template <typename type>
	struct decode_result_to_raw_error {};
	template <>
	struct decode_result_to_raw_error<unicode::utf_8::decode_result>
	{
		using type = raw_error_message_type::utf_8_decode_error;
	};
	template <>
	struct decode_result_to_raw_error<unicode::utf_16::decode_result>
	{
		using type = raw_error_message_type::utf_16_decode_error;
	};
	template <>
	struct decode_result_to_raw_error<unicode::utf_32::decode_result>
	{
		using type = raw_error_message_type::utf_32_decode_error;
	};
	template <typename type>
	using decode_result_to_raw_error_t = decode_result_to_raw_error<type>::type;
}

namespace pdn
{
	template <typename begin_it_t, typename end_it_t, concepts::function_package_for_code_point_iterator function_package>
	class code_point_iterator
	{
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = ::std::iter_value_t<begin_it_t>;
		using char_type         = unicode::code_point_t;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
		const char_type& get() const noexcept
		{
			return curr_value;
		}
		bool eof() const
		{
			return begin == end;
		}
		void to_next()
		{
			func_pkg->update(get());
			to_next_impl();
		}
		const char_type& operator*() const noexcept
		{
			return get();
		}
		code_point_iterator& operator++()
		{
			to_next();
			return *this;
		}
		template <typename it_other_t>
		friend bool operator==(const code_point_iterator& lhs, const it_other_t& rhs) noexcept
		{
			return lhs.begin == rhs;
		}
	private:
		template <bool is_first = false>
		void to_next_impl()
		{
			if constexpr (!is_first)
			{
				if (begin != end)
				{
					++begin;
					++offset;
				}
			}
			while (begin != end)
			{
				using decision = unicode::convert_decision<::std::basic_string_view<code_unit_type>, unicode::ucpstring>;
				auto result = decision::template decode<false>(begin, end);
				offset += result.distance();
				if (result) [[likely]]
				{
					curr_value = result.value();
					break;
				}
				else
				{
					using result_type = decltype(result);
					const auto last = begin == end ? code_unit_type{} : *begin;
					func_pkg->handle_error(error_message{
						result.errc(),
						func_pkg->position(),
						func_pkg->generate_error_message(raw_error_message{
							result.errc(),
							func_pkg->position(),
							detail::decode_result_to_raw_error_t<result_type>{ result, last, offset }
						})
					});
					if (begin != end && !decision::decoder::template is_reaching_next<false>(result))
					{
						++begin;
						++offset;
					}
				}
			}
		}
	public:
		code_point_iterator(begin_it_t begin_it, end_it_t end_it, function_package& func_package) :
			func_pkg{ &func_package },
			begin   { ::std::move(begin_it) },
			end     { ::std::move(end_it) }
		{
			to_next_impl<true>();
		}
	private:
		function_package*     func_pkg{};
		size_type             offset{};
		begin_it_t            begin;
		end_it_t              end;
		unicode::code_point_t curr_value{};
	};

	template <typename begin_it_t, typename end_it_t, concepts::function_package_for_code_point_iterator function_package>
	inline auto make_code_point_iterator(begin_it_t begin_it, end_it_t end_it, function_package& func_package)
	{
		return code_point_iterator{ ::std::move(begin_it), ::std::move(end_it), func_package };
	}
}

#endif
