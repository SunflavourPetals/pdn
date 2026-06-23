#ifndef PDN_Header_pdn_entity
#define PDN_Header_pdn_entity

#include <type_traits>
#include <utility>
#include <variant>
#include <cstddef>
#include <string>
#include <optional>

#include "pdn_type.h"
#include "pdn_proxy.h"
#include "pdn_utf_code_convert.h"
#include "pdn_entity_utility.h"
#include "pdn_entity_forward_decl.h"

namespace pdn::detail
{
	// common accessor
	template <typename entity_t, typename char_t>
	struct crtp_accessor
	{
		using entity_type = entity_t;
		using char_type   = char_t;

		[[nodiscard]] auto as_int() const -> type::i64;

		template <type::concepts::pdn_sint in>
		[[nodiscard]] auto as_int(in) const -> in;

		[[nodiscard]] auto as_uint() const -> type::u64;

		template <type::concepts::pdn_uint un>
		[[nodiscard]] auto as_uint(un) const -> un;

		[[nodiscard]] auto as_fp() const -> type::f64;

		template <type::concepts::pdn_fp fn>
		[[nodiscard]] auto as_fp(fn) const -> fn;

		[[nodiscard]] auto as_bool() const -> type::boolean;

		[[nodiscard]] auto as_char() const -> type::character<char_type>;

		[[nodiscard]] auto as_string() const -> const type::string<char_type>&;

		[[nodiscard]] auto as_u8string() const -> unicode::u8string;

		[[nodiscard]] auto as_u16string() const -> unicode::u16string;

		[[nodiscard]] auto as_u32string() const -> unicode::u32string;

		[[nodiscard]] auto as_list() const -> const type::list<char_type>&;

		[[nodiscard]] auto as_object() const -> const type::object<char_type>&;

		// get_opt is for basic types only, returns std::nullopt if type mismatch or no value
		template <typename target_t>
		[[nodiscard]] auto get_opt() const -> ::std::optional<target_t>;

		// test if the entity holds a value of type target_t, returns false if type mismatch or no value
		template <typename target_t>
		[[nodiscard]] bool type_test() const;
	};

	// accessor for entity(get is entity only)
	template <typename entity_t, typename char_t>
	struct crtp_accessor_e : crtp_accessor<entity_t, char_t>
	{
		using entity_type = entity_t;
		using char_type   = char_t;

		template <typename target_t>
		[[nodiscard]] auto get() const& -> const target_t&;

		template <typename target_t>
		[[nodiscard]] auto get() && ->target_t&&;

		template <typename target_t>
		[[nodiscard]] auto get() & ->target_t&;

		template <typename target_t>
		[[nodiscard]] auto get_if() const& -> const target_t*;

		template <typename target_t>
		[[nodiscard]] auto get_if() & ->target_t*;

		template <typename target_t>
		auto get_if() && ->target_t* = delete;

		template <typename target_t>
		auto get_if() const&& -> const target_t* = delete;
	};
}

namespace pdn
{
	template <typename char_t>
	class const_refer : public detail::crtp_accessor<const_refer<char_t>, char_t>
	{
	public:
		using char_type   = char_t;
		using entity_type = entity<char_type>;
		using index_type  = ::std::size_t;
		using key_type    = ::std::basic_string_view<char_type>;
	public:
		auto at(index_type index)   const -> const_refer { return get() ? get()->at(index) : const_refer{}; }
		auto at(const key_type key) const -> const_refer { return get() ? get()->at(key)   : const_refer{}; }

		auto operator[](index_type index)   const -> const_refer { return at(index); }
		auto operator[](const key_type key) const -> const_refer { return at(key); }

		bool has_value() const noexcept { return get(); }
		// get() returns pointer to entity, not pointer to value of entity, nullable if no value
		const entity_type* get() const noexcept { return ptr; }
		const entity_type* operator->() const noexcept { return get(); }
		const entity_type& operator*() const noexcept { return *get(); }
		explicit operator bool() const noexcept { return has_value(); }
		// get_if() returns pointer to value of entity, not pointer to entity, nullable if no value or type mismatch
		template <typename target_t>
		[[nodiscard]] auto get_if() const -> const target_t*;

		const_refer() = default;
		const_refer(const const_refer& m) : ptr{ m.ptr } {}
		const_refer(const entity_type& e) : ptr{ &e } {}
		const_refer(entity_type&&) = delete;
		const_refer& operator=(const const_refer& m) = default;
	private:
		const entity_type* ptr{};
	};

