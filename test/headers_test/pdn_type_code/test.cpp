// Review test for pdn_type_code.h.
// type_code is the runtime name of a PDN type. The checks below pin the enumerator set, the
// round trip through both mapping tables, and the names used in error messages.

#include <pdn/pdn_type_code.h>
#include <pdn/pdn_type_code_to_error_msg_string.h>
#include <pdn/pdn_type_code_to_type.h>
#include <pdn/pdn_type_to_type_code.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace
{
	using pdn::type_code;
	using namespace pdn::error_message_literals;

	// Order follows pdn-reference.md: the 15 PDN types plus unknown.
	constexpr ::std::array<type_code, 16> all_codes{
		type_code::unknown,
		type_code::i8, type_code::i16, type_code::i32, type_code::i64,
		type_code::u8, type_code::u16, type_code::u32, type_code::u64,
		type_code::f32, type_code::f64,
		type_code::boolean, type_code::character, type_code::string, type_code::list, type_code::object,
	};
	static_assert(all_codes.size() == 16);

	// Every code has a non-empty name, and no two codes share one.
	constexpr bool names_are_distinct()
	{
		for (::std::size_t i = 0; i < all_codes.size(); ++i)
		{
			if (pdn::type_code_to_error_msg_string_view(all_codes[i]).empty()) return false;
			for (::std::size_t j = i + 1; j < all_codes.size(); ++j)
			{
				if (pdn::type_code_to_error_msg_string_view(all_codes[i]) ==
				    pdn::type_code_to_error_msg_string_view(all_codes[j])) return false;
			}
		}
		return true;
	}
	static_assert(names_are_distinct());

	// Spot check: the spelling is the PDN type name, not the C++ one.
	static_assert(pdn::type_code_to_error_msg_string_view(type_code::boolean) == u8"boolean"_emv);
	static_assert(pdn::type_code_to_error_msg_string_view(type_code::character) == u8"character"_emv);
	static_assert(pdn::type_code_to_error_msg_string_view(type_code::unknown) == u8"unknown"_emv);
	static_assert(pdn::type_code_to_error_msg_string(type_code::u64) == u8"u64"_em);

	// unknown is not a PDN type: it maps to void, and void maps back to unknown.
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::unknown, char8_t>, void>);
	static_assert(pdn::type_to_type_code_v<void, char8_t> == type_code::unknown);
	// Same fallback for any type PDN does not have. Note that double is not one of them: PDN's
	// f64 is spelled double on every supported platform.
	static_assert(pdn::type_to_type_code_v<long double, char8_t> == type_code::unknown);
	static_assert(pdn::type_to_type_code_v<double, char8_t> == type_code::f64);

	// Round trip: type -> code -> type, for all 15 real types.
	template <type_code c>
	constexpr bool round_trips()
	{
		return pdn::type_to_type_code_v<pdn::type_code_to_type_t<c, char8_t>, char8_t> == c;
	}
	static_assert(round_trips<type_code::i8>());
	static_assert(round_trips<type_code::i16>());
	static_assert(round_trips<type_code::i32>());
	static_assert(round_trips<type_code::i64>());
	static_assert(round_trips<type_code::u8>());
	static_assert(round_trips<type_code::u16>());
	static_assert(round_trips<type_code::u32>());
	static_assert(round_trips<type_code::u64>());
	static_assert(round_trips<type_code::f32>());
	static_assert(round_trips<type_code::f64>());
	static_assert(round_trips<type_code::boolean>());
	static_assert(round_trips<type_code::character>());
	static_assert(round_trips<type_code::string>());
	static_assert(round_trips<type_code::list>());
	static_assert(round_trips<type_code::object>());

	// The scalar codes really name integers of the requested width.
	static_assert(sizeof(pdn::type_code_to_type_t<type_code::i8, char8_t>) == 1);
	static_assert(sizeof(pdn::type_code_to_type_t<type_code::i16, char8_t>) == 2);
	static_assert(sizeof(pdn::type_code_to_type_t<type_code::i32, char8_t>) == 4);
	static_assert(sizeof(pdn::type_code_to_type_t<type_code::i64, char8_t>) == 8);
	static_assert(sizeof(pdn::type_code_to_type_t<type_code::u64, char8_t>) == 8);
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::f32, char8_t>, float>);
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::f64, char8_t>, double>);
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::boolean, char8_t>, bool>);

	// The container codes are templated on the code unit type.
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::character, char8_t>,
	                             pdn::type::character<char8_t>>);
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::string, char8_t>,
	                             pdn::type::string<char8_t>>);
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::list, char8_t>,
	                             pdn::type::list<char8_t>>);
	static_assert(::std::same_as<pdn::type_code_to_type_t<type_code::object, char8_t>,
	                             pdn::type::object<char8_t>>);
}

int main()
{
	return 0;
}
