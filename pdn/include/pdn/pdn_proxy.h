#ifndef PDN_Header_pdn_proxy
#define PDN_Header_pdn_proxy

#include <cstddef>
#include <type_traits>
#include <utility>
#include <memory>

#include "pdn_false.h"

// std::unique_ptr (hence every accessor of proxy) is constexpr only since C++23 (P2273).
// __cpp_lib_constexpr_memory is 201811L in C++20 (that value belongs to P0784, not to
// constexpr unique_ptr) and 202202L from C++23 on, so testing the value - not just the
// presence of the macro - is required here.
// In C++20 the accessors are deliberately left non-constexpr: a constexpr member of a
// class template that no specialization can ever satisfy is ill-formed, no diagnostic
// required ([dcl.constexpr]/5). Plain accessors keep the library conforming and start
// working in constant expressions as soon as the project moves to C++23.
#if defined(__cpp_lib_constexpr_memory) && __cpp_lib_constexpr_memory >= 202202L
#	define PDN_CONSTEXPR_PROXY constexpr
#else
#	define PDN_CONSTEXPR_PROXY
#endif

namespace pdn
{
	template <typename t>
	class proxy
	{
	private:
		using handle_type = ::std::unique_ptr<t>;
	public:
		using element_type = typename handle_type::element_type;
		using pointer = typename handle_type::pointer;
		using const_pointer = typename ::std::pointer_traits<pointer>::template rebind<::std::add_const_t<t>>;
		using deleter_type = typename handle_type::deleter_type;
		explicit PDN_CONSTEXPR_PROXY operator bool() const noexcept
		{
			return static_cast<bool>(handle);
		}
		PDN_CONSTEXPR_PROXY auto get() noexcept -> pointer
		{
			return handle.get();
		}
		PDN_CONSTEXPR_PROXY auto get() const noexcept -> const_pointer
		{
			return handle.get();
		}
		PDN_CONSTEXPR_PROXY auto operator*() noexcept -> ::std::add_lvalue_reference_t<t>
		{
			return *handle;
		}
		PDN_CONSTEXPR_PROXY auto operator*() const noexcept -> ::std::add_lvalue_reference_t<::std::add_const_t<t>>
		{
			return *handle;
		}
		PDN_CONSTEXPR_PROXY auto operator->() noexcept -> pointer
		{
			return handle.get();
		}
		PDN_CONSTEXPR_PROXY auto operator->() const noexcept -> const_pointer
		{
			return handle.get();
		}
		proxy() : proxy(::std::make_unique<t>()) {}
		proxy(const proxy& o) : proxy(::std::make_unique<t>(*o)) {}
		proxy(proxy&& o) noexcept
		{
			*this = ::std::move(o);
		}
		proxy& operator=(const proxy& o)
		{
			*this = proxy{ o };
			return *this;
		}
		proxy& operator=(proxy&& o) noexcept
		{
			::std::swap(handle, o.handle);
			return *this;
		}
		template <typename type, typename... args_t>
		friend auto make_proxy(args_t&&... args) -> proxy<type>;
	private:
		handle_type handle{};
		explicit proxy(handle_type h) : handle{ ::std::move(h) } {}
	};

	template <typename type>
	class proxy<type[]>
	{
		static_assert(detail::false_v<type[]>, "[pdn] cannot create proxy class for array");
	};

	template <typename type, ::std::size_t n>
	class proxy<type[n]>
	{
		static_assert(detail::false_v<type[n]>, "[pdn] cannot create proxy class for array");
	};

	template <typename type, typename... args_t>
	auto make_proxy(args_t&&... args) -> proxy<type>
	{
		return proxy<type>{ ::std::make_unique<type>(::std::forward<args_t>(args)...) };
	}
}

namespace pdn::detail
{
	template <typename t>
	struct remove_proxy_helper
	{
		using type = t;
	};

	template <typename t>
	struct remove_proxy_helper<proxy<t>>
	{
		using type = t;
	};
}

namespace pdn
{
	// The type behind a proxy, with cv-qualifiers and references removed, like std::remove_cvref_t.
	template <typename t>
	struct remove_proxy : detail::remove_proxy_helper<::std::remove_cvref_t<t>> {};

	template <typename t>
	using remove_proxy_t = typename remove_proxy<t>::type;
}

#undef PDN_CONSTEXPR_PROXY

#endif