	template <typename char_t>
	class refer : public const_refer<char_t>
	{
	private:
		using base_type = const_refer<char_t>;
	public:
		using char_type   = char_t;
		using entity_type = entity<char_type>;
		using index_type  = ::std::size_t;
		using key_type    = ::std::basic_string_view<char_type>;
	public:
		auto at(index_type index)   const -> refer { return get() ? get()->at(index) : refer{}; }
		auto at(const key_type key) const -> refer { return get() ? get()->at(key)   : refer{}; }

		auto operator[](index_type index)   const -> refer { return at(index); }
		auto operator[](const key_type key) const -> refer { return at(key); }

		bool has_value() const noexcept { return get(); }
		// get() returns pointer to entity, not pointer to value of entity, nullable if no value
		entity_type* get() const noexcept { return get_from_base(); }
		entity_type* operator->() const noexcept { return get(); }
		entity_type& operator*() const noexcept { return *get(); }
		explicit operator bool() const noexcept { return has_value(); }
		// get_if() returns pointer to value of entity, nullable if no value or type mismatch
		template <typename target_t>
		[[nodiscard]] auto get_if() const -> target_t*;

		refer() = default;
		refer(const refer& m) : base_type(m) {}
		refer(entity_type& e) : base_type(e) {}
		refer& operator=(const refer& m) = default;
	private:
		entity_type* get_from_base() const noexcept
		{
			return const_cast<entity_type*>(base_type::get());
		}
	};
}

namespace pdn
{
	template <typename char_t = unicode::u8char_t>
	class entity : public detail::crtp_accessor_e<entity<char_t>, char_t>, public type::detail::entity_variant<char_t>
	{
	public:
		using type::detail::entity_variant<char_t>::entity_variant;
		using char_type    = char_t;
		using index_type   = ::std::size_t;
		using key_type     = ::std::basic_string_view<char_type>;
		using i8           = type::i8;
		using i16          = type::i16;
		using i32          = type::i32;
		using i64          = type::i64;
		using u8           = type::u8;
		using u16          = type::u16;
		using u32          = type::u32;
		using u64          = type::u64;
		using f32          = type::f32;
		using f64          = type::f64;
		using boolean      = type::boolean;
		using character    = type::character<char_type>;
		using string       = type::string<char_type>;
		using list         = type::list<char_type>;
		using object       = type::object<char_type>;
		using string_proxy = proxy<string>;
		using list_proxy   = proxy<list>;
		using object_proxy = proxy<object>;

	public:
		auto  ref() const&& -> const_refer<char_type> = delete;
		auto  ref()      &  ->       refer<char_type> { return       refer<char_type>{ *this }; }
		auto  ref() const&  -> const_refer<char_type> { return const_refer<char_type>{ *this }; }
		auto cref() const&& -> const_refer<char_type> = delete;
		auto cref() const&  -> const_refer<char_type> { return const_refer<char_type>{ *this }; }

		auto at(index_type) const&& ->const_refer<char_type> = delete;
		auto at(index_type index) const& -> const_refer<char_type>
		{
			if (auto prp = ::std::get_if<list_proxy>(this); prp) // pointer to proxy of list
			{
				if (list& arr = **prp; index < arr.size())
				{
					return const_refer<char_type>{ arr[index] };
				}
			}
			return const_refer<char_type>{};
		}
		auto at(index_type index) & -> refer<char_type>
		{
			if (auto prp = ::std::get_if<list_proxy>(this); prp) // pointer to proxy of list
			{
				if (list& arr = **prp; index < arr.size())
				{
					return refer<char_type>{ arr[index] };
				}
			}
			return refer<char_type>{};
		}

		auto at(const key_type) const&& -> const_refer<char_type> = delete;
		auto at(const key_type key) const& -> const_refer<char_type>
		{
			if (auto prp = ::std::get_if<object_proxy>(this); prp) // pointer to proxy of object
			{
				object& o = **prp;
				if (auto it = o.find(key); it != o.end())
				{
					return const_refer<char_type>{ it->second };
				}
			}
			return const_refer<char_type>{};
		}
		auto at(const key_type key) & -> refer<char_type>
		{
			if (auto prp = ::std::get_if<object_proxy>(this); prp) // pointer to proxy of object
			{
				object& o = **prp;
				if (auto it = o.find(key); it != o.end())
				{
					return refer<char_type>{ it->second };
				}
			}
			return refer<char_type>{};
		}

		auto operator[](index_type index) const& -> const entity&
		{
			return get<list>(*this)[index];
		}
		auto operator[](index_type index) & -> entity&
		{
			return get<list>(*this)[index];
		}
		auto operator[](index_type index) && -> entity&&
		{
			return ::std::move((*this)[index]);
		}

