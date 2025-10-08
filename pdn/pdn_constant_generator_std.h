#ifndef PDN_Header_pdn_constant_generator_std
#define PDN_Header_pdn_constant_generator_std

#include <cstddef>
#include <variant>
#include <numbers>
#include <limits>
#include <optional>
#include <functional>
#include <unordered_map>

#include "pdn_unicode_base.h"
#include "pdn_types.h"
#include "pdn_proxy.h"
#include "pdn_token_value_variant.h"
#include "pdn_entity.h"

namespace pdn::dev_util
{
	struct constant_table_key_hasher
	{
		using is_transparent = void; // enables heterogeneous operations.
		using string_view = unicode::u8string_view;
		auto operator()(string_view sv) const -> ::std::size_t
		{
			return ::std::hash<string_view>{}(sv);
		}
	};
	template <typename char_t>
	using constant_table_base = ::std::unordered_map<
		unicode::u8string,
		entity<char_t>,
		constant_table_key_hasher,
		::std::equal_to<>>;

	template <typename char_t>
	inline auto test_string_hello() -> entity<char_t>
	{
		using c = char_t;
		auto s = types::string<char_t>
		{
			c('H'), c('e'), c('l'), c('l'), c('o'), c(','), c(' '),
			c('w'), c('o'), c('r'), c('l'), c('d'), c('!')
		};
		return make_proxy<types::string<char_t>>(::std::move(s));
	}

	template <typename char_t>
	inline auto test_list_fib_10() -> entity<char_t>
	{
		// 1, 1, 2, 3, 5, 8, 13, 21, 34, 55
		auto list = types::list<char_t>{};
		list.push_back(1);
		list.push_back(1);
		list.push_back(2);
		list.push_back(3);
		list.push_back(5);
		list.push_back(8);
		list.push_back(13);
		list.push_back(21);
		list.push_back(34);
		list.push_back(55);
		return make_proxy<types::list<char_t>>(::std::move(list));
	}

	template <typename char_t>
	inline auto test_object_me() -> entity<char_t>
	{
		using c = char_t;
		using s = types::string<c>;

		auto date = types::object<char_t>{};
		date[s{ c('y') }] = 2024;
		date[s{ c('m') }] = 12;
		date[s{ c('d') }] = 21;

		auto root = types::object<char_t>{};
		root[s{ c('n'), c('a'), c('m'), c('e') }] = make_proxy<types::string<char_t>>(s{ c('P'), c('D'), c('N') });
		root[s{ c('d'), c('a'), c('t'), c('e') }] = make_proxy<types::object<char_t>>(::std::move(date));

		return make_proxy<types::object<char_t>>(::std::move(root));
	}
}

namespace pdn
{
	template <typename char_t>
	class constant_table : public dev_util::constant_table_base<char_t>
	{
	public:
		constant_table()
		{
			using namespace literals::unicode_literals;
			using namespace ::std::numbers;

			auto& self = *this;

			constexpr auto f64_inf  = ::std::numeric_limits<types::f64>::infinity();
			constexpr auto f64_qNaN = ::std::numeric_limits<types::f64>::quiet_NaN();
			constexpr auto f64_sNaN = ::std::numeric_limits<types::f64>::signaling_NaN();

			self[u8"true"_s]          = true;
			self[u8"false"_s]         = false;
			self[u8"e"_s]             = e;
			self[u8"log2e"_s]         = log2e;
			self[u8"log10e"_s]        = log10e;
			self[u8"pi"_s]            = pi;
			self[u8"inv_pi"_s]        = inv_pi;
			self[u8"inv_sqrtpi"_s]    = inv_sqrtpi;
			self[u8"ln2"_s]           = ln2;
			self[u8"ln10"_s]          = ln10;
			self[u8"sqrt2"_s]         = sqrt2;
			self[u8"sqrt3"_s]         = sqrt3;
			self[u8"inv_sqrt3"_s]     = inv_sqrt3;
			self[u8"egamma"_s]        = egamma;
			self[u8"phi"_s]           = phi;
			self[u8"\u03C0"_s]        = pi;     // Greek Small Letter Pi,    not std
			self[u8"\u03B3"_s]        = egamma; // Greek Small Letter Gamma, not std
			self[u8"\u03A6"_s]        = phi;    // Greek Capital Letter Phi, not std
			self[u8"infinity"_s]      = f64_inf;
			self[u8"inf"_s]           = f64_inf;
			self[u8"quiet_NaN"_s]     = f64_qNaN;
			self[u8"qNaN"_s]          = f64_qNaN;
			self[u8"qnan"_s]          = f64_qNaN;
			self[u8"NaN"_s]           = f64_qNaN;
			self[u8"nan"_s]           = f64_qNaN;
			self[u8"signaling_NaN"_s] = f64_sNaN;
			self[u8"sNaN"_s]          = f64_sNaN;
			self[u8"snan"_s]          = f64_sNaN;
			self[u8"hello"_s]         = dev_util::test_string_hello<char_t>(); // not std
			self[u8"fib_10_list"_s]   = dev_util::test_list_fib_10<char_t>();  // not std
			self[u8"me_object"_s]     = dev_util::test_object_me<char_t>();    // not std
		}
		static auto instance() -> const constant_table<char_t>&
		{
			static constant_table<char_t> obj{};
			return obj;
		}
	};

	template <typename char_t>
	inline auto constant_generator_std_function(const unicode::u8string& s) -> ::std::optional<entity<char_t>>
	{
		if (auto result = constant_table<char_t>::instance().find(s); result != constant_table<char_t>::instance().end())
		{
			return ::std::make_optional<entity<char_t>>(result->second);
		}
		return ::std::nullopt;
	}
}

#endif
