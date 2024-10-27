#ifndef PDN_Header_pdn_code_point_iterator
#define PDN_Header_pdn_code_point_iterator

#include <type_traits>
#include <iterator>
#include <utility>
#include <string>
#include <memory>
#include <format>
#include <cstdint>

#include "pdn_unicode.h"
#include "pdn_code_convert.h"
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

namespace pdn::dev_util
{
	template <typename type>
	concept function_package_for_code_point_iterator
		 = concepts::source_position_getter<type>
		&& concepts::error_handler<type>
		&& concepts::error_message_generator<type>;
}

namespace pdn
{
	template <typename begin_it_t, typename end_it_t, dev_util::function_package_for_code_point_iterator function_package>
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
				}
			}
			while (begin != end)
			{
				using decision = unicode::convert_decision<::std::basic_string_view<code_unit_type>, unicode::code_point_string>;
				auto result = decision::template decode<false>(begin, end);
				if (result)
				{
					curr_value = result.value();
					break;
				}
				else
				{
					static_assert(sizeof(::std::uint32_t) >= sizeof(unicode::code_point_t));
					::std::string hex_s;
					if (begin != end)
					{
						hex_s = ::std::format("0x{:08X}", static_cast<::std::uint32_t>(*begin));
					}
					else
					{
						using namespace std::string_literals;
						hex_s = "EOF"s;
					}
					auto hex_em_s = reinterpret_to_err_msg_str(hex_s);
					func_pkg->handle_error(error_message{
						func_pkg->position(),
						result.error(),
						func_pkg->generate_error_message(result.error(), hex_em_s) });
					if (result.distance() == 0) ++begin;
				}
			}
		}
	public:
		code_point_iterator(begin_it_t begin_it, end_it_t end_it, function_package& func_package) :
			func_pkg{ &func_package },
			begin{ ::std::move(begin_it) },
			end{ ::std::move(end_it) }
		{
			to_next_impl<true>();
		}
	private:
		function_package*     func_pkg{};
		begin_it_t            begin;
		end_it_t              end;
		unicode::code_point_t curr_value{};
	};

	template <typename begin_it_t, typename end_it_t, dev_util::function_package_for_code_point_iterator function_package>
	inline auto make_code_point_iterator(begin_it_t begin_it, end_it_t end_it, function_package& func_package)
	{
		return code_point_iterator{ ::std::move(begin_it), ::std::move(end_it), func_package };
	}
}

#endif