		auto operator[](const key_type key) const& -> const entity&
		{
			auto& o = get<object>(*this);
			auto it = o.find(key);
			if (it == o.end())
			{
				throw ::std::out_of_range{ "out_of_range" };
			}
			return it->second;
		}
		auto operator[](const key_type key) & -> entity&
		{
			auto& o = get<object>(*this);
			auto it = o.find(key);
			if (it == o.end())
			{
				throw ::std::out_of_range{ "out_of_range" };
			}
			return it->second;
		}
		auto operator[](const key_type key) && -> entity&&
		{
			return ::std::move((*this)[key]);
		}
	};
}

// aliases
namespace pdn
{
	template <typename char_t>
	using entity_ref = refer<char_t>;

	template <typename char_t>
	using entity_cref = const_refer<char_t>;

	using u8entity       = entity<char8_t>;
	using u8entity_ref   = entity_ref<char8_t>;
	using u8entity_cref  = entity_cref<char8_t>;
	using u16entity      = entity<char16_t>;
	using u16entity_ref  = entity_ref<char16_t>;
	using u16entity_cref = entity_cref<char16_t>;
	using u32entity      = entity<char32_t>;
	using u32entity_ref  = entity_ref<char32_t>;
	using u32entity_cref = entity_cref<char32_t>;
}

// as accessors
namespace pdn
{
	inline constexpr auto auto_int_tag = type::auto_int{};
	inline constexpr auto i8_tag  = type::i8{};
	inline constexpr auto i16_tag = type::i16{};
	inline constexpr auto i32_tag = type::i32{};
	inline constexpr auto i64_tag = type::i64{};
	inline constexpr auto auto_uint_tag = type::auto_uint{};
	inline constexpr auto u8_tag  = type::u8{};
	inline constexpr auto u16_tag = type::u16{};
	inline constexpr auto u32_tag = type::u32{};
	inline constexpr auto u64_tag = type::u64{};
	inline constexpr auto f32_tag = type::f32{};
	inline constexpr auto f64_tag = type::f64{};

	template <typename char_t>
	[[nodiscard]] auto as_int(const entity<char_t>& e) -> type::i64
	{
		return ::std::visit([](const auto& v) { return detail::as_int(v); }, e);
	}

