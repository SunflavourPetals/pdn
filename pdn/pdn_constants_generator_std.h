#ifndef PDN_Header_pdn_constants_generator_std
#define PDN_Header_pdn_constants_generator_std

#include <variant>
#include <numbers>
#include <limits>
#include <unordered_map>

#include "pdn_unicode_base.h"
#include "pdn_code_convert.h"
#include "pdn_types.h"
#include "pdn_token_value_variant.h"

namespace pdn
{
	class constants_table : public ::std::unordered_map<unicode::utf_8_code_unit_string, constant_variant<unicode::utf_8_code_unit_t>>
	{
	public:
		constants_table()
		{
			using namespace literals::unicode_literals;
			using namespace ::std::numbers;

			auto& self = *this;

			self[u8"true"_ucus]          = true;
			self[u8"false"_ucus]         = false;
			self[u8"e"_ucus]             = e;
			self[u8"log2e"_ucus]         = log2e;
			self[u8"log10e"_ucus]        = log10e;
			self[u8"pi"_ucus]            = pi;
			self[u8"inv_pi"_ucus]        = inv_pi;
			self[u8"inv_sqrtpi"_ucus]    = inv_sqrtpi;
			self[u8"ln2"_ucus]           = ln2;
			self[u8"ln10"_ucus]          = ln10;
			self[u8"sqrt2"_ucus]         = sqrt2;
			self[u8"sqrt3"_ucus]         = sqrt3;
			self[u8"inv_sqrt3"_ucus]     = inv_sqrt3;
			self[u8"egamma"_ucus]        = egamma;
			self[u8"phi"_ucus]           = phi;
			self[u8"\u03C0"_ucus]        = pi; // Greek Small Letter Pi
			self[u8"\u03B3"_ucus]        = egamma; // Greek Small Letter Gamma
			self[u8"\u03A6"_ucus]        = phi; // Greek Capital Letter Phi
			self[u8"infinity"_ucus]      = ::std::numeric_limits<types::f64>::infinity();
			self[u8"inf"_ucus]           = ::std::numeric_limits<types::f64>::infinity();
			self[u8"quiet_NaN"_ucus]     = ::std::numeric_limits<types::f64>::quiet_NaN();
			self[u8"qNaN"_ucus]          = ::std::numeric_limits<types::f64>::quiet_NaN();
			self[u8"qnan"_ucus]          = ::std::numeric_limits<types::f64>::quiet_NaN();
			self[u8"NaN"_ucus]           = ::std::numeric_limits<types::f64>::quiet_NaN();
			self[u8"nan"_ucus]           = ::std::numeric_limits<types::f64>::quiet_NaN();
			self[u8"signaling_NaN"_ucus] = ::std::numeric_limits<types::f64>::signaling_NaN();
			self[u8"sNaN"_ucus]          = ::std::numeric_limits<types::f64>::signaling_NaN();
			self[u8"snan"_ucus]          = ::std::numeric_limits<types::f64>::signaling_NaN();
			self[u8"hello"_ucus]         = u8"Hello, world!"_ucus;
		}
		static constants_table& instance()
		{
			static constants_table obj{};
			return obj;
		}
	};

	template <typename char_t>
	inline bool constants_generator_std_function(const unicode::utf_8_code_unit_string& s, constant_variant<char_t>& r)
	{
		if (auto result = constants_table::instance().find(s); result != constants_table::instance().end())
		{
			::std::visit([&](const auto& arg)
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				if constexpr (::std::same_as<arg_t, types::string<unicode::utf_8_code_unit_t>>)
				{
					using str = types::string<char_t>;
					r = unicode::code_convert<str>(arg);
				}
				else if constexpr (::std::same_as<arg_t, types::character<unicode::utf_8_code_unit_t>>)
				{
					using str = types::string<char_t>;
					using cha = types::character<char_t>;
					using u8_sv = ::std::basic_string_view<unicode::utf_8_code_unit_t>;

					auto cha_str = unicode::code_convert<str>(u8_sv{ arg.data(), arg.size() });
					r = cha{ cha_str.begin(), cha_str.size() };
				}
				else
				{
					r = arg;
				}
			}, result->second);
			return true;
		}

		return false;
	}
}

#endif
