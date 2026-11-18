#ifndef PDN_Header_pdn_parse_result
#define PDN_Header_pdn_parse_result

#include <cassert>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_entity.h"
#include "pdn_parser_utility.h"
#include "pdn_parse_error_code.h"

namespace pdn
{
	// What every parse overload returns: the parsed entity together with the reason parsing stopped.
	// 'success' is the zero value of parse_error_code, which is not an enumerator (see
	// pdn_parse_error_code.h); success(), failed(), has_value() and operator bool only look at that
	// member. The entity is always constructed, so value(), operator* and operator-> never throw and
	// never dangle: they assert success() in debug builds, and with NDEBUG they expose the default
	// entity (see default_ent() below) instead of reading anything uninitialized.
	template <unicode::concepts::code_unit char_t>
	class parse_result
	{
	public:
		using entity_type = entity<char_t>;
		// The stop reason; the zero value means the parse succeeded.
		[[nodiscard]] auto error_code() const noexcept -> parse_error_code
		{
			return err_c;
		}
		[[nodiscard]] bool success() const noexcept
		{
			return err_c == parse_error_code{};
		}
		[[nodiscard]] bool failed() const noexcept
		{
			return !success();
		}
		[[nodiscard]] bool has_value() const noexcept
		{
			return success();
		}
		// The parsed entity. Reading it from a failed result is a precondition violation: debug builds
		// assert, release builds (NDEBUG) hand out the default entity described in default_ent().
		[[nodiscard]] auto value() noexcept -> entity_type&
		{
			assert(success());
			return ent;
		}
		[[nodiscard]] auto value() const noexcept -> const entity_type&
		{
			assert(success());
			return ent;
		}
		operator bool() const noexcept
		{
			return has_value();
		}
		[[nodiscard]] auto operator->() noexcept -> entity_type*
		{
			assert(success());
			return &ent;
		}
		[[nodiscard]] auto operator->() const noexcept -> const entity_type*
		{
			assert(success());
			return &ent;
		}
		[[nodiscard]] auto operator*() & noexcept -> entity_type&
		{
			assert(success());
			return ent;
		}
		[[nodiscard]] auto operator*() && noexcept -> entity_type&&
		{
			assert(success());
			return ::std::move(ent);
		}
		[[nodiscard]] auto operator*() const& noexcept -> const entity_type&
		{
			assert(success());
			return ent;
		}
		[[nodiscard]] auto operator*() const&& noexcept -> const entity_type&&
		{
			assert(success());
			return ::std::move(ent);
		}
		// The coarse name of the stop reason, always non-null, for logging and for std::exception-like
		// use. The detailed text (specific error plus position) is produced by the error message
		// generator, which needs the error_message reported through handle_error instead.
		[[nodiscard]] auto what() const noexcept -> const char*
		{
			using enum parse_error_code;
			switch (err_c)
			{
			case parse_error:             return "parse_error";
			case parse_error_limit:       return "parse_error_limit";
			case nesting_depth_exceed:    return "nesting_depth_exceed";
			case open_file_failed:        return "open_file_failed";
			case bad_stream:              return "bad_stream";
			case stream_operation_failed: return "stream_operation_failed";
			case incomplete_code_unit:    return "incomplete_code_unit";
			case user_cancelled:          return "user_cancelled";
			}
			if (err_c == parse_error_code{})
			{
				return "success";
			}
			return "unknown_parse_error_code"; // no enumerator is left: the value was not produced by this class
		}
		// A failed result: the stop reason plus the default entity.
		explicit parse_result(parse_error_code error_code) : err_c{ error_code }, ent{ default_ent() } {}
		// A successful result.
		explicit parse_result(entity_type&& ent_result) : err_c{}, ent{ ::std::move(ent_result) } {}
		// A failed result that still carries the entity parsed so far: pdn_parse.h uses it when the
		// token stream ended successfully but the function package reported errors while parsing.
		parse_result(parse_error_code error_code, entity_type&& ent_result) :
			err_c{ error_code },
			ent{ ::std::move(ent_result) } {}
	private:
		// The entity a failed result holds: the same default the parser uses when it recovers from an
		// error (type_code::unknown maps to an i32 holding zero), so value() is always a valid entity.
		static auto default_ent() -> entity_type
		{
			return parser_utility::default_entity_value<char_t>(type_code::unknown);
		}
		parse_error_code err_c;
		entity_type ent;
	};
}

#endif
