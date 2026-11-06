#ifndef PDN_Header_pdn_stop_reason
#define PDN_Header_pdn_stop_reason

namespace pdn
{
	enum class stop_reason_code
	{
		parse_error_limit = 1,
		open_file_failed, // for input file stream
		bad_stream,
		stream_operation_failed,
		incomplete_code_unit,
		user_cancelled, // for user defined function package
	};
}

#endif
