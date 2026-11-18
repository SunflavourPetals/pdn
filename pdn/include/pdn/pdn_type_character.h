#ifndef PDN_Header_pdn_type_character
#define PDN_Header_pdn_type_character

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_type_character_utility.h"

namespace pdn::type::detail
{
	template <unicode::concepts::code_unit char_t>
	class character
	{
	private:
		static_assert(::std::same_as<char_t, ::std::remove_cv_t<char_t>>,
		              "[pdn] character requires a char type without cv-qualifiers");
		// A plain value type storing at most max_size() code units plus a size. That
		// they encode exactly one Unicode scalar value is the caller's requirement (the
		// lexer guarantees it for PDN source), not an invariant checked here; (first,
		// count) copies trunc_size(count) units, so count truncates to max_size().
		static constexpr auto max_sz = detail::max_code_unit_count_for_character_v<char_t>;
		using data_type = ::std::array<char_t, max_sz>;
		static constexpr auto trunc_size(::std::size_t sz) noexcept { return sz > max_sz ? max_sz : sz; }
	public:
		using small_size_type = ::std::uint32_t;
		using value_type      = char_t;
		using size_type       = typename data_type::size_type;
		using difference_type = ::std::ptrdiff_t;
		using pointer         = value_type*;
		using const_pointer   = const value_type*;
		using reference       = value_type&;
		using const_reference = const value_type&;
	public:
		static constexpr auto max_size() noexcept { return max_sz; }
		constexpr character() noexcept = default;
		template <typename it_t>
		constexpr character(it_t first, size_type count)
			noexcept(noexcept(static_cast<char_t>(*first)) && noexcept(++first)) :
			sz{ static_cast<small_size_type>(trunc_size(count)) }
		{
			const auto end = cont.begin() + sz;
			for (auto it = cont.begin(); it != end; ++it, ++first)
			{
				*it = static_cast<char_t>(*first);
			}
		}
		constexpr character(const character& rhs) noexcept : cont{ rhs.cont }, sz{ rhs.sz } {}
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
		friend constexpr bool operator==(character lhs, character rhs) noexcept
		{
			return lhs.to_string_view() == rhs.to_string_view();
		}
	private:
		data_type       cont{};
		small_size_type sz{ 1u };
	};
}

#endif