	template <typename char_t, type::concepts::pdn_sint in>
	[[nodiscard]] auto as_int(const entity<char_t>& e, in) -> in
	{
		return ::std::visit([]<typename arg_t>(const arg_t& v) { return detail::as_int<arg_t, in>(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_uint(const entity<char_t>& e) -> type::u64
	{
		return ::std::visit([](const auto& v) { return detail::as_uint(v); }, e);
	}

	template <typename char_t, type::concepts::pdn_uint un>
	[[nodiscard]] auto as_uint(const entity<char_t>& e, un) -> un
	{
		return ::std::visit([]<typename arg_t>(const arg_t& v) { return detail::as_uint<arg_t, un>(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_fp(const entity<char_t>& e) -> type::f64
	{
		return ::std::visit([](const auto& v) { return detail::as_fp(v); }, e);
	}

	template <typename char_t, type::concepts::pdn_fp fn>
	[[nodiscard]] auto as_fp(const entity<char_t>& e, fn) -> fn
	{
		return ::std::visit([]<typename arg_t>(const arg_t& v) { return detail::as_fp<arg_t, fn>(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_bool(const entity<char_t>& e) -> type::boolean
	{
		return ::std::visit([](const auto& v) { return detail::as_accessor<char_t>::as_bool(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_char(const entity<char_t>& e) -> type::character<char_t>
	{
		return ::std::visit([](const auto& v) { return detail::as_accessor<char_t>::as_char(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_string(const entity<char_t>& e) -> const type::string<char_t>&
	{
		return ::std::visit([](const auto& v) -> decltype(auto) { return detail::as_accessor<char_t>::as_string(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_u8string(const entity<char_t>& e) -> unicode::u8string
	{
		if constexpr (::std::same_as<::std::remove_cv_t<char_t>, unicode::u8char_t>
			       && ::std::convertible_to<decltype(as_string(e)), unicode::u8string>)
		{
			return as_string(e);
		}
		else
		{
			return unicode::code_convert<unicode::u8string>(as_string(e));
		}
	}

	template <typename char_t>
	[[nodiscard]] auto as_u16string(const entity<char_t>& e) -> unicode::u16string
	{
		if constexpr (::std::same_as<::std::remove_cv_t<char_t>, unicode::u16char_t>
			       && ::std::convertible_to<decltype(as_string(e)), unicode::u16string>)
		{
			return as_string(e);
		}
		else
		{
			return unicode::code_convert<unicode::u16string>(as_string(e));
		}
	}

	template <typename char_t>
	[[nodiscard]] auto as_u32string(const entity<char_t>& e) -> unicode::u32string
	{
		if constexpr (::std::same_as<::std::remove_cv_t<char_t>, unicode::u32char_t>
			       && ::std::convertible_to<decltype(as_string(e)), unicode::u32string>)
		{
			return as_string(e);
		}
		else
		{
			return unicode::code_convert<unicode::u32string>(as_string(e));
		}
	}

	template <typename char_t>
	[[nodiscard]] auto as_list(const entity<char_t>& e) -> const type::list<char_t>&
	{
		return ::std::visit([](const auto& v) -> decltype(auto) { return detail::as_accessor<char_t>::as_list(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_object(const entity<char_t>& e) -> const type::object<char_t>&
	{
		return ::std::visit([](const auto& v) -> decltype(auto) { return detail::as_accessor<char_t>::as_object(v); }, e);
	}

	template <typename char_t>
	[[nodiscard]] auto as_int(const_refer<char_t> e) -> type::i64
	{
		return e ? as_int(*e) : type::i64{};
	}

	template <typename char_t, type::concepts::pdn_sint in>
	[[nodiscard]] auto as_int(const const_refer<char_t>& e, in tag) -> in
	{
		return e ? as_int(*e, tag) : in{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_uint(const_refer<char_t> e) -> type::u64
	{
		return e ? as_uint(*e) : type::u64{};
	}

	template <typename char_t, type::concepts::pdn_uint un>
	[[nodiscard]] auto as_uint(const const_refer<char_t>& e, un tag) -> un
	{
		return e ? as_uint(*e, tag) : un{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_fp(const_refer<char_t> e) -> type::f64
	{
		return e ? as_fp(*e) : type::f64{};
	}

	template <typename char_t, type::concepts::pdn_fp fn>
	[[nodiscard]] auto as_fp(const const_refer<char_t>& e, fn tag) -> fn
	{
		return e ? as_fp(*e, tag) : fn{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_bool(const_refer<char_t> e) -> type::boolean
	{
		return e ? as_bool(*e) : type::boolean{};
	}

	template <typename char_t>
	[[nodiscard]] inline auto as_char(const_refer<char_t> e) -> type::character<char_t>
	{
		return e ? as_char(*e) : type::character<char_t>{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_string(const_refer<char_t> e) -> const type::string<char_t>&
	{
		return e ? as_string(*e) : detail::as_accessor<char_t>::null_string_val();
	}

	template <typename char_t>
	[[nodiscard]] auto as_u8string(const_refer<char_t> e) -> unicode::u8string
	{
		return e ? as_u8string(*e) : unicode::u8string{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_u16string(const_refer<char_t> e) -> unicode::u16string
	{
		return e ? as_u16string(*e) : unicode::u16string{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_u32string(const_refer<char_t> e) -> unicode::u32string
	{
		return e ? as_u32string(*e) : unicode::u32string{};
	}

	template <typename char_t>
	[[nodiscard]] auto as_list(const_refer<char_t> e) -> const type::list<char_t>&
	{
		return e ? as_list(*e) : detail::as_accessor<char_t>::null_list_val();
	}

	template <typename char_t>
	[[nodiscard]] auto as_object(const_refer<char_t> e) -> const type::object<char_t>&
	{
		return e ? as_object(*e) : detail::as_accessor<char_t>::null_object_val();
	}
}

// get, get_if, get_opt, type_test accessors
namespace pdn
{
	template <typename target_t, typename char_t>
	[[nodiscard]] auto get(const entity<char_t>& e) -> const target_t&
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			return *::std::get<proxy<target_t>>(e);
		}
		else
		{
			return ::std::get<target_t>(e);
		}
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get(entity<char_t>&& e) -> target_t&&
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			return ::std::move(*::std::get<proxy<target_t>>(e));
		}
		else
		{
			return ::std::get<target_t>(::std::move(e));
		}
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get(entity<char_t>& e) -> target_t&
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			return *::std::get<proxy<target_t>>(e);
		}
		else
		{
			return ::std::get<target_t>(e);
		}
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_if(const entity<char_t>& e) -> const target_t*
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			if (auto p = ::std::get_if<proxy<target_t>>(&e))
			{
				return p->get();
			}
			return nullptr;
		}
		else
		{
			return ::std::get_if<target_t>(&e);
		}
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_if(entity<char_t>& e) -> target_t*
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			if (auto p = ::std::get_if<proxy<target_t>>(&e))
			{
				return p->get();
			}
			return nullptr;
		}
		else
		{
			return ::std::get_if<target_t>(&e);
		}
	}

	template <typename target_t, typename char_t>
	auto get_if(entity<char_t>&& e) -> target_t* = delete;

	template <typename target_t, typename char_t>
	auto get_if(const entity<char_t>&& e) -> const target_t* = delete;

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_if(const_refer<char_t> e) -> const target_t*
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			if (auto p = ::std::get_if<proxy<target_t>>(e.get()))
			{
				return p->get();
			}
			return nullptr;
		}
		else
		{
			return ::std::get_if<target_t>(e.get());
		}
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_if(refer<char_t> e) -> target_t*
	{
		if constexpr (detail::has_proxy_v<target_t>)
		{
			if (auto p = ::std::get_if<proxy<target_t>>(e.get()))
			{
				return p->get();
			}
			return nullptr;
		}
		else
		{
			return ::std::get_if<target_t>(e.get());
		}
	}

	// just for basic types
	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_opt(const entity<char_t>& e) -> ::std::optional<target_t>
	{
		static_assert(type::concepts::basic_types<target_t, char_t>, "requires pdn basic types");
		if (auto p = ::std::get_if<target_t>(&e))
		{
			return ::std::make_optional<target_t>(*p);
		}
		return ::std::nullopt;
	}

	// just for basic types
	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_opt(const_refer<char_t> e) -> ::std::optional<target_t>
	{
		static_assert(type::concepts::basic_types<target_t, char_t>, "requires pdn basic types");
		if (auto p = ::std::get_if<target_t>(e.get()))
		{
			return ::std::make_optional<target_t>(*p);
		}
		return ::std::nullopt;
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] bool type_test(const entity<char_t>& e)
	{
		return get_if<target_t>(e);
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] bool type_test(const_refer<char_t> e)
	{
		return get_if<target_t>(e);
	}
}

namespace pdn
{
	template <typename char_t>
	template <typename target_t>
	[[nodiscard]] auto const_refer<char_t>::get_if() const -> const target_t*
	{
		return pdn::get_if<target_t>(*this);
	}

	template <typename char_t>
	template <typename target_t>
	[[nodiscard]] auto refer<char_t>::get_if() const -> target_t*
	{
		return pdn::get_if<target_t>(*this);
	}
}

namespace pdn::detail
{
	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_int() const -> type::i64
	{
		return pdn::as_int(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <type::concepts::pdn_sint in>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_int(in) const -> in
	{
		return pdn::as_int(*static_cast<const entity_t*>(this), in{});
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_uint() const -> type::u64
	{
		return pdn::as_uint(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <type::concepts::pdn_uint un>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_uint(un) const -> un
	{
		return pdn::as_uint(*static_cast<const entity_t*>(this), un{});
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_fp() const -> type::f64
	{
		return pdn::as_fp(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <type::concepts::pdn_fp fn>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_fp(fn) const -> fn
	{
		return pdn::as_fp(*static_cast<const entity_t*>(this), fn{});
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_bool() const -> type::boolean
	{
		return pdn::as_bool(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_char() const -> type::character<char_type>
	{
		return pdn::as_char(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_string() const -> const type::string<char_type>&
	{
		return pdn::as_string(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_u8string() const -> unicode::u8string
	{
		return pdn::as_u8string(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_u16string() const -> unicode::u16string
	{
		return pdn::as_u16string(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_u32string() const -> unicode::u32string
	{
		return pdn::as_u32string(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_list() const -> const type::list<char_type>&
	{
		return pdn::as_list(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::as_object() const -> const type::object<char_type>&
	{
		return pdn::as_object(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get() const& -> const target_t&
	{
		auto& self = *static_cast<const entity_t*>(this);
		return pdn::get<target_t>(self);
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get() && ->target_t&&
	{
		auto& self = *static_cast<entity_t*>(this);
		return pdn::get<target_t>(::std::move(self));
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get() & ->target_t&
	{
		auto& self = *static_cast<entity_t*>(this);
		return pdn::get<target_t>(self);
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get_if() const& -> const target_t*
	{
		return pdn::get_if<target_t>(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get_if() & ->target_t*
	{
		return pdn::get_if<target_t>(*static_cast<entity_t*>(this));
	}

	// just for basic types
	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::get_opt() const -> ::std::optional<target_t>
	{
		return pdn::get_opt<target_t>(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] bool crtp_accessor<entity_t, char_t>::type_test() const
	{
		return pdn::type_test<target_t>(*static_cast<const entity_t*>(this));
	}
}

#endif
