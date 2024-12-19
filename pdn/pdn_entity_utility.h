#ifndef PDN_Header_pdn_entity_utility
#define PDN_Header_pdn_entity_utility

#include <variant>

#include "pdn_types.h"
#include "pdn_proxy.h"

#include "pdn_entity_forward_decl.h"

namespace pdn::types::dev_util
{
	template <typename char_t>
	using entity_variant = ::std::variant<
		i8, i16, i32, i64,
		u8, u16, u32, u64,
		f32, f64,
		boolean,
		character<char_t>,
		proxy<string<char_t>>,
		proxy<list<char_t>>,
		proxy<object<char_t>>>;
}

namespace pdn::dev_util
{
	template <typename arg_t>
	inline auto as_int(const arg_t& arg) -> types::i64
	{
		using namespace types::concepts;
		using namespace types;
		if constexpr (pdn_sint<arg_t> || pdn_bool<arg_t>)
		{
			return arg;
		}
		else if constexpr (pdn_fp<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<i64>::max();
			constexpr auto min = ::std::numeric_limits<i64>::min();
			if (arg >= arg_t(max)) return max;
			if (arg <= arg_t(min)) return min;
			if (::std::isnan(arg)) return 0;
			return arg;
		}
		else if constexpr (pdn_uint<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<i64>::max();
			return arg > u64(max) ? max : arg;
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t>
	inline auto as_uint(const arg_t& arg) -> types::u64
	{
		using namespace types::concepts;
		using namespace types;
		if constexpr (pdn_uint<arg_t> || pdn_bool<arg_t>)
		{
			return arg;
		}
		else if constexpr (pdn_sint<arg_t>)
		{
			return arg < 0 ? 0 : arg;
		}
		else if constexpr (pdn_fp<arg_t>)
		{
			if (constexpr auto max = ::std::numeric_limits<u64>::max(); arg > arg_t(max)) return max;
			if (::std::isnan(arg)) return 0;
			return arg < 0 ? 0 : arg;
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t>
	inline auto as_fp(const arg_t& arg) -> types::f64
	{
		using namespace types::concepts;
		if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
		{
			return arg;
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t>
	inline auto as_bool(const arg_t& arg) -> types::boolean
	{
		using namespace types::concepts;
		if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
		{
			return arg;
		}
		else
		{
			return false;
		}
	}

	template <typename char_t>
	struct as_accessor
	{
		template <typename arg_t>
		static auto as_int(const arg_t& arg) -> types::i64
		{
			return dev_util::as_int(arg);
		}

		template <typename arg_t>
		static auto as_uint(const arg_t& arg) -> types::u64
		{
			return dev_util::as_uint(arg);
		}

		template <typename arg_t>
		static auto as_fp(const arg_t& arg) -> types::f64
		{
			return dev_util::as_fp(arg);
		}

		template <typename arg_t>
		static auto as_bool(const arg_t& arg) -> types::boolean
		{
			return dev_util::as_bool(arg);
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

namespace pdn::dev_util
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
