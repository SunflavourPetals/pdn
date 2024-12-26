#ifndef PDN_Header_pdn_entity_as_accessor
#define PDN_Header_pdn_entity_as_accessor

#include <variant>

#include "pdn_types.h"
#include "pdn_entity_utility.h"
#include "pdn_entity_forward_decl.h"

namespace pdn
{
	inline constexpr auto auto_int_tag = types::auto_int{};
	inline constexpr auto i8_tag  = types::i8{};
	inline constexpr auto i16_tag = types::i16{};
	inline constexpr auto i32_tag = types::i32{};
	inline constexpr auto i64_tag = types::i64{};
	inline constexpr auto auto_uint_tag = types::auto_uint{};
	inline constexpr auto u8_tag  = types::u8{};
	inline constexpr auto u16_tag = types::u16{};
	inline constexpr auto u32_tag = types::u32{};
	inline constexpr auto u64_tag = types::u64{};
	inline constexpr auto f32_tag = types::f32{};
	inline constexpr auto f64_tag = types::f64{};

	template <typename char_t>
	inline auto as_int(const entity<char_t>& e) -> types::i64
	{
		return ::std::visit([](const auto& v) { return dev_util::as_int(v); }, e);
	}

	template <typename char_t, typename in>
	inline auto as_int(const entity<char_t>& e, in) -> in
	{
		return ::std::visit([]<typename arg_t>(const arg_t& v) { return dev_util::as_int<arg_t, in>(v); }, e);
	}

	template <typename char_t>
	inline auto as_uint(const entity<char_t>& e) -> types::u64
	{
		return ::std::visit([](const auto& v) { return dev_util::as_uint(v); }, e);
	}

	template <typename char_t, typename un>
	inline auto as_uint(const entity<char_t>& e, un) -> un
	{
		return ::std::visit([]<typename arg_t>(const arg_t& v) { return dev_util::as_uint<arg_t, un>(v); }, e);
	}

	template <typename char_t>
	inline auto as_fp(const entity<char_t>& e) -> types::f64
	{
		return ::std::visit([](const auto& v) { return dev_util::as_fp(v); }, e);
	}

	template <typename char_t, typename fn>
	inline auto as_fp(const entity<char_t>& e, fn) -> fn
	{
		return ::std::visit([]<typename arg_t>(const arg_t& v) { return dev_util::as_fp<arg_t, fn>(v); }, e);
	}

	template <typename char_t>
	inline auto as_bool(const entity<char_t>& e) -> types::boolean
	{
		return ::std::visit([](const auto& v) { return dev_util::as_bool(v); }, e);
	}

	template <typename char_t>
	inline auto as_char(const entity<char_t>& e) -> types::character<char_t>
	{
		return ::std::visit([](const auto& v) { return dev_util::as_accessor<char_t>::as_char(v); }, e);
	}

	template <typename char_t>
	inline auto as_string(const entity<char_t>& e) -> const types::string<char_t>&
	{
		return ::std::visit([](const auto& v) -> decltype(auto) { return dev_util::as_accessor<char_t>::as_string(v); }, e);
	}

	template <typename char_t>
	inline auto as_list(const entity<char_t>& e) -> const types::list<char_t>&
	{
		return ::std::visit([](const auto& v) -> decltype(auto) { return dev_util::as_accessor<char_t>::as_list(v); }, e);
	}

	template <typename char_t>
	inline auto as_object(const entity<char_t>& e) -> const types::object<char_t>&
	{
		return ::std::visit([](const auto& v) -> decltype(auto) { return dev_util::as_accessor<char_t>::as_object(v); }, e);
	}

	template <typename char_t>
	inline auto as_int(const_refer<char_t> e) -> types::i64
	{
		return e ? as_int(*e) : types::i64{};
	}

	template <typename char_t, typename in>
	inline auto as_int(const const_refer<char_t>& e, in tag) -> in
	{
		return e ? as_int(*e, tag) : in{};
	}

	template <typename char_t>
	inline auto as_uint(const_refer<char_t> e) -> types::u64
	{
		return e ? as_uint(*e) : types::u64{};
	}

	template <typename char_t, typename un>
	inline auto as_uint(const const_refer<char_t>& e, un tag) -> un
	{
		return e ? as_uint(*e, tag) : un{};
	}

	template <typename char_t>
	inline auto as_fp(const_refer<char_t> e) -> types::f64
	{
		return e ? as_fp(*e) : types::f64{};
	}

	template <typename char_t, typename fn>
	inline auto as_fp(const const_refer<char_t>& e, fn tag) -> fn
	{
		return e ? as_fp(*e, tag) : fn{};
	}

	template <typename char_t>
	inline auto as_bool(const_refer<char_t> e) -> types::boolean
	{
		return e ? as_bool(*e) : types::boolean{};
	}

	template <typename char_t>
	inline auto as_char(const_refer<char_t> e) -> types::character<char_t>
	{
		return e ? as_char(*e) : types::character<char_t>{};
	}

	template <typename char_t>
	inline auto as_string(const_refer<char_t> e) -> const types::string<char_t>&
	{
		return e ? as_string(*e) : dev_util::as_accessor<char_t>::null_string_val();
	}

	template <typename char_t>
	inline auto as_list(const_refer<char_t> e) -> const types::list<char_t>&
	{
		return e ? as_list(*e) : dev_util::as_accessor<char_t>::null_list_val();
	}

	template <typename char_t>
	inline auto as_object(const_refer<char_t> e) -> const types::object<char_t>&
	{
		return e ? as_object(*e) : dev_util::as_accessor<char_t>::null_object_val();
	}
}

#endif
