#ifndef PDN_Header_pdn_entity_utility
#define PDN_Header_pdn_entity_utility

#include <type_traits>
#include <variant>
#include <cmath>

#include "pdn_types.h"
#include "pdn_proxy.h"

#include "pdn_entity_forward_decl.h"

namespace pdn::types::detail
{
	template <typename char_t>
	using entity_variant = ::std::variant<
		i32, i64, i8, i16,
		u32, u64, u8, u16,
		f32, f64,
		boolean,
		character<char_t>,
		proxy<string<char_t>>,
		proxy<list<char_t>>,
		proxy<object<char_t>>>;
}

namespace pdn::detail
{
	template <typename arg_t, types::concepts::pdn_sint tar_int = types::i64>
	inline auto as_int(const arg_t& arg) -> tar_int
	{
		using namespace types::concepts;
		if constexpr (pdn_bool<arg_t>)
		{
			return arg;
		}
		else if constexpr (pdn_sint<arg_t>)
		{
			constexpr auto tar_max = ::std::numeric_limits<tar_int>::max();
			constexpr auto tar_min = ::std::numeric_limits<tar_int>::min();
			if (arg > tar_max) return tar_max;
			if (arg < tar_min) return tar_min;
			return static_cast<tar_int>(arg);
		}
		else if constexpr (pdn_uint<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<tar_int>::max();
			return arg > types::u64(max) ? max : tar_int(arg);
		}
		else if constexpr (pdn_fp<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<tar_int>::max();
			constexpr auto min = ::std::numeric_limits<tar_int>::min();
			if (::std::isnan(arg)) return 0;
			if (arg >= arg_t(max)) return max;
			if (arg <= arg_t(min)) return min;
			return static_cast<tar_int>(arg);
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t, types::concepts::pdn_uint tar_uint = types::u64>
	inline auto as_uint(const arg_t& arg) -> tar_uint
	{
		using namespace types::concepts;
		if constexpr (pdn_bool<arg_t>)
		{
			return arg;
		}
		else if constexpr (pdn_uint<arg_t>)
		{
			constexpr auto tar_max = ::std::numeric_limits<tar_uint>::max();
			if (arg > tar_max) return tar_max;
			return static_cast<tar_uint>(arg);
		}
		else if constexpr (pdn_sint<arg_t>)
		{
			constexpr auto tar_max = ::std::numeric_limits<tar_uint>::max();
			if (arg < 0) return 0;
			if (static_cast<types::u64>(arg) > tar_max) return tar_max;
			return static_cast<tar_uint>(arg);
		}
		else if constexpr (pdn_fp<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<tar_uint>::max();
			if (::std::isnan(arg)) return 0;
			if (arg > arg_t(max)) return max;
			return arg < 0 ? tar_uint(0) : tar_uint(arg);
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t, types::concepts::pdn_fp tar_fp = types::f64>
	inline auto as_fp(const arg_t& arg) -> tar_fp
	{
		using namespace types::concepts;
		if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
		{
			return static_cast<tar_fp>(arg);
		}
		else
		{
			return 0;
		}
	}

	template <typename char_t, typename arg_t>
	inline auto as_bool(const arg_t& arg) -> types::boolean
	{
		using namespace types::concepts;
		if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
		{
			return static_cast<bool>(arg);
		}
		else if constexpr (::std::same_as<arg_t, types::character<char_t>>)
		{
			return arg != types::character<char_t>{};
		}
		else
		{
			return false;
		}
	}

	template <typename char_t>
	struct as_accessor
	{
		template <typename arg_t, typename in = types::i64>
		static auto as_int(const arg_t& arg) -> in
		{
			return detail::as_int<arg_t, in>(arg);
		}

		template <typename arg_t, typename un = types::u64>
		static auto as_uint(const arg_t& arg) -> un
		{
			return detail::as_uint<arg_t, un>(arg);
		}

		template <typename arg_t, typename fn = types::f64>
		static auto as_fp(const arg_t& arg) -> fn
		{
			return detail::as_fp<arg_t, fn>(arg);
		}

		template <typename arg_t>
		static auto as_bool(const arg_t& arg) -> types::boolean
		{
			return detail::as_bool<char_t>(arg);
		}

		template <typename arg_t>
		static auto as_char(const arg_t& arg) -> types::character<char_t>
		{
			if constexpr (::std::same_as<arg_t, types::character<char_t>>)
			{
				return arg;
			}
			else
			{
				return {};
			}
		}

		template <typename arg_t>
		static auto as_string(const arg_t& arg) -> const types::string<char_t>&
		{
			if constexpr (::std::same_as<arg_t, proxy<types::string<char_t>>>)
			{
				return *arg;
			}
			else
			{
				return null_string_val();
			}
		}

		template <typename arg_t>
		static auto as_list(const arg_t& arg) -> const types::list<char_t>&
		{
			if constexpr (::std::same_as<arg_t, proxy<types::list<char_t>>>)
			{
				return *arg;
			}
			else
			{
				return null_list_val();
			}
		}

		template <typename arg_t>
		static auto as_object(const arg_t& arg) -> const types::object<char_t>&
		{
			if constexpr (::std::same_as<arg_t, proxy<types::object<char_t>>>)
			{
				return *arg;
			}
			else
			{
				return null_object_val();
			}
		}

		static auto null_string_val() -> const types::string<char_t>&
		{
			static const auto null_val = types::string<char_t>{};
			return null_val;
		}
		static auto null_list_val() -> const types::list<char_t>&
		{
			static const auto null_val = types::list<char_t>{};
			return null_val;
		}
		static auto null_object_val() -> const types::object<char_t>&
		{
			static const auto null_val = types::object<char_t>{};
			return null_val;
		}
	};
}

namespace pdn::detail
{
	template <typename t>
	struct has_proxy
	{
		static constexpr bool value = false;
	};
	template <typename char_t>
	struct has_proxy<types::string<char_t>>
	{
		static constexpr bool value = true;
	};
	template <typename char_t>
	struct has_proxy<types::list<char_t>>
	{
		static constexpr bool value = true;
	};
	template <typename char_t>
	struct has_proxy<types::object<char_t>>
	{
		static constexpr bool value = true;
	};
	template <typename t>
	inline static constexpr bool has_proxy_v = has_proxy<t>::value;
}

#endif
