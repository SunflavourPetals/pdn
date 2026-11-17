#include <pdn/spdn.h>
#include <pdn/pdn_make_slashes_string.h>

#include <cstdio>
#include <string>
#include <string_view>

#include <pdn/pdn_utf_code_convert.h>

#define CHECK(condition)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(condition))                                                     \
        {                                                                     \
            ::std::printf("FAILED: %s (line %d)\n", #condition, __LINE__);    \
            ++fails;                                                          \
        }                                                                     \
    } while (false)

namespace
{
    using u8s = ::std::u8string;
    using u16s = ::std::u16string;
    using u32s = ::std::u32string;

    int fails = 0;

    // the escape helpers take code points, not a string of code units
    auto code_points(::std::u8string_view s) -> pdn::unicode::ucpstring
    {
        return pdn::unicode::code_convert<pdn::unicode::ucpstring>(s);
    }

    auto code_point_view(const pdn::unicode::ucpstring& s) -> pdn::unicode::ucpstring_view
    {
        return pdn::unicode::ucpstring_view{ s.data(), s.size() };
    }

    // parse `x: <literal>` and return the string it holds
    auto string_of(::std::u8string_view source) -> u8s
    {
        auto r = pdn::parse(source, pdn::utf8_tag);
        if (!r)
        {
            return {};
        }
        return u8s{ pdn::as_object(r.value()).at(u8"x").as_string() };
    }

    // feed the escaped body back to the parser and compare with the original code points
    void round_trip(::std::u8string_view label, const u8s& body, const u8s& expected)
    {
        u8s source = u8"x: \"" + body + u8"\"";
        auto got = string_of(source);
        if (got != expected)
        {
            ::std::printf("FAILED round trip through parse: %s (source size %zu)\n",
                          reinterpret_cast<const char*>(label.data()), source.size());
            ++fails;
        }
        auto r = pdn::parse(source, pdn::utf8_tag);
        if (!r)
        {
            ::std::printf("FAILED parse for %s\n", reinterpret_cast<const char*>(label.data()));
            ++fails;
            return;
        }
        auto out = pdn::make_u8serializer().serialize(pdn::as_object(r.value()));
        auto again = string_of(out);
        if (again != expected)
        {
            ::std::printf("FAILED round trip through serialize: %s\n", reinterpret_cast<const char*>(label.data()));
            ++fails;
        }
    }

