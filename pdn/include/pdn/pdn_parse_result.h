#ifndef PDN_Header_pdn_parse_result
#define PDN_Header_pdn_parse_result

#include <utility>
#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_entity.h"
#include "pdn_parser_utility.h"
#include "pdn_parse_error_code.h"

namespace pdn
{
	template <unicode::concepts::code_unit char_t>
	class parse_result
	{
	public:
		using entity_type = entity<char_t>;
		auto error_code() const noexcept -> parse_error_code
		{
			return err_c;
		}
		bool success() const noexcept
		{
			return err_c == parse_error_code{};
		}
		bool failed() const noexcept
		{
			return !success();
		}
		bool has_value() const noexcept
		{
			return success();
		}
		auto value() noexcept -> entity_type&
		{
			return ent;
		}
		auto value() const noexcept -> const entity_type&
		{
			return ent;
		}
		operator bool() const noexcept
		{
			return has_value();
		}
		auto operator->() noexcept -> entity_type*
		{
			return &ent;
		}
		auto operator->() const noexcept -> const entity_type*
		{
			return &ent;
		}
		auto operator*() & noexcept -> entity_type&
		{
			return ent;
		}
		auto operator*() && noexcept -> entity_type&&
		{
			return ::std::move(ent);
		}
		auto operator*() const& noexcept -> const entity_type&
		{
			return ent;
		}
		auto operator*() const&& noexcept -> const entity_type&&
		{
			return ::std::move(ent);
		}
		auto what() const noexcept -> const char*
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
			default:
				if (err_c == parse_error_code{})
				{
					return "success";
				}
				else
				{
					return "unknown_parse_error_code";
				}
			}
		}
		explicit parse_result(parse_error_code error_code) : err_c{ error_code }, ent{ default_ent() } {}
		explicit parse_result(entity_type&& ent_result) : err_c{}, ent{ ::std::move(ent_result) } {}
		parse_result(parse_error_code error_code, entity_type&& ent_result) :
			err_c{ error_code },
			ent{ ::std::move(ent_result) } {}
	private:
		static auto default_ent() -> entity_type
		{
			return parser_utility::default_entity_value<char_t>(type_code::unknown);
		}
		parse_error_code err_c;
		entity_type ent;
	};
}

#endif
