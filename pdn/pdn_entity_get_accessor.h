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

namespace pdn
{
	template <typename target_t, typename char_t>
	[[nodiscard]] auto get(const entity<char_t>& e) -> const target_t&
	{
		if constexpr (dev_util::has_proxy_v<target_t>)
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
		if constexpr (dev_util::has_proxy_v<target_t>)
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
		if constexpr (dev_util::has_proxy_v<target_t>)
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
		if constexpr (dev_util::has_proxy_v<target_t>)
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
		if constexpr (dev_util::has_proxy_v<target_t>)
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
	[[nodiscard]] auto get_ptr(const_refer<char_t> e) -> const target_t*
	{
		if constexpr (dev_util::has_proxy_v<target_t>)
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
	[[nodiscard]] auto get_ptr(refer<char_t> e) -> target_t*
	{
		if constexpr (dev_util::has_proxy_v<target_t>)
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

#endif
