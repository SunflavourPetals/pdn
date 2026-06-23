#ifndef PDN_Header_pdn_token_value_variant
#define PDN_Header_pdn_token_value_variant

#include <type_traits>
#include <utility>
#include <variant>

#include "pdn_unicode_base.h"
#include "pdn_type.h"
#include "pdn_proxy.h"

namespace pdn::detail
{
	class at_iden_string_proxy
	{
	public:
		using id_string = unicode::u8string;
		using id_string_view = unicode::u8string_view;
		auto get_id() const -> const id_string& { return *pr; }
		auto get_id() -> id_string& { return *pr; };
		at_iden_string_proxy() = default;
		at_iden_string_proxy(id_string_view id) : pr{ make_proxy<id_string>(id) } {}
		at_iden_string_proxy(id_string&& id) : pr{ make_proxy<id_string>(::std::move(id)) } {}
	private:
		proxy<id_string> pr{};
	};
}

namespace pdn
{
	template <typename char_t>
	using token_value_variant = ::std::variant<
		::std::monostate,
		type::i8,  type::i16, type::i32, type::i64,
		type::u8,  type::u16, type::u32, type::u64,
		type::f32, type::f64,
		type::boolean,
		type::character<char_t>,
		proxy<type::string<char_t>>,
		detail::at_iden_string_proxy>;
}

#endif
