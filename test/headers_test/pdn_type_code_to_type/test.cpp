// Tests for pdn/include/pdn/pdn_type_code_to_type.h.
//
// What is pinned down here:
//   * every PDN type code maps to the exact C++ type entity stores for it;
//   * type_code::unknown maps to void through the primary template (no specialization),
//     so "no PDN type" is representable and distinct from every real type;
//   * char_t only takes part in the key for the character/string/list/object families,
//     where it is forwarded into the type; scalars ignore it.

#include <pdn/pdn_type_code_to_type.h>

#include <string>
#include <cstdio>
#include <type_traits>

namespace
{
	using pdn::type_code;
	using pdn::type_code_to_type_t;
	using u8c = pdn::unicode::u8char_t;
	using u16c = pdn::unicode::u16char_t;
	using u32c = pdn::unicode::u32char_t;

	// Scalars: fixed-width aliases from pdn_type_basic.h, independent of char_t.
	static_assert(std::is_same_v<type_code_to_type_t<type_code::i8, u8c>, pdn::type::i8>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::i16, u8c>, pdn::type::i16>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::i32, u8c>, pdn::type::i32>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::i64, u8c>, pdn::type::i64>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::u8, u8c>, pdn::type::u8>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::u16, u8c>, pdn::type::u16>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::u32, u8c>, pdn::type::u32>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::u64, u8c>, pdn::type::u64>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::f32, u8c>, pdn::type::f32>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::f64, u8c>, pdn::type::f64>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::boolean, u8c>, pdn::type::boolean>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::i8, u32c>, pdn::type::i8>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::boolean, u16c>, pdn::type::boolean>);

	// Composites: char_t is forwarded into the type.
	static_assert(std::is_same_v<type_code_to_type_t<type_code::character, u8c>, pdn::type::character<u8c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::character, u16c>, pdn::type::character<u16c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::character, u32c>, pdn::type::character<u32c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::string, u8c>, pdn::type::string<u8c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::string, u16c>, pdn::type::string<u16c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::string, u32c>, pdn::type::string<u32c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::list, u8c>, pdn::type::list<u8c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::list, u16c>, pdn::type::list<u16c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::object, u8c>, pdn::type::object<u8c>>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::object, u32c>, pdn::type::object<u32c>>);

	// The concrete aliases agree with the generic parameters.
	static_assert(std::is_same_v<type_code_to_type_t<type_code::character, u8c>, pdn::type::u8char>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::string, u16c>, pdn::type::u16string>);
	static_assert(std::is_same_v<type_code_to_type_t<type_code::list, u32c>, pdn::type::u32list>);

	// unknown is not a PDN type: the primary template answers void.
	static_assert(std::is_void_v<type_code_to_type_t<type_code::unknown, u8c>>);
	static_assert(std::is_void_v<type_code_to_type_t<type_code::unknown, u16c>>);
	static_assert(!std::is_void_v<type_code_to_type_t<type_code::boolean, u8c>>);

	// The struct form exposes the same member for direct use.
	static_assert(std::is_same_v<pdn::type_code_to_type<type_code::boolean, u8c>::type, pdn::type::boolean>);
	static_assert(std::is_same_v<pdn::type_code_to_type<type_code::object, u16c>::type, pdn::type::object<u16c>>);
}

int main()
{
	// Every check above is a compile-time assertion; running the binary proves they hold.
	std::printf("all checks passed\n");
	return 0;
}
