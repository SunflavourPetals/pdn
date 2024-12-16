#ifndef PDN_Header_pdn_types_character
#define PDN_Header_pdn_types_character

#include <cstddef>
#include <cstdint>
#include <string>

#include "pdn_types_character_utility.h"

namespace pdn::types::impl
{
	template <typename char_t>
	class character
	{
	private:
		static constexpr auto max_sz = dev_util::max_code_unit_count_for_character_v<char_t>;
		using data_type = ::std::array<char_t, max_sz>;
		static constexpr auto trunc_size(::std::size_t sz) { return sz > max_sz ? max_sz : sz; }
	public:
		using small_size_type = ::std::uint_fast16_t;
		using value_type      = char_t;
		using size_type       = typename data_type::size_type;
		using difference_type = ::std::ptrdiff_t;
		using pointer         = value_type*;
		using const_pointer   = const value_type*;
		using reference       = value_type&;
		using const_reference = const value_type&;
	public:
		static constexpr auto max_size() { return max_sz; }
		constexpr character() = default;
		template <typename it_t>
		constexpr character(it_t first, size_type count) noexcept :
			sz{ static_cast<small_size_type>(trunc_size(count)) }
		{
			const auto end = cont.begin() + sz;
			for (auto it = cont.begin(); it != end; ++it, ++first)
			{
				*it = *first;
			}
		}
		constexpr character(const character& rhs) : cont{ rhs.cont }, sz{ rhs.sz } {}
		constexpr character& operator= (const character& rhs) noexcept = default;
		constexpr auto data() const noexcept -> const_pointer
		{
			return cont.data();
		}
		constexpr auto size() const noexcept -> size_type
		{
			return sz;
		}
		constexpr auto to_string_view() const noexcept -> ::std::basic_string_view<char_t>
		{
			return { data(), size() };
		}
	private:
		data_type       cont{};
		small_size_type sz{ 1u };
	};
}

#endif
