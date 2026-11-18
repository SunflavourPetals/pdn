#ifndef PDN_Header_pdn_code_point_iterator
#define PDN_Header_pdn_code_point_iterator

#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_utf_decoder.h"
#include "pdn_convert_decision.h"
#include "pdn_raw_error_message.h"
#include "pdn_error_message.h"
#include "pdn_eof_detector_concept.h"
#include "pdn_source_position_recorder_concept.h"
#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_stop_reason.h"
#include "pdn_stoppable_concept.h"

//    byte input stream (provide: read byte...) such as ifstream
//     |
//     +---> BOM reader (provide: get BOM)
//     |
//     v
//    byte iterator (provide: get byte, advance(++it), check EOF)
//     |
//     v
//    code unit iterator (provide: get code unit, advance(++it), check EOF)
//     |
//     +---- utf-8 decoder ---+
//     |                      |
//     +---- utf-16 decoder --+---- (provide: decode to get unicode code-point)
//     |                      |
//     +---- utf-32 decoder --+
//     v
// >> code point iterator (provide: get code point, advance(++it), check EOF)
//     |
//     v
//    lexer (provide: get token{ token code, value, token position })
//     |
//     v
//    parser (provide: parse) --> pdn document object model

// This stage turns code units into Unicode scalar values for the lexer. When the decoder rejects a
// sequence, the error is reported to the function package and the iterator either resynchronises at
// the first code unit that can start a sequence or skips the code units the decoder already read, so
// the lexer only ever sees scalar values. The position recorder advances by one replacement
// character per rejected sequence, which keeps line/column counting in step with the code points the
// lexer sees. `offset` counts consumed code units and locates the rejected sequence.
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
	struct decode_result_to_raw_error<unicode::utf8::decode_result>
	{
		using type = raw_error_message_type::utf8_decode_error;
	};
	template <>
	struct decode_result_to_raw_error<unicode::utf16::decode_result>
	{
		using type = raw_error_message_type::utf16_decode_error;
	};
	template <>
	struct decode_result_to_raw_error<unicode::utf32::decode_result>
	{
		using type = raw_error_message_type::utf32_decode_error;
	};
	template <typename type>
	using decode_result_to_raw_error_t = decode_result_to_raw_error<type>::type;
}

