#ifndef PDN_Header_pdn_code_point_iterator
#define PDN_Header_pdn_code_point_iterator

#include <type_traits>
#include <utility>
#include <string>
#include <memory>
#include <functional>
#include <format>
#include <cstdint>

#include "pdn_unicode.h"
#include "pdn_code_convert.h"
#include "pdn_error_handler.h"
#include "pdn_error_message_generator.h"
#include "pdn_error_message_generator_en.h"
#include "pdn_error_message.h"

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

namespace pdn
{
	template <typename begin_it_t, typename end_it_t>
	class code_point_iterator
	{
	public:
		using iterator_category = void; // it does not satisfy the requirements of any legacy iterator
		using code_unit_type = ::std::remove_cvref_t<decltype(*(::std::declval<begin_it_t>()))>;
		using char_type = unicode::code_point_t;
		using size_type = ::std::size_t;

		using position_getter = ::std::function<source_position()>;
	public:
		unicode::code_point_t operator*() const noexcept
		{
			return curr_value;
		}
		code_point_iterator& operator++()
		{
		label_restart:
			if (begin == end)
			{
				is_end = true;
				return *this;
			}

			using decision = unicode::convert_decision<::std::basic_string_view<code_unit_type>, unicode::code_point_string>;

			auto result = decision::template decode<true>(begin, end);

			if (!result)
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
				err_handler({ pos_getter(), result.error(), err_msg_gen(result.error(), hex_em_s) });
				goto label_restart;
			}

			curr_value = result.value();

			return *this;
		}
		friend bool operator==(const code_point_iterator& lhs, const end_it_t& end) noexcept
		{
			return lhs.is_end;
		}
	public:
		code_point_iterator(begin_it_t begin_it,
		                    end_it_t end_it,
		                    position_getter pos_getter,
		                    error_handler err_handler,
		                    error_message_generator err_msg_generator = error_message_generator_en) :
			begin{ ::std::move(begin_it) },
			end{ ::std::move(end_it) },
			pos_getter{ ::std::move(pos_getter) },
			err_handler{ ::std::move(err_handler) },
			err_msg_gen{ ::std::move(err_msg_generator) }
		{
			++(*this);
		}
		code_point_iterator(const code_point_iterator&) = delete;
		code_point_iterator(code_point_iterator&& o)
		{
			*this = ::std::move(o);
		}
		code_point_iterator& operator=(const code_point_iterator&) = delete;
		code_point_iterator& operator=(code_point_iterator&& o)
		{
			::std::swap(begin, o.begin);
			::std::swap(end, o.end);
			::std::swap(pos_getter, o.pos_getter);
			::std::swap(err_handler, o.err_handler);
			::std::swap(err_msg_gen, o.err_msg_gen);
			::std::swap(curr_value, o.curr_value);
			::std::swap(is_end, o.is_end);
		}
	private:
		begin_it_t begin;
		end_it_t end;
		position_getter pos_getter;
		error_handler err_handler;
		error_message_generator err_msg_gen;
		unicode::code_point_t curr_value{};
		bool is_end{};
	};

	template <typename begin_it_t, typename end_it_t>
	inline auto make_code_point_iterator(begin_it_t begin_it,
	                                     end_it_t end_it,
	                                     typename code_point_iterator<begin_it_t, end_it_t>::position_getter pos_getter,
	                                     error_handler err_handler,
	                                     error_message_generator err_msg_generator = error_message_generator_en)
	{
		return code_point_iterator
		{
			::std::move(begin_it),
			::std::move(end_it),
			::std::move(pos_getter),
			::std::move(err_handler),
			::std::move(err_msg_generator)
		};
	}
}

#endif
