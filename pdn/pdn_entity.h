#ifndef PDN_Header_pdn_entity
#define PDN_Header_pdn_entity

#include <type_traits>
#include <utility>
#include <variant>
#include <cstddef>
#include <string>

#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_entity_utility.h"
#include "pdn_entity_forward_decl.h"

namespace pdn
{
	template <typename char_t>
	class refer
	{
	public:
		using char_type   = char_t;
		using entity_type = entity<char_type>;
		using index_type  = ::std::size_t;
		using key_type    = ::std::basic_string_view<char_type>;
	public:
		auto at(index_type index)   const -> refer { return ptr ? refer{ ptr->at(index) } : refer{}; }
		auto at(const key_type key) const -> refer { return ptr ? refer{ ptr->at(key)   } : refer{}; }

		auto operator[](index_type index)   const -> refer { return at(index); }
		auto operator[](const key_type key) const -> refer { return at(key); }

		bool has_value() const noexcept { return get(); }
		entity_type* get() const noexcept { return ptr; }
		entity_type* operator->() const noexcept { return ptr; }
		entity_type& operator*() const noexcept { return *ptr; }
		explicit operator bool() const noexcept { return has_value(); }

		refer() = default;
		refer(const refer& m) : ptr{ m.ptr } {}
		refer(entity_type& e) : ptr{ &e } {}
		refer& operator=(const refer& m) = default;
	private:
		entity_type* ptr{};
	};

	template <typename char_t>
	class const_refer
	{
	public:
		using char_type   = char_t;
		using entity_type = entity<char_type>;
		using index_type  = ::std::size_t;
		using key_type    = ::std::basic_string_view<char_type>;
	public:
		auto at(index_type index)   const -> const_refer { return ptr ? const_refer{ ptr->at(index) } : const_refer{}; }
		auto at(const key_type key) const -> const_refer { return ptr ? const_refer{ ptr->at(key)   } : const_refer{}; }

		auto operator[](index_type index)   const -> const_refer { return at(index); }
		auto operator[](const key_type key) const -> const_refer { return at(key); }

		bool has_value() const noexcept { return get(); }
		const entity_type* get() const noexcept { return ptr; }
		const entity_type* operator->() const noexcept { return ptr; }
		const entity_type& operator*() const noexcept { return *ptr; }
		explicit operator bool() const noexcept { return has_value(); }

		const_refer() = default;
		const_refer(const const_refer& m) : ptr{ m.ptr } {}
		const_refer(const entity_type& e) : ptr{ &e } {}
		const_refer(refer<char_t> r) : ptr{ r ? r.get() : nullptr } {}
		const_refer& operator=(const const_refer& m) = default;
	private:
		const entity_type* ptr{};
	};
}

namespace pdn
{
	template <typename char_t = unicode::utf_8_code_unit_t>
	class entity : public types::dev_util::entity_variant<char_t>
	{
	public:
		using types::dev_util::entity_variant<char_t>::entity_variant;
		using char_type    = char_t;
		using index_type   = ::std::size_t;
		using key_type     = ::std::basic_string_view<char_type>;
		using i8           = types::i8;
		using i16          = types::i16;
		using i32          = types::i32;
		using i64          = types::i64;
		using u8           = types::u8;
		using u16          = types::u16;
		using u32          = types::u32;
		using u64          = types::u64;
		using f32          = types::f32;
		using f64          = types::f64;
		using boolean      = types::boolean;
		using character    = types::character<char_type>;
		using string       = types::string<char_type>;
		using list         = types::list<char_type>;
		using object       = types::object<char_type>;
		using string_proxy = proxy<string>;
		using list_proxy   = proxy<list>;
		using object_proxy = proxy<object>;

	public:
		auto  ref()       & ->       refer<char_type> { return       refer<char_type>{ *this }; }
		auto  ref() const & -> const_refer<char_type> { return const_refer<char_type>{ *this }; }
		auto cref() const & -> const_refer<char_type> { return const_refer<char_type>{ *this }; }

		auto at(index_type index) const -> const_refer<char_type>
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
		auto at(index_type index) -> refer<char_type>
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
		auto at(const key_type key) const -> const_refer<char_type>
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
		auto at(const key_type key) -> refer<char_type>
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

		auto operator[](index_type index) const -> const entity& { return get<list>(*this)[index]; }
		auto operator[](index_type index)       ->       entity& { return get<list>(*this)[index]; }

		auto operator[](const key_type key) const -> const entity&
		{
			auto& o = get<object>(*this);
			auto it = o.find(key);
			if (it == o.end())
			{
				throw ::std::out_of_range{ "out_of_range" };
			}
			return it->second;
		}
		auto operator[](const key_type key) -> entity&
		{
			auto& o = get<object>(*this);
			auto it = o.find(key);
			if (it == o.end())
			{
				throw ::std::out_of_range{ "out_of_range" };
			}
			return it->second;
		}
	};
}

#endif
