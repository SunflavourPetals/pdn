#ifndef PDN_Header_pdn_types_basic
#define PDN_Header_pdn_types_basic

#include "pdn_types_config.h"

namespace pdn::types
{
	using config::i8;
	using config::i16;
	using config::i32;
	using config::i64;
	using config::u8;
	using config::u16;
	using config::u32;
	using config::u64;
	using config::f32;
	using config::f64;
	using config::boolean;
	using config::character;
}

namespace pdn::types::dev_util
{
	using ::std::same_as;

	template <typename     int_t> struct alias_sint        { using type = void; };
	template <same_as<i8>  int_t> struct alias_sint<int_t> { using type = i8;   };
	template <same_as<i16> int_t> struct alias_sint<int_t> { using type = i16;  };
	template <same_as<i32> int_t> struct alias_sint<int_t> { using type = i32;  };
	template <same_as<i64> int_t> struct alias_sint<int_t> { using type = i64;  };
	template <typename     int_t> using  alias_sint_t  =  alias_sint<int_t>::type;
	template <typename     int_t> struct alias_uint        { using type = void; };
	template <same_as<u8>  int_t> struct alias_uint<int_t> { using type = u8;   };
	template <same_as<u16> int_t> struct alias_uint<int_t> { using type = u16;  };
	template <same_as<u32> int_t> struct alias_uint<int_t> { using type = u32;  };
	template <same_as<u64> int_t> struct alias_uint<int_t> { using type = u64;  };
	template <typename     int_t> using  alias_uint_t  =  alias_uint<int_t>::type;

	template <typename int_t>
	inline constexpr bool sint_has_alias_v = !same_as<alias_sint_t<int_t>, void>;
	template <typename int_t>
	inline constexpr bool uint_has_alias_v = !same_as<alias_uint_t<int_t>, void>;

	template <typename int_t>
	using simu_int_t  = ::std::conditional_t<sint_has_alias_v<int_t>, alias_sint_t<int_t>, i32>;
	template <typename int_t>
	using simu_uint_t = ::std::conditional_t<uint_has_alias_v<int_t>, alias_uint_t<int_t>, u32>;
}

namespace pdn::types
{
	using auto_int  = dev_util::simu_int_t <int>;
	using auto_uint = dev_util::simu_uint_t<unsigned int>;
}

namespace pdn::types::concepts
{
	template <typename int_t>
	concept pdn_sint
		 = ::std::same_as<int_t, i8>
		|| ::std::same_as<int_t, i16>
		|| ::std::same_as<int_t, i32>
		|| ::std::same_as<int_t, i64>;

	template <typename int_t>
	concept pdn_uint
		 = ::std::same_as<int_t, u8>
		|| ::std::same_as<int_t, u16>
		|| ::std::same_as<int_t, u32>
		|| ::std::same_as<int_t, u64>;

	template <typename t>
	concept pdn_integral = pdn_sint<t> || pdn_uint<t>;

	template <typename fp_t>
	concept pdn_fp
		 = ::std::same_as<fp_t, f32>
		|| ::std::same_as<fp_t, f64>;

	template <typename bool_t>
	concept pdn_bool = ::std::same_as<bool_t, boolean>;

	template <typename t, typename char_t>
	concept basic_types
		 = concepts::pdn_integral<t>
		|| concepts::pdn_fp<t>
		|| concepts::pdn_bool<t>
		|| ::std::same_as<t, character<char_t>>;
}

#endif