namespace pdn
{
	// Input iterator over code points. It deliberately does not model a standard iterator:
	// iterator_concept and iterator_category are void because copying it shares the underlying stream
	// (positions are not independent) and ranges algorithms assume that they are.
	template <typename begin_it_t, typename end_it_t, concepts::function_package_for_code_point_iterator function_package>
	class code_point_iterator
	{
		// iteration can be stopped by the code unit iterator or by the function package
		static constexpr bool stoppable = concepts::stoppable<begin_it_t> || concepts::stoppable<function_package>;
	public:
		using iterator_concept  = void;
		using iterator_category = void;
		using code_unit_type    = ::std::iter_value_t<begin_it_t>;
		using char_type         = unicode::code_point_t;
		using size_type         = ::std::size_t;
		using value_type        = char_type;
		bool stop_requested() const noexcept requires (stoppable)
		{
			return stop_reason_c != stop_reason_code{};
		}
		auto stop_reason() const noexcept -> stop_reason_code requires (stoppable)
		{
			return stop_reason_c;
		}
		// the code point the iterator currently points at; after eof() it is the last value decoded
		const char_type& get() const noexcept
		{
			return curr_value;
		}
		// the code unit iterator has no eof(): compare it against the end iterator
		bool eof() const noexcept(noexcept(::std::declval<const begin_it_t&>() == ::std::declval<const end_it_t&>()))
			requires (!concepts::eof_detector<begin_it_t>)
		{
			return begin == end;
		}
		// the code unit iterator can answer eof() itself (for example when it reads from a stream)
		bool eof() const noexcept(noexcept(::std::declval<const begin_it_t&>().eof()))
			requires concepts::eof_detector<begin_it_t>
		{
			return begin.eof();
		}
		// leave the current code point: report it to the position recorder, then decode the next one
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
		// compares the underlying code unit position against the end iterator (or any comparable value)
		template <typename it_other_t>
		friend bool operator==(const code_point_iterator& lhs, const it_other_t& rhs) noexcept
		{
			return lhs.begin == rhs;
		}
		// decodes the first code point without advancing: it keeps the position the caller starts from
		code_point_iterator(begin_it_t begin_it, end_it_t end_it, function_package& func_package) :
			func_pkg{ &func_package },
			begin{ ::std::move(begin_it) },
			end{ ::std::move(end_it) }
		{
			to_next_impl<true>();
		}
	private:
		template <bool is_first = false>
		void to_next_impl()
		{
			// the decoder stopped on the last code unit of the previous code point; step over it
			if constexpr (!is_first) if (!eof()) [[unlikely]]
			{
				++begin;
				if constexpr (concepts::stoppable<begin_it_t>) if (begin.stop_requested()) [[unlikely]]
				{
					request_stop(begin.stop_reason());
					return;
				}
				++offset;
			}
			using decision = unicode::convert_decision<::std::basic_string_view<code_unit_type>, unicode::ucpstring>;
			while (!eof())
			{
				// decode<false> stops on the last code unit of a sequence, so ++begin reaches the next
				// one; a rejected sequence is reported and skipped here, and the loop continues until a
				// code point is available, eof() is reached or a stop is requested.
				auto result = decision::template decode<false>(begin, end);
				// distance counts the code units the decoder consumed; the last one is left on begin
				offset += result.distance();
				if (result) [[likely]]
				{
					curr_value = result.value();
					break;
				}
				else [[unlikely]]
				{
					if constexpr (concepts::stoppable<begin_it_t>) if (begin.stop_requested()) [[unlikely]]
					{
						request_stop(begin.stop_reason());
						return;
					}
					using result_type = decltype(result);
					// the unit reported with the error: the one the decoder stopped on, or 0 at eof
					const auto last = eof() ? code_unit_type{} : *begin;
					func_pkg->handle_error(error_message{
						result.errc(),
						func_pkg->position(),
						func_pkg->generate_error_message(raw_error_message{
							result.errc(),
							func_pkg->position(),
							detail::decode_result_to_raw_error_t<result_type>{ result, last, offset }
						})
					});
					if constexpr (concepts::stoppable<function_package>) if (func_pkg->stop_requested()) [[unlikely]]
					{
						request_stop(func_pkg->stop_reason());
						return;
					}
					// one replacement character per rejected sequence keeps line/column in step with the lexer view
					func_pkg->update(U'\uFFFD');
					// false: the decoder already stopped where the next sequence starts, so skip one code unit
					if (!eof() && !decision::decoder_type::template is_reaching_next<false>(result))
					{
						++begin;
						if constexpr (concepts::stoppable<begin_it_t>) if (begin.stop_requested()) [[unlikely]]
						{
							request_stop(begin.stop_reason());
							return;
						}
						++offset;
					}
				}
			}
		}
		// keep the first reason: later ones are consequences of an earlier stop
		void request_stop(stop_reason_code reason) noexcept requires (stoppable)
		{
			if (stop_reason_c == stop_reason_code{})
			{
				stop_reason_c = reason;
			}
		}
		// the package that receives positions and errors; it outlives the iterator
		function_package*     func_pkg{};
		// code units consumed so far: the offset of the last consumed unit
		size_type             offset{};
		begin_it_t            begin;
		end_it_t              end;
		unicode::code_point_t curr_value{};
		stop_reason_code      stop_reason_c{}; // available only when stoppable<begin_it_t> or stoppable<function_package>
	};

	template <typename begin_it_t, typename end_it_t, concepts::function_package_for_code_point_iterator function_package>
	auto make_code_point_iterator(begin_it_t begin_it, end_it_t end_it, function_package& func_package)
	{
		return code_point_iterator{ ::std::move(begin_it), ::std::move(end_it), func_package };
	}
}

#endif
