#ifndef PDN_Header_pdn_ordered_map
#define PDN_Header_pdn_ordered_map

#include <utility>
#include <memory>
#include <functional>

#include <type_traits>
#include <algorithm>
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

		auto begin()        { return bt::begin();  }
		auto end()          { return bt::end();    }
		auto begin()  const { return bt::begin();  }
		auto end()    const { return bt::end();    }
		auto cbegin() const { return bt::cbegin(); }
		auto cend()   const { return bt::cend();   }

		auto empty() const { return bt::empty(); }
		auto size()  const { return bt::size();  }

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
	};
}

#endif
