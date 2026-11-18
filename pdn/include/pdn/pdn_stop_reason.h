#ifndef PDN_Header_pdn_stop_reason
#define PDN_Header_pdn_stop_reason

namespace pdn
{
	// Reason a layer asked to stop parsing. The unnamed value 0 means "no stop requested", which is
	// why the first enumerator is 1: every request_stop(...) only records the reason while the stored
	// one is still 0, so the first reason wins. Every code below except user_cancelled is produced by
	// this library; user_cancelled exists for user defined function packages.
	enum class stop_reason_code
	{
		parse_error_limit = 1,
		nesting_depth_exceed,
		bad_stream,
		stream_operation_failed,
		incomplete_code_unit,
		user_cancelled, // for user defined function package
	};
}

#endif
