#include <pdn/pdn_parse_error_code.h>

#include <array>
#include <cassert>
#include <cstddef>

namespace
{
	using pdn::parse_error_code;
	using pdn::stop_reason_code;

	struct mapping_case
	{
		stop_reason_code reason;
		parse_error_code code;
	};

	// Every produced stop reason has a same named parse error code.
	constexpr ::std::array<mapping_case, 6> mappings{ {
		{ stop_reason_code::parse_error_limit,       parse_error_code::parse_error_limit       },
		{ stop_reason_code::nesting_depth_exceed,    parse_error_code::nesting_depth_exceed    },
		{ stop_reason_code::bad_stream,              parse_error_code::bad_stream              },
		{ stop_reason_code::stream_operation_failed, parse_error_code::stream_operation_failed },
		{ stop_reason_code::incomplete_code_unit,    parse_error_code::incomplete_code_unit    },
		{ stop_reason_code::user_cancelled,          parse_error_code::user_cancelled          },
	} };
}

// 1. The numbering is part of the contract: 0 means success, and parse_result::operator bool relies on it.
static_assert(static_cast<int>(parse_error_code::parse_error) == 1);
static_assert(static_cast<int>(parse_error_code::parse_error_limit) == 2);
static_assert(static_cast<int>(parse_error_code::nesting_depth_exceed) == 3);
static_assert(static_cast<int>(parse_error_code::open_file_failed) == 4);
static_assert(static_cast<int>(parse_error_code::bad_stream) == 5);
static_assert(static_cast<int>(parse_error_code::stream_operation_failed) == 6);
static_assert(static_cast<int>(parse_error_code::incomplete_code_unit) == 7);
static_assert(static_cast<int>(parse_error_code::user_cancelled) == 8);
static_assert(static_cast<int>(parse_error_code{}) == 0);

// 2. Each stop reason maps to the code of the same name, and the mapping is constexpr and noexcept.
static_assert(pdn::to_parse_error_code(stop_reason_code::parse_error_limit) == parse_error_code::parse_error_limit);
static_assert(pdn::to_parse_error_code(stop_reason_code::nesting_depth_exceed) == parse_error_code::nesting_depth_exceed);
static_assert(pdn::to_parse_error_code(stop_reason_code::bad_stream) == parse_error_code::bad_stream);
static_assert(pdn::to_parse_error_code(stop_reason_code::stream_operation_failed) == parse_error_code::stream_operation_failed);
static_assert(pdn::to_parse_error_code(stop_reason_code::incomplete_code_unit) == parse_error_code::incomplete_code_unit);
static_assert(pdn::to_parse_error_code(stop_reason_code::user_cancelled) == parse_error_code::user_cancelled);
static_assert(noexcept(pdn::to_parse_error_code(stop_reason_code::bad_stream)));

// 3. The two codes that are not stop reasons are never produced by the mapping:
//    parse_error means "recoverable errors were reported", open_file_failed comes from the file overloads.
static_assert(pdn::to_parse_error_code(stop_reason_code::parse_error_limit) != parse_error_code::parse_error);
static_assert(pdn::to_parse_error_code(stop_reason_code::parse_error_limit) != parse_error_code::open_file_failed);

int main()
{
	// 0 is the success value and is not one of the enumerators, so 'error_code() == parse_error_code{}' is a
	// reliable success test even for a default constructed result.
	assert(parse_error_code{} == static_cast<parse_error_code>(0));
	for (const auto& m : mappings)
	{
		assert(pdn::to_parse_error_code(m.reason) == m.code);
		assert(static_cast<int>(pdn::to_parse_error_code(m.reason)) != 0);
	}

	// The six mappings are pairwise distinct, so no two stop reasons collapse into one error code.
	for (::std::size_t i = 0; i < mappings.size(); ++i)
	{
		for (::std::size_t j = i + 1; j < mappings.size(); ++j)
		{
			assert(mappings[i].code != mappings[j].code);
		}
	}
}