// Tests for pdn/include/pdn/pdn_type.h, without including pdn_entity.h.
//
// pdn_type.h is the facade that names the library's types in pdn::type. This translation unit
// pins what the names are and that entity stays forward declared, so the header can be used
// before the entity definition exists (pdn_entity.h includes pdn_type.h, not the other way
// around).

#include <pdn/pdn_type.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename t>
concept complete = requires { sizeof(t); };

static_assert(!complete<pdn::entity<char8_t>>, "pdn_type.h must not pull in the entity definition");

// The container types themselves are complete even with a forward declared element type.
static_assert(complete<pdn::type::u8list>);
static_assert(complete<pdn::type::u8object>);

static_assert(std::same_as<pdn::type::string<char8_t>, std::u8string>);
static_assert(std::same_as<pdn::type::string<char16_t>, std::u16string>);
static_assert(std::same_as<pdn::type::string<char32_t>, std::u32string>);

static_assert(std::same_as<pdn::type::list<char8_t>, std::vector<pdn::entity<char8_t>>>);
static_assert(std::same_as<
	pdn::type::object<char8_t>,
	::std::unordered_map<std::u8string, pdn::entity<char8_t>, pdn::type::config::key_hasher, ::std::equal_to<>>>);

static_assert(std::same_as<pdn::type::u8char, pdn::type::character<pdn::unicode::u8char_t>>);
static_assert(std::same_as<pdn::type::u16char, pdn::type::character<pdn::unicode::u16char_t>>);
static_assert(std::same_as<pdn::type::u32char, pdn::type::character<pdn::unicode::u32char_t>>);
static_assert(pdn::type::u8char::max_size() == 4);
static_assert(pdn::type::u16char::max_size() == 2);
static_assert(pdn::type::u32char::max_size() == 1);

static_assert(std::same_as<pdn::type::u8string, std::u8string>);
static_assert(std::same_as<pdn::type::u16string, std::u16string>);
static_assert(std::same_as<pdn::type::u32string, std::u32string>);
static_assert(std::same_as<pdn::type::u8list, pdn::type::list<pdn::unicode::u8char_t>>);
static_assert(std::same_as<pdn::type::u16list, pdn::type::list<pdn::unicode::u16char_t>>);
static_assert(std::same_as<pdn::type::u32list, pdn::type::list<pdn::unicode::u32char_t>>);
static_assert(std::same_as<pdn::type::u8object, pdn::type::object<pdn::unicode::u8char_t>>);
static_assert(std::same_as<pdn::type::u16object, pdn::type::object<pdn::unicode::u16char_t>>);
static_assert(std::same_as<pdn::type::u32object, pdn::type::object<pdn::unicode::u32char_t>>);

static_assert(std::same_as<pdn::type::i8, std::int8_t>);
static_assert(std::same_as<pdn::type::u64, std::uint64_t>);
static_assert(std::same_as<pdn::type::f32, float>);
static_assert(std::same_as<pdn::type::f64, double>);
static_assert(std::same_as<pdn::type::boolean, bool>);
static_assert(std::same_as<pdn::type::auto_int, pdn::type::i32> || std::same_as<pdn::type::auto_int, int>);

int main()
{
	std::printf("all checks passed\n");
	return 0;
}
