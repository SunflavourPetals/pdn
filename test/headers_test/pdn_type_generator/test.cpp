#include <array>
#include <cstdio>
#include <string_view>
#include <type_traits>
#include <utility>

#include <pdn/spdn.h>

namespace
{
    using namespace pdn::literals::unicode_literals;

    int fails = 0;

    #define CHECK(condition) \
        do { if (!(condition)) { ::std::printf("FAILED: %s  (line %d)\n", #condition, __LINE__); ++fails; } } while (false)

    using u8c  = pdn::unicode::u8char_t;
    using u16c = pdn::unicode::u16char_t;
    using u32c = pdn::unicode::u32char_t;

    using gen8  = pdn::default_type_generator<u8c>;
    using gen16 = pdn::default_type_generator<u16c>;
    using gen32 = pdn::default_type_generator<u32c>;

    static_assert(pdn::concepts::type_generator<gen8, u8c>);
    static_assert(pdn::concepts::type_generator<gen16, u16c>);
    static_assert(pdn::concepts::type_generator<gen32, u32c>);
    static_assert(::std::same_as<decltype(gen8::generate_type(u8"i8"_s)), pdn::type_code>);
    static_assert(noexcept(gen8::generate_type(u8"i8"_s)) == noexcept(gen8::generate_type(u8"i8"_s)));

    void test_every_name()
    {
        using namespace pdn::literals::unicode_literals;
        using pdn::type_code;
        struct entry { ::std::u8string_view name; type_code code; };
        constexpr ::std::array table{
            entry{ u8"i8", type_code::i8 },         entry{ u8"i16", type_code::i16 },
            entry{ u8"i32", type_code::i32 },       entry{ u8"i64", type_code::i64 },
            entry{ u8"u8", type_code::u8 },         entry{ u8"u16", type_code::u16 },
            entry{ u8"u32", type_code::u32 },       entry{ u8"u64", type_code::u64 },
            entry{ u8"f32", type_code::f32 },       entry{ u8"f64", type_code::f64 },
            entry{ u8"f", type_code::f32 },         entry{ u8"float", type_code::f32 },
            entry{ u8"double", type_code::f64 },    entry{ u8"boolean", type_code::boolean },
            entry{ u8"bool", type_code::boolean },  entry{ u8"character", type_code::character },
            entry{ u8"char", type_code::character }, entry{ u8"c", type_code::character },
            entry{ u8"string", type_code::string }, entry{ u8"str", type_code::string },
            entry{ u8"s", type_code::string },      entry{ u8"list", type_code::list },
            entry{ u8"object", type_code::object }, entry{ u8"obj", type_code::object },
            entry{ u8"int", type_code::i32 },       entry{ u8"i", type_code::i32 },
#ifndef PDN_NO_EXTENSIONS
            entry{ u8"cppint", type_code::i32 },    entry{ u8"cppuint", type_code::u32 },
#endif
            entry{ u8"uint", type_code::u32 },      entry{ u8"u", type_code::u32 },
        };
#ifndef PDN_NO_EXTENSIONS
        static_assert(table.size() == 30);
#else
        static_assert(table.size() == 28);
#endif
        for (const auto& e : table)
        {
            CHECK(gen8::generate_type(::std::u8string(e.name.begin(), e.name.size())) == e.code);
            CHECK(gen8::generate_type(::std::u8string(e.name.begin(), e.name.size())) == e.code); // stable
        }
    }

    void test_unknown()
    {
        using namespace pdn::literals::unicode_literals;
        const ::std::array<::std::u8string_view, 8> bad{
            u8"", u8"nope", u8"I8", u8"int8", u8"i128", u8"i 8", u8"i8 ", u8"\u00E9" };
        for (const auto& name : bad)
        {
            CHECK(gen8::generate_type(::std::u8string(name.data(), name.size())) == pdn::type_code::unknown);
        }
#ifdef PDN_NO_EXTENSIONS
        // The two extension spellings are unknown when the macro is defined.
        CHECK(gen8::generate_type(u8"cppint") == pdn::type_code::unknown);
        CHECK(gen8::generate_type(u8"cppuint") == pdn::type_code::unknown);
#endif
    }

    void test_other_char_types()
    {
        using namespace pdn::literals::unicode_literals;
        CHECK(gen16::generate_type(u"i64"_s) == pdn::type_code::i64);
        CHECK(gen16::generate_type(u"obj"_s) == pdn::type_code::object);
        CHECK(gen16::generate_type(u"nope"_s) == pdn::type_code::unknown);
        CHECK(gen32::generate_type(U"f64"_s) == pdn::type_code::f64);
        CHECK(gen32::generate_type(U"list"_s) == pdn::type_code::list);
        CHECK(gen32::generate_type(U"nope"_s) == pdn::type_code::unknown);
    }

    void test_parse_integration()
    {
        const auto doc = pdn::parse(::std::u8string_view{ u8"a: i8 1\nb: f32 1.5\nc 2" }, pdn::utf8_tag);
        CHECK(static_cast<bool>(doc));
        const auto& obj = pdn::as_object(doc.value());
        CHECK(pdn::type_of(obj.at(u8"a")) == pdn::type_code::i8);
        CHECK(pdn::type_of(obj.at(u8"b")) == pdn::type_code::f32);
        CHECK(pdn::type_of(obj.at(u8"c")) == pdn::type_code::i32);

        const auto bad = pdn::parse(::std::u8string_view{ u8"x: nope 1" }, pdn::utf8_tag);
        CHECK(!static_cast<bool>(bad));
    }
}

int main()
{
    test_every_name();
    test_unknown();
    test_other_char_types();
    test_parse_integration();
    if (fails != 0)
    {
        ::std::printf("pdn_type_generator: %d failure(s)\n", fails);
        return 1;
    }
    ::std::printf("pdn_type_generator ok\n");
    return 0;
}