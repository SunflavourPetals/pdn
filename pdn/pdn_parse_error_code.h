#ifndef PDN_Header_pdn_parse_error_code
#define PDN_Header_pdn_parse_error_code

#ifdef __cpp_lib_unreachable
#include <utility>
#else
#include <cassert>
#endif

#include "pdn_error_string.h"
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

	constexpr auto to_error_message_string(parse_error_code err_c) -> error_msg_string_view
	{
		using enum parse_error_code;
		switch (err_c)
		{
		case parse_error:             return u8"parse_error"_emv;
		case parse_error_limit:       return u8"parse_error_limit"_emv;
		case nesting_depth_exceed:    return u8"nesting_depth_exceed"_emv;
		case open_file_failed:        return u8"open_file_failed"_emv;
		case bad_stream:              return u8"bad_stream"_emv;
		case stream_operation_failed: return u8"stream_operation_failed"_emv;
		case incomplete_code_unit:    return u8"incomplete_code_unit"_emv;
		case user_cancelled:          return u8"user_cancelled"_emv;
		default:                      return u8"unknown_parse_error_code"_emv;
		}
	}

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
		default:
#ifdef __cpp_lib_unreachable
			::std::unreachable();
#else
			assert(false && "[pdn] unknown stop reason");
#endif
			return user_cancelled; // unreachable
		}
	}
}

#endif
