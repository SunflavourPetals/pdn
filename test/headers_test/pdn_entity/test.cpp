// Tests for pdn/include/pdn/pdn_entity.h.
#include <cstdio>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <pdn/spdn.h>

namespace un = pdn::unicode;
namespace type = pdn::type;

using u8s = un::u8string;
using u8sv = un::u8string_view;
using entity = pdn::entity<char8_t>;

static int fails = 0;

#define CHECK(...)                                                                      \
	do                                                                                  \
	{                                                                                   \
		if (!(__VA_ARGS__))                                                             \
		{                                                                               \
			++fails;                                                                    \
			::std::printf("FAIL line %d\n", __LINE__);                                  \
		}                                                                               \
	} while (0)

static auto parse_ok(u8sv text) -> entity
{
	auto r = pdn::parse(text, pdn::utf8_tag);
	if (!r)
	{
		::std::printf("unexpected parse failure\n");
		++fails;
		return entity{};
	}
	return ::std::move(r).value();
}

static auto make_sample() -> entity
{
	return parse_ok(u8sv{ u8"i 1\nl [1, 2, 3]\no { a: 1 b: 2 }\ns \"hi\"\nc 'x'\nf 1.5\nt @true\n" });
}

static void test_default_entity()
{
	entity e{};
	CHECK(e.index() == 0u);
	CHECK(e.type_of() == pdn::type_code::i32);
	CHECK(e.as_int() == 0);
	CHECK(e.get_if<type::i32>() != nullptr);
	CHECK(e.get_if<type::i64>() == nullptr);
	CHECK(e.get_opt<type::i32>().value() == 0);
	CHECK(!e.get_opt<type::i64>().has_value());
	CHECK(e.type_test<type::i32>());
	CHECK(!e.type_test<type::i64>());
}

static void test_equality()
{
	entity a{ type::i32{ 1 } };
	entity b{ type::i64{ 1 } };
	entity c{ type::i32{ 1 } };
	CHECK(a == c);
	CHECK(!(a == b));                                   // different alternatives are never equal
	CHECK(a != b);
	CHECK(!(a == entity{}));

	entity s1{ pdn::make_proxy<type::string<char8_t>>(u8sv{ u8"hi" }) };
	entity s2{ pdn::make_proxy<type::string<char8_t>>(u8sv{ u8"hi" }) };
	entity s3{ pdn::make_proxy<type::string<char8_t>>(u8sv{ u8"ho" }) };
	CHECK(s1 == s2);
	CHECK(!(s1 == s3));

	// object equality does not depend on insertion order
	auto o1 = parse_ok(u8sv{ u8"o { a: 1 b: 2 }" });
	auto o2 = parse_ok(u8sv{ u8"o { b: 2 a: 1 }" });
	CHECK(o1 == o2);

	// a null refer only equals another null refer
	pdn::const_refer<char8_t> null1{}, null2{};
	CHECK(null1 == null2);
	CHECK(!(null1 == o1.ref()));
	CHECK(o1.ref() == o1.ref());
	CHECK(o1 == o1.ref());
}

static void test_refer()
{
	auto e = make_sample();
	CHECK(e.type_of() == pdn::type_code::object);

	auto cref = e.cref();
	CHECK(cref.has_value());
	CHECK(static_cast<bool>(cref));
	CHECK(cref.get() == &e);
	CHECK(cref->type_of() == pdn::type_code::object);

	auto ref = e.ref();
	CHECK(ref.has_value());
	CHECK(ref.get() == &e);

	// a refer can modify the entity it points to; a const_refer cannot
	*ref = type::i32{ 42 };
	CHECK(e.as_int() == 42);
	static_assert(::std::same_as<decltype(e.ref()), pdn::refer<char8_t>>);
	static_assert(::std::same_as<decltype(e.cref()), pdn::const_refer<char8_t>>);
	static_assert(::std::same_as<decltype(::std::as_const(e).ref()), pdn::const_refer<char8_t>>);
}

static void test_at()
{
	auto e = make_sample();
	CHECK(e.at(u8sv{ u8"i" }).as_int() == 1);
	CHECK(!e.at(u8sv{ u8"missing" }).has_value());
	CHECK(!e.at(0).has_value());                        // an object has no index access

	auto s = e.at(u8sv{ u8"s" });
	CHECK(s.has_value());
	CHECK(!s.at(0).has_value());                        // a string is not a list

	auto l = e.at(u8sv{ u8"l" });
	CHECK(l.at(0).as_int() == 1);
	CHECK(l.at(2).as_int() == 3);
	CHECK(!l.at(3).has_value());                        // out of range: empty refer, no throw

	CHECK(e[u8sv{ u8"o" }][u8sv{ u8"b" }].as_int() == 2);
	CHECK(e[u8sv{ u8"l" }][1].as_int() == 2);
	CHECK(e[u8sv{ u8"l" }].ref()[2].as_int() == 3);
}

static void test_subscript_throws()
{
	auto e = make_sample();

	bool threw = false;
	try { (void)e[u8sv{ u8"missing" }]; }
	catch (const ::std::out_of_range&) { threw = true; }
	CHECK(threw);

	threw = false;
	try { (void)e[u8sv{ u8"i" }][u8sv{ u8"k" }]; }
	catch (const ::std::bad_variant_access&) { threw = true; }
	CHECK(threw);

	threw = false;
	try { (void)e[u8sv{ u8"i" }][0]; }
	catch (const ::std::bad_variant_access&) { threw = true; }
	CHECK(threw);
}

