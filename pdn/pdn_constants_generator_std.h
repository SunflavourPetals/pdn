#ifndef PDN_Header_pdn_constants_generator_std
#define PDN_Header_pdn_constants_generator_std

#include <cstddef>
#include <variant>
#include <numbers>
#include <limits>
#include <optional>
#include <functional>
#include <unordered_map>

#include "pdn_unicode_base.h"
#include "pdn_code_convert.h"
#include "pdn_types.h"
#include "pdn_token_value_variant.h"
#include "pdn_constants_variant.h"

namespace pdn::dev_util
{
	struct constants_table_key_hasher
	{
		using is_transparent = void; // Enables heterogeneous operations.
		using string_view = unicode::utf_8_code_unit_string_view;
		auto operator()(string_view sv) const -> ::std::size_t
		{
			return ::std::hash<string_view>{}(sv);
		}
	};
	using constants_table_base = ::std::unordered_map<
		unicode::utf_8_code_unit_string,
		constant_variant<unicode::utf_8_code_unit_t>,
		constants_table_key_hasher,
		::std::equal_to<>>;
}

namespace pdn
{
	class constants_table : public dev_util::constants_table_base
	{
	public:
		constants_table()
		{
			using namespace literals::unicode_literals;
			using namespace ::std::numbers;

			auto& self = *this;

			constexpr auto f64_inf  = ::std::numeric_limits<types::f64>::infinity();
			constexpr auto f64_qNaN = ::std::numeric_limits<types::f64>::quiet_NaN();
			constexpr auto f64_sNaN = ::std::numeric_limits<types::f64>::signaling_NaN();

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
			self[u8"\u03C0"_ucus]        = pi;     // Greek Small Letter Pi
			self[u8"\u03B3"_ucus]        = egamma; // Greek Small Letter Gamma
			self[u8"\u03A6"_ucus]        = phi;    // Greek Capital Letter Phi
			self[u8"infinity"_ucus]      = f64_inf;
			self[u8"inf"_ucus]           = f64_inf;
			self[u8"quiet_NaN"_ucus]     = f64_qNaN;
			self[u8"qNaN"_ucus]          = f64_qNaN;
			self[u8"qnan"_ucus]          = f64_qNaN;
			self[u8"NaN"_ucus]           = f64_qNaN;
			self[u8"nan"_ucus]           = f64_qNaN;
			self[u8"signaling_NaN"_ucus] = f64_sNaN;
			self[u8"sNaN"_ucus]          = f64_sNaN;
			self[u8"snan"_ucus]          = f64_sNaN;
			self[u8"hello"_ucus]         = u8"Hello, world!"_ucus;
		}
		static auto instance() -> const constants_table&
		{
			static constants_table obj{};
			return obj;
		}
	};

	template <typename char_t>
	inline auto constants_generator_std_function(const unicode::utf_8_code_unit_string& s) -> ::std::optional<constant_variant<char_t>>
	{
		if (auto result = constants_table::instance().find(s); result != constants_table::instance().end())
		{
			return ::std::visit([&](const auto& arg) -> ::std::optional<constant_variant<char_t>>
			{
				using arg_t = ::std::decay_t<decltype(arg)>;
				if constexpr (::std::same_as<arg_t, types::string<unicode::utf_8_code_unit_t>>)
				{
					using str = types::string<char_t>;
					return unicode::code_convert<str>(arg);
				}
				else if constexpr (::std::same_as<arg_t, types::character<unicode::utf_8_code_unit_t>>)
				{
					using str = types::string<char_t>;
					using cha = types::character<char_t>;
					using u8_sv = ::std::basic_string_view<unicode::utf_8_code_unit_t>;

					auto cha_str = unicode::code_convert<str>(u8_sv{ arg.data(), arg.size() });
					return cha{ cha_str.begin(), cha_str.size() };
				}
				else
				{
					return arg;
				}
				return ::std::nullopt;
			}, result->second);
		}
		return ::std::nullopt;
	}
}

#endif
