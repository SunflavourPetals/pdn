#ifndef PDN_Header_pdn_token_value_variant
#define PDN_Header_pdn_token_value_variant

#include <utility>
#include <variant>

#include "pdn_unicode_base.h"
#include "pdn_type.h"
#include "pdn_proxy.h"

namespace pdn::detail
{
	// Payload of an at-identifier token (@name, e.g. @i32_max). The constant table these names are
	// looked up against is keyed by UTF-8, so the lexer always stores the name as UTF-8, converting
	// the source text when char_t is not u8char_t. The type is therefore independent of char_t,
	// unlike the string payload of token_value_variant<char_t>.
	class at_iden_string_proxy
	{
	public:
		using id_string = unicode::u8string;
		using id_string_view = unicode::u8string_view;
		auto get_id() const -> const id_string& { return *pr; }
		auto get_id() -> id_string& { return *pr; }
		at_iden_string_proxy() = default;
		at_iden_string_proxy(id_string_view id) : pr{ make_proxy<id_string>(id) } {}
		explicit at_iden_string_proxy(const id_string::value_type* id) : pr{ make_proxy<id_string>(id) } {}
		at_iden_string_proxy(id_string&& id) : pr{ make_proxy<id_string>(::std::move(id)) } {}
	private:
		proxy<id_string> pr{};
	};
}

namespace pdn
{
	// Value part of a token. monostate for tokens that carry no value (operators, separators, eof);
	// the exact PDN scalar type for a literal, because the lexer fixes the type while scanning, so
	// there is no "auto" alternative here; character<char_t> for a character literal; and a proxy to
	// an owned string for identifiers and string literals. The string sits behind a proxy so that
	// copying a token does not copy the text; whoever takes ownership moves the proxy out of the
	// variant, e.g. std::get<proxy<type::string<char_t>>>(std::move(tk.value)).
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
