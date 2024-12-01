#ifndef PDN_Header_pdn_proxy
#define PDN_Header_pdn_proxy

#include <type_traits>
#include <utility>
#include <memory>

namespace pdn
{
	template <typename t>
	class proxy
	{
	private:
		using handle_type = ::std::unique_ptr<t>;
	public:
		using element_type = typename handle_type::element_type;
		using pointer      = typename handle_type::pointer;
		using deleter_type = typename handle_type::deleter_type;
		explicit operator bool() const noexcept
		{
			return static_cast<bool>(handle);
		}
		auto operator*() const noexcept -> ::std::add_lvalue_reference_t<t>
		{
			return *handle;
		}
		auto operator->() const noexcept -> pointer
		{
			return handle.get();
		}
		proxy() = default;
		explicit proxy(handle_type h) : handle{ ::std::move(h) } {}
		proxy(const proxy& o) : handle{ ::std::make_unique<t>(*o) } {}
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
	private:
		handle_type handle{};
	};

	template <typename type>
	class proxy<type[]>
	{
		static_assert(false, "[pdn] cannot create proxy class for array");
	};

	template <typename type, typename... args_t>
	auto make_proxy(args_t&&... args) -> proxy<type>
	{
		return proxy<type>{ ::std::make_unique<type>(::std::forward<args_t>(args)...) };
	}
}

namespace pdn::type_traits
{
	template <typename t>
	struct remove_proxy
	{
		using type = t;
	};
	template <typename t>
	struct remove_proxy<proxy<t>>
	{
		using type = t;
	};
	template <typename t>
	struct remove_proxy<const proxy<t>>
	{
		using type = t;
	};
	template <typename t>
	struct remove_proxy<volatile proxy<t>>
	{
		using type = t;
	};
	template <typename t>
	struct remove_proxy<const volatile proxy<t>>
	{
		using type = t;
	};
	template <typename t>
	using remove_proxy_t = typename remove_proxy<t>::type;
}

#endif
