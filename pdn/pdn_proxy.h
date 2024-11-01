#ifndef PDN_Header_pdn_proxy
#define PDN_Header_pdn_proxy

#include <memory>

namespace pdn
{
	template <typename t>
	class proxy : public ::std::unique_ptr<t>
	{
	private:
		using base_type = ::std::unique_ptr<t>;
	public:
		using typename base_type::element_type;
		using typename base_type::pointer;
		using typename base_type::deleter_type;
		using base_type::base_type;
		proxy(const proxy& o)         : proxy(static_cast<const base_type&>(o)) {}
		proxy(proxy&& o) noexcept     : proxy(static_cast<base_type&&>(o)) {}
		proxy(const base_type& o)     : base_type(o ? ::std::make_unique<t>(*o) : base_type{}) {}
		proxy(base_type&& o) noexcept : base_type(::std::move(o)) {}
		proxy& operator=(const proxy& o)
		{
			*this = proxy{ o };
			return *this;
		}
		proxy& operator=(const base_type& o)
		{
			*this = proxy{ o };
			return *this;
		}
		proxy& operator=(proxy&& o) noexcept
		{
			this->base_type::operator=(::std::move(o));
			return *this;
		}
		proxy& operator=(base_type&& o) noexcept
		{
			this->base_type::operator=(::std::move(o));
			return *this;
		}
	};

	template <typename t>
	class proxy<t[]>
	{
		static_assert(false, "[pdn] cannot create proxy class for array");
	};

	template <typename t, typename... args_t>
	proxy<t> make_proxy(args_t&&... args)
	{
		return ::std::make_unique<t>(::std::forward<args_t>(args)...);
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