    void test_escape_table()
    {
        using pdn::make_slashes_string;

        CHECK(make_slashes_string<u8s>(U'\'') == u8"\\'");
        CHECK(make_slashes_string<u8s>(U'"') == u8"\\\"");
        CHECK(make_slashes_string<u8s>(U'\\') == u8"\\\\");
        CHECK(make_slashes_string<u8s>(U'\a') == u8"\\a");
        CHECK(make_slashes_string<u8s>(U'\b') == u8"\\b");
        CHECK(make_slashes_string<u8s>(U'\f') == u8"\\f");
        CHECK(make_slashes_string<u8s>(U'\n') == u8"\\n");
        CHECK(make_slashes_string<u8s>(U'\r') == u8"\\r");
        CHECK(make_slashes_string<u8s>(U'\t') == u8"\\t");
        CHECK(make_slashes_string<u8s>(U'\v') == u8"\\v");

        // NUL keeps the short \0 form unless the next character written is an octal digit;
        // the octal escape would otherwise swallow that digit and change the value.
        const pdn::unicode::code_point_t nul_only[] = { U'\0' };
        const pdn::unicode::code_point_t nul_one[] = { U'\0', U'1' };
        const pdn::unicode::code_point_t nul_seven[] = { U'\0', U'7' };
        const pdn::unicode::code_point_t nul_eight[] = { U'\0', U'8' };
        const pdn::unicode::code_point_t nul_a[] = { U'\0', U'a' };
        const pdn::unicode::code_point_t nul_tab[] = { U'\0', U'\t' };
        CHECK(make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_only, 1 }) == u8"\\0");
        CHECK(make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_one, 2 }) == u8"\\o{0}1");
        CHECK(make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_seven, 2 }) == u8"\\o{0}7");
        CHECK(make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_eight, 2 }) == u8"\\08");
        CHECK(make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_a, 2 }) == u8"\\0a");
        CHECK(make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_tab, 2 }) == u8"\\0\\t");
        CHECK(pdn::make_slashes_id_string<u8s>(pdn::unicode::ucpstring_view{ nul_one, 2 }) == u8"\\o{0}1");
        CHECK(make_slashes_string<u8s>(U'\0') == u8"\\0");
        CHECK(make_slashes_string<u8s>(U'\u0085') == u8"\\u0085");
        CHECK(make_slashes_string<u8s>(U'\u2028') == u8"\\u2028");
        CHECK(make_slashes_string<u8s>(U'\u2029') == u8"\\u2029");

        // everything else is written as itself
        CHECK(make_slashes_string<u8s>(U'a') == u8"a");
        CHECK(make_slashes_string<u8s>(U'@') == u8"@");
        CHECK(make_slashes_string<u8s>(U'`') == u8"`");
        CHECK(make_slashes_string<u8s>(U'\u4E2D') == u8"\u4E2D");
        CHECK(make_slashes_string<u8s>(U'\U0001F600') == u8"\U0001F600");

        // non scalar values produce nothing
        CHECK(make_slashes_string<u8s>(0xD800u).empty());
        CHECK(make_slashes_string<u8s>(0xDFFFu).empty());
        CHECK(make_slashes_string<u8s>(0x110000u).empty());

        // the code unit type follows the string type
        CHECK(make_slashes_string<u16s>(U'\U0001F600') == u"\U0001F600");
        CHECK(make_slashes_string<u32s>(U'\U0001F600') == U"\U0001F600");
        CHECK(make_slashes_string<u16s>(U'\u2028') == u"\\u2028");
        CHECK(make_slashes_string<u32s>(U'\U0001F600').size() == 1);
    }

    void test_identifier_table()
    {
        using pdn::make_slashes_id_string;

        CHECK(make_slashes_id_string<u8s>(U'`') == u8"\\`");
        CHECK(make_slashes_id_string<u8s>(U'\'') == u8"\\'");
        CHECK(make_slashes_id_string<u8s>(U'\\') == u8"\\\\");
        CHECK(make_slashes_id_string<u8s>(U'\n') == u8"\\n");
        CHECK(make_slashes_id_string<u8s>(U'a') == u8"a");

        u8s sv = u8"a`b";
        CHECK(make_slashes_id_string<u8s>(code_point_view(code_points(sv))) == u8"a\\`b");
    }

    void test_round_trip()
    {
        using pdn::make_slashes_string;

        round_trip(u8"nul", make_slashes_string<u8s>(U'\0'), u8s(1, u8'\0'));
        // the regression: NUL followed by an octal digit used to merge into one character
        const pdn::unicode::code_point_t nul_one_rt[] = { U'\0', U'1' };
        round_trip(u8"nul then 1", make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_one_rt, 2 }), u8s(1, u8'\0') + u8"1");
        const pdn::unicode::code_point_t nul_seven_rt[] = { U'\0', U'7' };
        round_trip(u8"nul then 7", make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_seven_rt, 2 }), u8s(1, u8'\0') + u8"7");
        const pdn::unicode::code_point_t nul_eight_rt[] = { U'\0', U'8' };
        round_trip(u8"nul then 8", make_slashes_string<u8s>(pdn::unicode::ucpstring_view{ nul_eight_rt, 2 }), u8s(1, u8'\0') + u8"8");
        round_trip(u8"tab", make_slashes_string<u8s>(U'\t'), u8s{ u8"\t" });
        round_trip(u8"quote", make_slashes_string<u8s>(U'\''), u8s{ u8"'" });
        round_trip(u8"dquote", make_slashes_string<u8s>(U'"'), u8s{ u8"\"" });
        round_trip(u8"backslash", make_slashes_string<u8s>(U'\\'), u8s{ u8"\\" });
        round_trip(u8"nel", make_slashes_string<u8s>(U'\u0085'), u8s{ u8"\u0085" });
        round_trip(u8"ls", make_slashes_string<u8s>(U'\u2028'), u8s{ u8"\u2028" });
        round_trip(u8"ps", make_slashes_string<u8s>(U'\u2029'), u8s{ u8"\u2029" });
        round_trip(u8"cjk", make_slashes_string<u8s>(U'\u4E2D'), u8s{ u8"\u4E2D" });
        round_trip(u8"emoji", make_slashes_string<u8s>(U'\U0001F600'), u8s{ u8"\U0001F600" });
        round_trip(u8"control 0x01 raw", u8s(1, '\x01'), u8s(1, '\x01'));

        // a string mixing all of them survives serialize + parse
        const char8_t mixed_raw[] = u8"a\tb\0c\rd\\e\"f'g\x01";
        u8s mixed{ mixed_raw, sizeof(mixed_raw) - 1 };
        u8s body = make_slashes_string<u8s>(code_point_view(code_points(mixed)));
        round_trip(u8"mixed", body, mixed);
    }

    void test_identifier_round_trip()
    {
        // a NUL inside a backtick identifier: the \o{0} form must survive a parse
        {
            const pdn::unicode::code_point_t nul_key[] = { U'\0', U'1' };
            u8s body = pdn::make_slashes_id_string<u8s>(pdn::unicode::ucpstring_view{ nul_key, 2 });
            CHECK(body == u8"\\o{0}1");
            u8s source = u8"`" + body + u8"`: 1";
            auto r = pdn::parse(source, pdn::utf8_tag);
            CHECK(r);
            if (r)
            {
                CHECK(pdn::as_object(r.value()).contains(u8s(1, u8'\0') + u8"1"));
            }
        }
        // keys that are not plain identifiers are written in backtick form
        for (::std::u8string_view key : { u8"a b", u8"a'b", u8"a\"b", u8"a`b", u8"a\tb" })
        {
            u8s key_s{ key };
            u8s body = pdn::make_slashes_id_string<u8s>(code_point_view(code_points(key_s)));
            u8s source = u8"`" + body + u8"`: 1";
            auto r = pdn::parse(source, pdn::utf8_tag);
            if (!r)
            {
                ::std::printf("FAILED parse identifier source size %zu\n", source.size());
                ++fails;
                continue;
            }
            auto obj = pdn::as_object(r.value());
            auto out = pdn::make_u8serializer().serialize(obj);
            auto r2 = pdn::parse(out, pdn::utf8_tag);
            if (!r2)
            {
                ::std::printf("FAILED re-parse identifier output size %zu\n", out.size());
                ++fails;
                continue;
            }
            auto obj2 = pdn::as_object(r2.value());
            CHECK(obj2.size() == 1);
            CHECK(obj2.contains(key));
        }
    }
    // A message is one line of text: the opt-in flag turns every non-printable code point into the
    // fixed width \uXXXX form, while PDN's own escapes keep precedence and printable text stays raw.
    void test_escape_non_printable()
    {
        using pdn::make_slashes_string;
        using pdn::unicode::ucpstring_view;

        namespace un = pdn::unicode;
        const auto escaped = [](un::ucpstring_view sv) { return pdn::make_slashes_string<u8s, true>(sv); };
        const auto escaped_cp = [](un::code_point_t c) { return pdn::make_slashes_string<u8s, true>(c); };
        const auto escaped_id = [](un::ucpstring_view sv) { return pdn::make_slashes_id_string<u8s, true>(sv); };

        const un::code_point_t controls[] = { U'a', U'\x01', U'\x1b', U'\x7f', U'b' };
        const un::ucpstring_view controls_view{ controls, 5 };
        // the default is unchanged: the serializer writes these code points as themselves
        CHECK(pdn::make_slashes_string<u8s>(controls_view) == u8s{ u8"a\x01\x1b\x7f" u8"b" });
        CHECK(escaped(controls_view) == u8"a\\u0001\\u001b\\u007fb");

        // PDN escapes win over the fixed width form
        CHECK(escaped_cp(U'\t') == u8"\\t");
        CHECK(escaped_cp(U'\n') == u8"\\n");
        CHECK(escaped_cp(U'\'') == u8"\\'");
        CHECK(escaped_cp(U'\\') == u8"\\\\");
        CHECK(escaped_cp(U'\0') == u8"\\0");

        // C1 controls, DEL and the line separators
        CHECK(escaped_cp(U'\u0080') == u8"\\u0080");
        CHECK(escaped_cp(U'\u0085') == u8"\\u0085");
        CHECK(escaped_cp(U'\u009f') == u8"\\u009f");
        CHECK(escaped_cp(U'\u2028') == u8"\\u2028");
        CHECK(escaped_cp(U'\u2029') == u8"\\u2029");

        // the twelve Bidi_Control code points: they occupy no width but can reorder the line
        CHECK(escaped_cp(U'\u061c') == u8"\\u061c");
        CHECK(escaped_cp(U'\u200e') == u8"\\u200e");
        CHECK(escaped_cp(U'\u200f') == u8"\\u200f");
        CHECK(escaped_cp(U'\u202a') == u8"\\u202a");
        CHECK(escaped_cp(U'\u202e') == u8"\\u202e");
        CHECK(escaped_cp(U'\u2066') == u8"\\u2066");
        CHECK(escaped_cp(U'\u2069') == u8"\\u2069");
        const un::code_point_t bidi[] = { U'a', U'\u202e', U'b' };
        // the default is still unchanged: the serializer writes the code point itself
        CHECK(pdn::make_slashes_string<u8s>(un::ucpstring_view{ bidi, 3 }) == u8s{ u8"a\u202eb" });
        CHECK(escaped(un::ucpstring_view{ bidi, 3 }) == u8"a\\u202eb");

        // printable text stays raw, non scalar values still produce nothing
        CHECK(escaped_cp(U'a') == u8"a");
        CHECK(escaped_cp(U'\u4E2D') == u8"\u4E2D");
        CHECK(escaped_cp(U'\U0001F600') == u8"\U0001F600");
        CHECK(escaped_cp(0xD800u).empty());

        // the flag reaches the code unit types and the identifier form
        CHECK((pdn::make_slashes_string<u16s, true>(U'\x01') == u"\\u0001"));
        CHECK((pdn::make_slashes_string<u32s, true>(U'\x01') == U"\\u0001"));
        const un::code_point_t id[] = { U'a', U'`', U'\x01' };
        CHECK(escaped_id(un::ucpstring_view{ id, 3 }) == u8"a\\`\\u0001");
        CHECK(pdn::make_slashes_id_string<u8s>(un::ucpstring_view{ id, 3 }) == u8s{ u8"a\\`\x01" });
    }
}

int main()
{
    test_escape_table();
    test_identifier_table();
    test_round_trip();
    test_identifier_round_trip();
    test_escape_non_printable();
    if (fails != 0)
    {
        ::std::printf("%d check(s) failed\n", fails);
    }
    return fails == 0 ? 0 : 1;
}
