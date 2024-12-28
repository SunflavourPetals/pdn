#ifndef PDN_Header_pdn_ordered_map
#define PDN_Header_pdn_ordered_map

#include <utility>
#include <memory>
#include <functional>

#include <type_traits>
#include <algorithm>
#include <stdexcept>

#include <deque>

namespace pdn::types::impl
{
	template <typename key_t,
	          typename val_t,
	          template <typename t, typename alloc> typename container_t = ::std::deque,
	          typename alloc_t = ::std::allocator<::std::pair<const key_t, val_t>>>
	class ordered_map : public container_t<::std::pair<const key_t, val_t>, alloc_t>
	{
		using base_type = container_t<::std::pair<const key_t, val_t>, alloc_t>;
		using bt = base_type;
	public:
		using key_type        = key_t;
		using mapped_type     = val_t;
		using value_type      = ::std::pair<const key_t, val_t>;
		using size_type       = bt::size_type;
		using difference_type = bt::difference_type;
		using allocator_type  = alloc_t;
		using reference       = value_type&;
		using const_reference = const value_type&;
		using pointer         = ::std::allocator_traits<alloc_t>::pointer;
		using const_pointer   = ::std::allocator_traits<alloc_t>::const_pointer;
		using iterator        = bt::iterator;
		using const_iterator  = bt::const_iterator;

		using base_type::base_type;

		auto get_base() noexcept -> base_type&
		{
			return *this;
		}
		auto get_base() const noexcept -> const base_type&
		{
			return *this;
		}

		auto get_allocator() const noexcept -> allocator_type
		{
			return bt::get_allocator();
		}

		auto begin()        { return bt::begin();  }
		auto end()          { return bt::end();    }
		auto begin()  const { return bt::begin();  }
		auto end()    const { return bt::end();    }
		auto cbegin() const { return bt::cbegin(); }
		auto cend()   const { return bt::cend();   }

		auto empty() const { return bt::empty(); }
		auto size()  const { return bt::size();  }

		void clear() noexcept
		{
			bt::clear();
		}

		iterator erase(iterator pos)
		{
			return bt::erase(pos);
		}

		iterator erase(const_iterator pos)
		{
			return bt::erase(pos);
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			return bt::erase(first, last);
		}

		template <typename k_t>
		size_type erase(k_t&& k)
		{
			auto k_it = find(k);
			if (k_it == end()) return 0;
			return erase(k_it), 1;
		}

		template <typename k_t>
		auto at(const k_t& k) const -> const mapped_type&
		{
			auto k_it = find(k);
			if (k_it == end())
			{
				throw ::std::out_of_range{ "out of range" };
			}
			return k_it->second;
		}

		template <typename k_t>
		auto at(const k_t& k) -> mapped_type&
		{
			auto k_it = find(k);
			if (k_it == end())
			{
				throw ::std::out_of_range{ "out of range" };
			}
			return k_it->second;
		}

		template <typename k_t>
		auto operator[](k_t&& k) -> mapped_type&
		{
			auto find_r = find(k);
			if (find_r != cend())
			{
				return find_r->second;
			}
			bt::emplace_back(::std::forward<k_t>(k), mapped_type{});
			return bt::back().second;
		}

		template <typename k_t>
		bool contains(const k_t& k) const
		{
			return find(k) != end();
		}

		template <typename k_t>
		auto count(const k_t& k) const -> size_type
		{
			return contains(k);
		}

		template <typename k_t>
		auto find(const k_t& k) const
		{
			return ::std::find_if(begin(), end(), [&](const auto& v) { return v.first == k; });
		}

		template <typename k_t>
		auto find(const k_t& k)
		{
			return ::std::find_if(begin(), end(), [&](const auto& v) { return v.first == k; });
		}
	};
}

#endif
