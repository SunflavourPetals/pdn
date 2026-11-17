// tests for pdn_entity_utility.h: the as_* conversion matrix, the tag machinery and the
// return type table used by entity's accessors
#include <pdn/spdn.h>

#include <cmath>
#include <cstdio>
#include <limits>
#include <type_traits>
#include <variant>

using namespace pdn;

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { ++fails; std::printf("FAILED: %s  (line %d)\n", #cond, __LINE__); } } while (0)

using u8s  = pdn::type::u8string;
using u16s = pdn::type::u16string;
using u8c  = pdn::unicode::u8char_t;

// ---------------------------------------------------------------- static checks
// the storage alternatives: 15 of them, and the first one is what a value-initialized entity holds
static_assert(std::variant_size_v<pdn::type::detail::entity_variant<u8c>> == 15);
static_assert(std::is_same_v<std::variant_alternative_t<0,  pdn::type::detail::entity_variant<u8c>>, pdn::type::i32>);
static_assert(std::is_same_v<std::variant_alternative_t<12, pdn::type::detail::entity_variant<u8c>>, pdn::proxy<u8s>>);
static_assert(std::is_same_v<std::variant_alternative_t<14, pdn::type::detail::entity_variant<u8c>>, pdn::proxy<pdn::type::u8object>>);

// has_proxy marks exactly the three proxied alternatives
static_assert(pdn::detail::has_proxy_v<u8s>);
static_assert(pdn::detail::has_proxy_v<pdn::type::u16list>);
static_assert(pdn::detail::has_proxy_v<pdn::type::u32object>);
static_assert(!pdn::detail::has_proxy_v<pdn::type::i32>);
static_assert(!pdn::detail::has_proxy_v<pdn::type::character<u8c>>);
static_assert(!pdn::detail::has_proxy_v<pdn::proxy<u8s>>);

// tag families and the tag-or-type forms
static_assert(pdn::detail::sint_tag<pdn::detail::auto_int_tag_t>);
static_assert(pdn::detail::sint_tag<pdn::detail::i64_tag_t>);
static_assert(!pdn::detail::sint_tag<pdn::detail::u8_tag_t>);
static_assert(pdn::detail::sint_or_tag<pdn::type::i8>);
static_assert(!pdn::detail::sint_or_tag<pdn::type::u8>);
static_assert(pdn::detail::uint_or_tag<pdn::type::u64>);
static_assert(pdn::detail::fp_or_tag<pdn::type::f32>);
static_assert(!pdn::detail::fp_or_tag<pdn::type::i32>);
static_assert(pdn::detail::char_series_tag<pdn::detail::u32char_tag_t>);
static_assert(pdn::detail::string_series_tag<pdn::detail::u16string_tag_t>);
static_assert(pdn::detail::type_tag<pdn::detail::object_tag_t>);
static_assert(pdn::detail::type_tag<pdn::detail::auto_uint_tag_t>);
static_assert(!pdn::detail::type_tag<pdn::type::i8>);
static_assert(!pdn::detail::type_tag<int>);

// as_tparam accepts tags, scalars, character<u8c> and u8string; as_tparam_pure also the concrete
// container spellings
static_assert(pdn::detail::as_tparam<pdn::detail::i8_tag_t>);
static_assert(pdn::detail::as_tparam<pdn::type::i8>);
static_assert(pdn::detail::as_tparam<pdn::type::u16char>);
static_assert(!pdn::detail::as_tparam<pdn::type::u16string>);
static_assert(pdn::detail::as_tparam_pure<pdn::type::u16string, pdn::unicode::u16char_t>);
static_assert(pdn::detail::as_tparam_pure<pdn::type::u8list, u8c>);
static_assert(pdn::detail::as_tparam_pure<pdn::type::u8object, u8c>);
static_assert(!pdn::detail::as_tparam_pure<void, u8c>);

// tag_to_type: the mapping of the tags, and the identity for plain types
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::detail::auto_int_tag_t, u8c>, pdn::type::i32>);
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::detail::auto_uint_tag_t, u8c>, pdn::type::u32>);
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::detail::character_tag_t, pdn::unicode::u16char_t>, pdn::type::character<pdn::unicode::u16char_t>>);
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::detail::string_tag_t, pdn::unicode::u16char_t>, pdn::type::u16string>);
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::detail::u8string_tag_t, pdn::unicode::u16char_t>, u8s>);
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::detail::list_tag_t, u8c>, pdn::type::u8list>);
static_assert(std::is_same_v<pdn::detail::tag_to_type_t<pdn::type::i8, u8c>, pdn::type::i8>);

