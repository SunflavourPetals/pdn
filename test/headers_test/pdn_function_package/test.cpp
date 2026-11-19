#include <cmath>
#include <cstddef>
#include <iostream>
#include <numbers>
#include <sstream>
#include <string>
#include <type_traits>

#include <pdn/pdn_function_package.h>

namespace
{
    using namespace pdn;
    using pdn::unicode::u8char_t;
    using pdn::unicode::u8string;

    int failures{};

    void check(const bool ok, const char* what)
    {
        if (!ok)
        {
            ++failures;
            ::std::cout << "FAILED: " << what << "\n";
        }
    }

    using pkg = default_function_package<u8char_t>;

    // The header asserts the same set for all three character widths; here we pin the u8 one so a
    // regression in the members shows up as a readable failure instead of a static_assert.
    static_assert(concepts::source_position_recorder<pkg>);
    static_assert(concepts::error_handler<pkg>);
    static_assert(concepts::error_message_generator<pkg>);
    static_assert(concepts::type_generator<pkg, u8char_t>);
    static_assert(concepts::constant_generator<pkg, u8char_t>);
    static_assert(concepts::stoppable<pkg>);
    static_assert(concepts::nesting_depth_limiter<pkg>);
    static_assert(::std::is_default_constructible_v<pkg>);
    static_assert(::std::is_constructible_v<pkg, pkg::error_count_t>);
    static_assert(::std::is_constructible_v<pkg, error_msg_string>);

    auto text_of(const error_msg_string& text) -> ::std::string
    {
        return ::std::string{ reinterpret_cast<const char*>(text.data()), text.size() };
    }

    auto redefine_msg(const ::std::size_t line, const ::std::size_t column, const u8string& name) -> error_message
    {
        using raw_iden = raw_error_message_type::identifier;
        const auto raw = raw_error_message{ { syntax_error_code::entity_redefine },
                                            { line, column },
                                            raw_iden{ name } };
        return error_message{ { syntax_error_code::entity_redefine },
                              { line, column },
                              pkg::generate_error_message(raw) };
    }

    void test_defaults()
    {
        pkg fp{};
        check(fp.filename().empty(), "defaults: no filename");
        check(fp.error_count() == 0, "defaults: no errors");
        check(!fp.stop_requested(), "defaults: not stopped");
        check(fp.stop_reason() == stop_reason_code{}, "defaults: no stop reason");
        check(fp.error_threshold() == pkg::default_error_limit, "defaults: default threshold");
        check(pkg::default_error_limit == 100, "defaults: default threshold is 100");
        check(fp.nesting_depth_limit() == pdn::default_nesting_depth_limiter::default_nesting_depth_limit,
              "defaults: nesting limit");
        check(pdn::default_nesting_depth_limiter::default_nesting_depth_limit == 256, "defaults: nesting limit is 256");
        check(fp.position() == (source_position{ 1, 1 }), "defaults: position is 1:1");
    }

    void test_position()
    {
        pkg fp{};
        fp.update(u8'a');
        check(fp.position() == (source_position{ 1, 2 }), "position: after one code point");
        fp.update(u8'\n');
        check(fp.position() == (source_position{ 2, 1 }), "position: after LF");
        fp.update(u8'b');
        check(fp.position() == (source_position{ 2, 2 }), "position: after b");
        fp.clear_position();
        check(fp.position() == (source_position{ 1, 1 }), "position: cleared");
    }

    void test_error_count_and_threshold()
    {
        pkg fp{};
        ::std::ostringstream out{};
        fp.handle_error(redefine_msg(3, 7, u8"x"), out);
        check(fp.error_count() == 1, "count: one error");
        check(out.str() == "syntax error(3:7) entity \"x\" is redefined in this scope\n", "count: text");
        check(!fp.stop_requested(), "count: below the threshold");

        fp.reset_error_threshold(2);
        check(fp.error_threshold() == 2, "threshold: updated");
        fp.handle_error();
        check(fp.error_count() == 2, "count: two errors");
        check(fp.stop_requested(), "threshold: stopped at the limit");
        check(fp.stop_reason() == stop_reason_code::parse_error_limit, "threshold: stop reason");

        fp.handle_error();
        check(fp.error_count() == 3, "count: three errors");
        check(fp.stop_reason() == stop_reason_code::parse_error_limit, "threshold: first reason wins");

        fp.clear_error();
        check(fp.error_count() == 0, "clear: count reset");
        check(!fp.stop_requested(), "clear: stop reset");
        check(fp.error_threshold() == 2, "clear: threshold kept");
    }

    void test_filename_prefix()
    {
        pkg fp{ u8string{ u8"input.pdn" }, 10 };
        check(fp.filename() == u8string{ u8"input.pdn" }, "filename: from the constructor");
        check(fp.error_threshold() == 10, "filename: threshold from the constructor");

        const auto msg = redefine_msg(1, 1, u8"x");
        const auto prefixed = fp.fill_filename(msg);
        check(text_of(prefixed.error_message) == "input.pdn: syntax error(1:1) entity \"x\" is redefined in this scope",
              "filename: fill_filename prefixes");
        check(prefixed.error_code == msg.error_code, "filename: code kept");
        check(prefixed.position == msg.position, "filename: position kept");

        ::std::ostringstream out{};
        fp.handle_error(msg, out);
        check(out.str() == "input.pdn: syntax error(1:1) entity \"x\" is redefined in this scope\n",
              "filename: handle_error prefixes");

        ::std::ostringstream bare{};
        fp.clear_filename();
        check(fp.filename().empty(), "filename: cleared");
        fp.handle_error(msg, bare);
        check(bare.str() == "syntax error(1:1) entity \"x\" is redefined in this scope\n",
              "filename: no prefix after clear");

        fp.set_filename(u8string{ u8"second.pdn" });
        check(fp.filename() == u8string{ u8"second.pdn" }, "filename: set again");
    }

    void test_generators()
    {
        const auto msg = pkg::generate_error_message(
            raw_error_message{ { syntax_error_code::entity_redefine },
                               { 2, 3 },
                               raw_error_message_type::identifier{ u8string{ u8"y" } } });
        check(text_of(msg) == "syntax error(2:3) entity \"y\" is redefined in this scope",
              "generators: error message");

        const auto pi = pkg::generate_constant(u8string{ u8"pi" });
        check(pi.has_value(), "generators: constant found");
        check(pi.has_value() && ::std::abs(pi->as_fp() - ::std::numbers::pi) < 1e-15,
              "generators: constant value");
        check(!pkg::generate_constant(u8string{ u8"nope" }).has_value(), "generators: constant missing");

        check(pkg::generate_type(u8string{ u8"i32" }) == type_code::i32, "generators: known type");
        check(pkg::generate_type(u8string{ u8"nope" }) == type_code::unknown, "generators: unknown type");
    }

    void test_constructors()
    {
        pkg only_threshold{ pkg::error_count_t{ 3 } };
        check(only_threshold.error_threshold() == 3, "ctor: threshold");
        check(only_threshold.filename().empty(), "ctor: filename stays empty");

        pkg with_name{ u8string{ u8"a.pdn" } };
        check(with_name.filename() == u8string{ u8"a.pdn" }, "ctor: filename");
        check(with_name.error_threshold() == pkg::default_error_limit, "ctor: default threshold");
    }
}

int main()
{
    test_defaults();
    test_position();
    test_error_count_and_threshold();
    test_filename_prefix();
    test_generators();
    test_constructors();

    if (failures != 0)
    {
        ::std::cout << failures << " check(s) failed\n";
        return 1;
    }
    ::std::cout << "all function package checks passed\n";
    return 0;
}
