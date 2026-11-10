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
		auto error_code() const noexcept
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
		auto value() noexcept -> entity<char_t>&
		{
			return ent;
		}
		auto value() const noexcept -> const entity<char_t>&
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