// as_rttype: by value for scalars, by const reference for the containers the entity owns, and by
// value for a string that has to be converted
static_assert(std::is_same_v<pdn::detail::as_rttype_t<pdn::type::i8, u8c>, pdn::type::i8>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<pdn::type::character<u8c>, u8c>, pdn::type::character<u8c>>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<pdn::detail::string_tag_t, u8c>, const u8s&>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<u8s, u8c>, const u8s&>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<u8s, pdn::unicode::u16char_t>, u8s>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<u16s, pdn::unicode::u16char_t>, const u16s&>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<pdn::type::u8list, u8c>, const pdn::type::u8list&>);
static_assert(std::is_same_v<pdn::detail::as_rttype_t<pdn::type::u8object, u8c>, const pdn::type::u8object&>);

int main()
{
    // ------------------------------------------------------ the detail level clamps
    CHECK(pdn::detail::as_int(pdn::type::i8{ 42 }) == 42);
    CHECK(pdn::detail::as_int(pdn::type::u64{ std::numeric_limits<pdn::type::u64>::max() }) == std::numeric_limits<pdn::type::i64>::max());
    CHECK(pdn::detail::as_int(pdn::type::i64{ std::numeric_limits<pdn::type::i64>::min() }) == std::numeric_limits<pdn::type::i64>::min());
    CHECK(pdn::detail::as_int(true) == 1);
    CHECK(pdn::detail::as_int(pdn::type::f64{ 1.0e300 }) == std::numeric_limits<pdn::type::i64>::max());
    CHECK(pdn::detail::as_int(pdn::type::f64{ -1.0e300 }) == std::numeric_limits<pdn::type::i64>::min());
    CHECK(pdn::detail::as_int(pdn::type::f64{ std::numeric_limits<double>::quiet_NaN() }) == 0);
    CHECK(pdn::detail::as_int(pdn::type::f64{ std::numeric_limits<double>::infinity() }) == std::numeric_limits<pdn::type::i64>::max());
    CHECK(pdn::detail::as_int(pdn::type::f64{ -std::numeric_limits<double>::infinity() }) == std::numeric_limits<pdn::type::i64>::min());
    CHECK(pdn::detail::as_int(pdn::type::f64{ -0.9 }) == 0);
    CHECK(pdn::detail::as_int(pdn::type::character<u8c>{}) == 0);
    const auto some_string = *pdn::make_proxy<u8s>(u8"7");
    CHECK(pdn::detail::as_int(some_string) == 0);
    CHECK((pdn::detail::as_int<pdn::type::i64, pdn::type::i8>(pdn::type::i64{ 300 }) == 127));
    CHECK((pdn::detail::as_int<pdn::type::i64, pdn::type::i8>(pdn::type::i64{ -300 }) == -128));

    CHECK(pdn::detail::as_uint(pdn::type::i64{ -1 }) == 0);
    CHECK(pdn::detail::as_uint(pdn::type::i8{ -128 }) == 0);
    CHECK(pdn::detail::as_uint(pdn::type::u8{ 200 }) == 200);
    CHECK(pdn::detail::as_uint(pdn::type::f64{ -2.5 }) == 0);
    CHECK(pdn::detail::as_uint(pdn::type::f64{ std::numeric_limits<double>::quiet_NaN() }) == 0);
    CHECK(pdn::detail::as_uint(pdn::type::f64{ 1.0e300 }) == std::numeric_limits<pdn::type::u64>::max());
    CHECK(pdn::detail::as_uint(true) == 1);
    CHECK((pdn::detail::as_uint<pdn::type::i64, pdn::type::u8>(pdn::type::i64{ 300 }) == 255));

    CHECK(pdn::detail::as_fp(pdn::type::i64{ -3 }) == -3.0);
    CHECK(pdn::detail::as_fp(pdn::type::u64{ std::numeric_limits<pdn::type::u64>::max() }) > 1.8e19);
    CHECK(pdn::detail::as_fp(true) == 1.0);
    CHECK(pdn::detail::as_fp(pdn::type::f32{ 0.5f }) == 0.5);
    CHECK(pdn::detail::as_fp(pdn::type::character<u8c>{}) == 0.0);

    CHECK(pdn::detail::as_bool<u8c>(pdn::type::i8{ 0 }) == false);
    CHECK(pdn::detail::as_bool<u8c>(pdn::type::i8{ -1 }) == true);
    CHECK(pdn::detail::as_bool<u8c>(pdn::type::f64{ 0.0 }) == false);
    CHECK(pdn::detail::as_bool<u8c>(pdn::type::f64{ std::numeric_limits<double>::quiet_NaN() }) == true);
    CHECK(pdn::detail::as_bool<u8c>(pdn::type::character<u8c>{}) == false);
    const pdn::type::character<u8c> the_char{ u8"a", 1 };
    CHECK(pdn::detail::as_bool<u8c>(the_char) == true);

    // the shared empty values behind a type mismatch: same object every time, never null
    const auto& empty_string = pdn::detail::as_accessor<u8c>::null_string_val();
    CHECK(empty_string.empty());
    CHECK(&empty_string == &pdn::detail::as_accessor<u8c>::as_string(pdn::type::i8{ 1 }));
    CHECK(&pdn::detail::as_accessor<u8c>::as_list(1) == &pdn::detail::as_accessor<u8c>::null_list_val());
    CHECK(&pdn::detail::as_accessor<u8c>::as_object(1) == &pdn::detail::as_accessor<u8c>::null_object_val());
    CHECK(pdn::detail::as_accessor<u8c>::as_char(pdn::type::i8{ 1 }) == pdn::type::character<u8c>{});
    CHECK(pdn::detail::as_accessor<u8c>::as_char(the_char) == the_char);

    // ------------------------------------------------------ through an entity
    auto doc = pdn::parse(std::u8string_view{ u8"a 1\nb -2\nc 3.5\nd @true\ne 'x'\nf \"s\"\ng [1]\nh {k 1}\ni 300\nj '\\0'\n" }, pdn::utf8_tag);
    CHECK(doc);
    if (!doc)
    {
        std::printf("parse failed: %d\n", static_cast<int>(doc.error_code()));
        return 1;
    }
    const auto& obj = pdn::as_object(doc.value());

    auto get = [&obj](pdn::unicode::u8string_view key) -> const pdn::entity<u8c>&
    {
        auto it = obj.find(key);
        return it->second;
    };
    const auto& e_a = get(u8"a");
    const auto& e_b = get(u8"b");
    const auto& e_c = get(u8"c");
    const auto& e_d = get(u8"d");
    const auto& e_e = get(u8"e");
    const auto& e_f = get(u8"f");
    const auto& e_g = get(u8"g");
    const auto& e_h = get(u8"h");
    const auto& e_i = get(u8"i");
    const auto& e_j = get(u8"j");

    CHECK(pdn::as_int(e_a) == 1);
    CHECK(pdn::as_int(e_b) == -2);
    CHECK(pdn::as_int(e_c) == 3);
    CHECK(pdn::as_int(e_d) == 1);
    CHECK(pdn::as_int(e_e) == 0);
    CHECK(pdn::as_int(e_f) == 0);
    CHECK(pdn::as_int(e_g) == 0);
    CHECK(pdn::as_int(e_h) == 0);
    CHECK(pdn::as_int(e_i, pdn::i8_tag) == 127);
    CHECK(pdn::as_uint(e_b) == 0);
    CHECK(pdn::as_uint(e_i, pdn::u8_tag) == 255);
    CHECK(pdn::as_fp(e_a) == 1.0);
    CHECK(pdn::as_fp(e_d) == 1.0);
    CHECK(pdn::as_fp(e_e) == 0.0);
    CHECK(pdn::as_bool(e_a) == true);
    CHECK(pdn::as_bool(e_e) == true);
    CHECK(pdn::as_bool(e_j) == false);
    CHECK(pdn::as_bool(e_f) == false);
    CHECK(pdn::as_bool(e_g) == false);
    CHECK(pdn::as_char(e_e) == pdn::type::character<u8c>(u8"x", 1));
    CHECK(pdn::as_char(e_a) == pdn::type::character<u8c>{});
    CHECK(pdn::as_string(e_f) == u8"s");
    CHECK(pdn::as_string(e_a).empty());
    CHECK(pdn::as_list(e_g).size() == 1);
    CHECK(pdn::as_list(e_a).empty());
    CHECK(pdn::as_object(e_h).contains(u8"k"));
    CHECK(pdn::as_object(e_a).empty());

    // as<T> and as<T>(tag) go through the same table
    CHECK(pdn::as<pdn::type::i16>(e_a) == 1);
    CHECK(pdn::as<pdn::type::u8string>(e_f) == u8"s");
    CHECK((pdn::as<pdn::detail::i8_tag_t>(e_i) == 127));

    // a value-initialized entity holds the first alternative, i32 0
    pdn::entity<u8c> default_entity{};
    CHECK(default_entity.index() == 0);
    CHECK(pdn::type_of(default_entity) == pdn::type_code::i32);
    CHECK(pdn::as_int(default_entity) == 0);

    // the proxied types are reachable through get<T> (which unwraps the proxy via has_proxy)
    CHECK(pdn::get<u8s>(e_f) == u8"s");
    CHECK(pdn::get_if<u8s>(e_f) != nullptr);
    CHECK(pdn::get_if<u8s>(e_a) == nullptr);

    std::printf(fails == 0 ? "pdn_entity_utility ok\n" : "pdn_entity_utility failed: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
