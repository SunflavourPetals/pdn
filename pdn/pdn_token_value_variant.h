#ifndef PDN_Header_pdn_token_value_variant
#define PDN_Header_pdn_token_value_variant

#include <type_traits>
#include <utility>
#include <variant>

#include "pdn_unicode_base.h"
#include "pdn_types.h"
#include "pdn_proxy.h"

namespace pdn::dev_util
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
		types::i8,  types::i16, types::i32, types::i64,
		types::u8,  types::u16, types::u32, types::u64,
		types::f32, types::f64,
		types::boolean,
		types::character<char_t>,
		proxy<types::string<char_t>>,
		dev_util::at_iden_string_proxy>;
}

#endif
