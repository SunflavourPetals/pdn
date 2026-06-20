#ifndef PDN_Header_pdn_entity_crtp_accessor
#define PDN_Header_pdn_entity_crtp_accessor

#include "pdn_types.h"
#include "pdn_entity.h"

namespace pdn::detail
{
	// impl in header pdn_entity_as_accessor.h and pdn_entity_get_accessor.h

	template <typename entity_t, typename char_t>
	struct crtp_accessor
	{
		using entity_type = entity_t;
		using char_type   = char_t;

		[[nodiscard]] auto as_int() const -> types::i64;

		template <types::concepts::pdn_sint in>
		[[nodiscard]] auto as_int(in) const -> in;

		[[nodiscard]] auto as_uint() const -> types::u64;

		template <types::concepts::pdn_uint un>
		[[nodiscard]] auto as_uint(un) const -> un;

		[[nodiscard]] auto as_fp() const -> types::f64;

		template <types::concepts::pdn_fp fn>
		[[nodiscard]] auto as_fp(fn) const -> fn;

		[[nodiscard]] auto as_bool() const -> types::boolean;

		[[nodiscard]] auto as_char() const -> types::character<char_type>;

		[[nodiscard]] auto as_string() const -> const types::string<char_type>&;

		[[nodiscard]] auto as_u8string() const -> unicode::u8string;

		[[nodiscard]] auto as_u16string() const -> unicode::u16string;

		[[nodiscard]] auto as_u32string() const -> unicode::u32string;

		[[nodiscard]] auto as_list() const -> const types::list<char_type>&;

		[[nodiscard]] auto as_object() const -> const types::object<char_type>&;

		// just for basic types
		template <typename target_t>
		[[nodiscard]] auto get_optional() const -> ::std::optional<target_t>;

		template <typename target_t>
		[[nodiscard]] bool type_test() const;
	};

	// impl in header pdn_entity_get_accessor.h
	// get is entity only

	template <typename entity_t, typename char_t>
	struct crtp_accessor_e : crtp_accessor<entity_t, char_t>
	{
		using entity_type = entity_t;
		using char_type   = char_t;

		template <typename target_t>
		[[nodiscard]] auto get() const& -> const target_t&;

		template <typename target_t>
		[[nodiscard]] auto get() && -> target_t&&;

		template <typename target_t>
		[[nodiscard]] auto get() & -> target_t&;

		template <typename target_t>
		[[nodiscard]] auto get_ptr() const& -> const target_t*;

		template <typename target_t>
		[[nodiscard]] auto get_ptr() & -> target_t*;

		template <typename target_t>
		auto get_ptr() && -> target_t* = delete;

		template <typename target_t>
		auto get_ptr() const&& -> const target_t* = delete;
	};
}

#endif