static void test_get_family()
{
	auto e = make_sample();
	const entity& ce = e;

	const auto& s = pdn::get<type::string<char8_t>>(ce[u8sv{ u8"s" }]);
	CHECK(s == u8s{ u8"hi" });
	CHECK(pdn::get_if<type::string<char8_t>>(ce[u8sv{ u8"s" }]) != nullptr);
	CHECK(pdn::get_if<type::list<char8_t>>(ce[u8sv{ u8"s" }]) == nullptr);

	CHECK(e[u8sv{ u8"i" }].get_opt<type::i32>().value() == 1);
	CHECK(!e[u8sv{ u8"s" }].get_opt<type::i32>().has_value());

	CHECK(e[u8sv{ u8"l" }].type_test<type::list<char8_t>>());
	CHECK(e[u8sv{ u8"l" }].type_test_any<type::i32, type::list<char8_t>>());
	CHECK(!e[u8sv{ u8"l" }].type_test_any<type::i32, type::string<char8_t>>());
	CHECK(e[u8sv{ u8"l" }].type_of() == pdn::type_code::list);

	// get() on an rvalue entity is allowed, get_if() is not
	auto fresh = entity{ type::i32{ 7 } };
	CHECK(::std::move(fresh).get<type::i32>() == 7);
}

static void test_as()
{
	auto e = make_sample();
	CHECK(e[u8sv{ u8"i" }].as_int() == 1);
	CHECK(e[u8sv{ u8"i" }].as_uint() == 1u);
	CHECK(e[u8sv{ u8"f" }].as_fp() == 1.5);
	CHECK(e[u8sv{ u8"t" }].as_bool());
	CHECK(e[u8sv{ u8"s" }].as_string() == u8s{ u8"hi" });
	CHECK(e[u8sv{ u8"l" }].as_list().size() == 3u);
	CHECK(e[u8sv{ u8"o" }].as_object().size() == 2u);

	auto ch = e[u8sv{ u8"c" }].as_char();
	CHECK(ch.size() == 1u);
	CHECK(ch.data()[0] == u8'x');

	CHECK(pdn::as<type::i32>(e[u8sv{ u8"i" }]) == 1);
	CHECK(pdn::as<type::i64>(e[u8sv{ u8"i" }]) == 1);
	CHECK(pdn::as<type::u8string>(e[u8sv{ u8"s" }]) == u8s{ u8"hi" });
	CHECK(pdn::as<int>(e[u8sv{ u8"i" }]) == 1);         // int is i32 on this platform
	CHECK(e[u8sv{ u8"i" }].as(pdn::i32_tag) == 1);
	CHECK(e[u8sv{ u8"i" }].as(pdn::int_tag) == 1);

	// as<> on a refer of the wrong type yields a zero value, not an exception
	CHECK(e[u8sv{ u8"s" }].as_int() == 0);
	CHECK(e[u8sv{ u8"i" }].as_string().empty());
}

static void test_empty_refer()
{
	pdn::const_refer<char8_t> r{};
	CHECK(!r.has_value());
	CHECK(r == pdn::const_refer<char8_t>{});
	CHECK(r.type_of() == pdn::type_code::unknown);
	CHECK(!r.type_test_any<type::i32, type::string<char8_t>>());
	CHECK(r.as_int() == 0);
	CHECK(r.as_string().empty());
	CHECK(r.as_list().empty());
	CHECK(r.as_object().empty());
	CHECK(r.get_if<type::i32>() == nullptr);
	CHECK(!r.at(0).has_value());
}

static void test_copy_deep()
{
	auto e = make_sample();
	entity copy = e;
	CHECK(copy == e);
	CHECK(pdn::get<type::string<char8_t>>(copy[u8sv{ u8"s" }]).data() !=
	      pdn::get<type::string<char8_t>>(e[u8sv{ u8"s" }]).data());

	copy[u8sv{ u8"s" }] = pdn::make_proxy<type::string<char8_t>>(u8sv{ u8"changed" });
	CHECK(e[u8sv{ u8"s" }].as_string() == u8s{ u8"hi" });
}

static void test_other_char_types()
{
	auto r = pdn::parse(un::u16string_view{ u"n 1\ns \"ab\"\n" }, pdn::utf16_tag);
	CHECK(static_cast<bool>(r));
	if (!r) return;

	auto e = ::std::move(r).value();
	CHECK(e[un::u16string_view{ u"n" }].as_int() == 1);
	CHECK(e[un::u16string_view{ u"s" }].as_string() == un::u16string{ u"ab" });
	CHECK(pdn::as<u8s>(e[un::u16string_view{ u"s" }]) == u8s{ u8"ab" });
	CHECK(e.type_of() == pdn::type_code::object);
}

static void test_type_properties()
{
	static_assert(::std::derived_from<entity, type::detail::entity_variant<char8_t>>);
	static_assert(::std::same_as<pdn::u8entity, entity>);
	static_assert(::std::same_as<pdn::entity_ref<char8_t>, pdn::refer<char8_t>>);
	static_assert(::std::same_as<pdn::entity_cref<char8_t>, pdn::const_refer<char8_t>>);
	static_assert(::std::is_copy_constructible_v<entity>);
	static_assert(::std::is_move_constructible_v<entity>);
	static_assert(::std::is_copy_assignable_v<entity>);
	static_assert(::std::is_nothrow_move_constructible_v<entity>);
	static_assert(::std::is_nothrow_copy_constructible_v<pdn::const_refer<char8_t>>);
	static_assert(sizeof(pdn::const_refer<char8_t>) == sizeof(void*));
}

int main()
{
	test_default_entity();
	test_equality();
	test_refer();
	test_at();
	test_subscript_throws();
	test_get_family();
	test_as();
	test_empty_refer();
	test_copy_deep();
	test_other_char_types();
	test_type_properties();

	::std::printf(fails == 0 ? "all entity tests passed\n" : "entity tests FAILED: %d\n", fails);
	return fails == 0 ? 0 : 1;
}
