#ifndef PDN_Header_pdn_entity_as_accessor
#define PDN_Header_pdn_entity_as_accessor

#include <variant>

#include "pdn_types.h"
#include "pdn_entity_utility.h"
#include "pdn_entity_forward_decl.h"

namespace pdn
{
	template <typename char_t>
	inline auto as_int(const entity<char_t>& e) -> types::i64
	{
		return ::std::visit(dev_util::as_int, e);
	}

	template <typename char_t>
	inline auto as_uint(const entity<char_t>& e) -> types::u64
	{
		return ::std::visit(dev_util::as_uint, e);
	}

	template <typename char_t>
	inline auto as_fp(const entity<char_t>& e) -> types::f64
	{
		return ::std::visit(dev_util::as_fp, e);
	}

	template <typename char_t>
	inline auto as_bool(const entity<char_t>& e) -> types::boolean
	{
		return ::std::visit(dev_util::as_bool, e);
	}

	template <typename char_t>
	inline auto as_char(const entity<char_t>& e) -> types::character<char_t>
	{
		return ::std::visit(dev_util::as_accessor<char_t>::as_char, e);
	}

	template <typename char_t>
	inline auto as_string(const entity<char_t>& e) -> const types::string<char_t>&
	{
		return ::std::visit(dev_util::as_accessor<char_t>::as_string, e);
	}

	template <typename char_t>
	inline auto as_list(const entity<char_t>& e) -> const types::list<char_t>&
	{
		return ::std::visit(dev_util::as_accessor<char_t>::as_list, e);
	}

	template <typename char_t>
	inline auto as_object(const entity<char_t>& e) -> const types::object<char_t>&
	{
		return ::std::visit(dev_util::as_accessor<char_t>::as_object, e);
	}

	template <typename char_t>
	inline auto as_int(const_refer<char_t> e) -> types::i64
	{
		return e ? as_int(*e) : types::i64{};
	}

	template <typename char_t>
	inline auto as_uint(const_refer<char_t> e) -> types::u64
	{
		return e ? as_uint(*e) : types::u64{};
	}

	template <typename char_t>
	inline auto as_fp(const_refer<char_t> e) -> types::f64
	{
		return e ? as_fp(*e) : types::f64{};
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
