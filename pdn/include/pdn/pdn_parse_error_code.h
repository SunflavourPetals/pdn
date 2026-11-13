#ifndef PDN_Header_pdn_parse_error_code
#define PDN_Header_pdn_parse_error_code

#include <cassert>
#include <utility>

#include "pdn_stop_reason.h"

namespace pdn
{
	enum class parse_error_code
	{
		parse_error = 1,
		parse_error_limit, // stop requested in parse because error limit exceed
		nesting_depth_exceed,
		open_file_failed, // for input file stream or filename | path
		bad_stream,
		stream_operation_failed,
		incomplete_code_unit,
		user_cancelled, // for user defined function package
	};

	// 0 is the 'no error' value: parse_result reports it on success and it is the reason the enumerators
	// start at 1. parse_error (1) is the generic failure - recoverable lexical/syntax errors were reported but
	// nothing requested a stop; open_file_failed (4) is only produced by the stream/file overloads of parse.
	// The remaining six values mirror stop_reason_code one to one, and every one of them is listed below:
	// the switch has no default, so a stop reason added without a mapping here is reported by -Wswitch
	// instead of silently turning into another value.
	constexpr auto to_parse_error_code(stop_reason_code c) noexcept -> parse_error_code
	{
		using enum parse_error_code;
		switch (c)
		{
		case stop_reason_code::parse_error_limit:       return parse_error_limit;
		case stop_reason_code::nesting_depth_exceed:    return nesting_depth_exceed;
		case stop_reason_code::bad_stream:              return bad_stream;
		case stop_reason_code::stream_operation_failed: return stream_operation_failed;
		case stop_reason_code::incomplete_code_unit:    return incomplete_code_unit;
		case stop_reason_code::user_cancelled:          return user_cancelled;
		}
#ifdef __cpp_lib_unreachable
		::std::unreachable();
#else
		assert(false && "[pdn] unknown stop reason");
#endif
		return parse_error; // unreachable: every stop_reason_code has a case above
	}
}

#endif
