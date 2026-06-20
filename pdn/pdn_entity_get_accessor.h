#ifndef PDN_Header_pdn_entity_get_accessor
#define PDN_Header_pdn_entity_get_accessor

#include <type_traits>
#include <utility>
#include <variant>
#include <optional>

#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_entity_utility.h"
#include "pdn_entity_forward_decl.h"
#include "pdn_entity_crtp_accessor.h"

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
	[[nodiscard]] auto get_ptr(const entity<char_t>& e) -> const target_t*
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
	[[nodiscard]] auto get_ptr(entity<char_t>& e) -> target_t*
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
	auto get_ptr(entity<char_t>&& e) -> target_t* = delete;

	template <typename target_t, typename char_t>
	auto get_ptr(const entity<char_t>&& e) -> const target_t* = delete;

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_ptr(const_refer<char_t> e) -> const target_t*
	{
		return e.get_ptr<target_t>();
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_ptr(refer<char_t> e) -> target_t*
	{
		return e.get_ptr<target_t>();
	}

	// just for basic types
	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_optional(const entity<char_t>& e) -> ::std::optional<target_t>
	{
		static_assert(types::concepts::basic_types<target_t, char_t>, "requires pdn basic types");
		if (auto p = ::std::get_if<target_t>(&e))
		{
			return ::std::make_optional<target_t>(*p);
		}
		return ::std::nullopt;
	}

	// just for basic types
	template <typename target_t, typename char_t>
	[[nodiscard]] auto get_optional(const_refer<char_t> e) -> ::std::optional<target_t>
	{
		static_assert(types::concepts::basic_types<target_t, char_t>, "requires pdn basic types");
		if (auto p = ::std::get_if<target_t>(e.get()))
		{
			return ::std::make_optional<target_t>(*p);
		}
		return ::std::nullopt;
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] bool type_test(const entity<char_t>& e)
	{
		return get_ptr<target_t>(e);
	}

	template <typename target_t, typename char_t>
	[[nodiscard]] bool type_test(const_refer<char_t> e)
	{
		return get_ptr<target_t>(e);
	}
}

namespace pdn::detail
{
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
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get_ptr() const& -> const target_t*
	{
		return pdn::get_ptr<target_t>(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor_e<entity_t, char_t>::get_ptr() & ->target_t*
	{
		return pdn::get_ptr<target_t>(*static_cast<entity_t*>(this));
	}
}

namespace pdn::detail
{
	// just for basic types
	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] auto crtp_accessor<entity_t, char_t>::get_optional() const -> ::std::optional<target_t>
	{
		return pdn::get_optional<target_t>(*static_cast<const entity_t*>(this));
	}

	template <typename entity_t, typename char_t>
	template <typename target_t>
	[[nodiscard]] bool crtp_accessor<entity_t, char_t>::type_test() const
	{
		return pdn::type_test<target_t>(*static_cast<const entity_t*>(this));
	}
}

#endif
