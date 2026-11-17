#include <pdn/pdn_code_point_iterator.h>
#include <pdn/pdn_error_message_generator.h>
#include <pdn/pdn_source_position_recorder.h>

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    int fails = 0;

    void check(bool ok, const char* what)
    {
        if (!ok)
        {
            ++fails;
            std::printf("FAILED: %s\n", what);
        }
    }

    using namespace pdn;

    // The three services the iterator consumes: where the next code point starts, how many errors
    // were reported, and how a raw payload turns into a message.
    struct recording_package
    {
        source_position_recorder   recorder{};
        std::vector<error_message> errors{};

        auto position() const noexcept -> source_position { return recorder.position(); }
        void update(unicode::code_point_t c) noexcept { recorder.update(c); }
        auto error_count() const noexcept -> std::size_t { return errors.size(); }
        void handle_error(const error_message& msg) { errors.push_back(msg); }
        void handle_error(error_message&& msg) { errors.push_back(std::move(msg)); }
        static auto generate_error_message(raw_error_message raw) -> error_msg_string
        {
            return default_error_message_generator::generate_error_message(std::move(raw));
        }
    };

    static_assert(concepts::function_package_for_code_point_iterator<recording_package>);

    // The same services, but this handler stops after the first error, the way a handler with an
    // error limit does. The iterator has to notice that and stop instead of resynchronizing.
    struct stopping_package
    {
        recording_package inner{};
        bool              stop{};

        auto position() const noexcept -> source_position { return inner.position(); }
        void update(unicode::code_point_t c) noexcept { inner.update(c); }
        auto error_count() const noexcept -> std::size_t { return inner.error_count(); }
        void handle_error(const error_message& msg)
        {
            inner.handle_error(msg);
            stop = true;
        }
        void handle_error(error_message&& msg)
        {
            inner.handle_error(std::move(msg));
            stop = true;
        }
        static auto generate_error_message(raw_error_message raw) -> error_msg_string
        {
            return recording_package::generate_error_message(std::move(raw));
        }
        auto stop_requested() const noexcept -> bool { return stop; }
        auto stop_reason() const noexcept -> stop_reason_code
        {
            return stop ? stop_reason_code::parse_error_limit : stop_reason_code{};
        }
    };

    static_assert(concepts::stoppable<stopping_package>);
    static_assert(concepts::function_package_for_code_point_iterator<stopping_package>);

    // A code unit iterator that asks to stop after a fixed number of advances. The iterator has to
    // check the request after every advance and forward it to its own stop state.
    struct counting_units
    {
        using value_type = unicode::u8char_t;

        const unicode::u8char_t* pos{};
        const unicode::u8char_t* last{};
        std::size_t              advanced{};
        std::size_t              stop_after{};
        bool                     stopped{};

        auto operator*() const noexcept -> unicode::u8char_t { return *pos; }
        auto operator++() noexcept -> counting_units&
        {
            ++pos;
            if (++advanced == stop_after)
            {
                stopped = true;
            }
            return *this;
        }
        auto eof() const noexcept -> bool { return pos == last; }
        auto stop_requested() const noexcept -> bool { return stopped; }
        auto stop_reason() const noexcept -> stop_reason_code
        {
            return stopped ? stop_reason_code::user_cancelled : stop_reason_code{};
        }
        friend auto operator==(const counting_units& lhs, const counting_units& rhs) noexcept -> bool
        {
            return lhs.pos == rhs.pos;
        }
    };

    static_assert(concepts::eof_detector<counting_units>);
    static_assert(concepts::stoppable<counting_units>);

    struct step
    {
        source_position position;
        char32_t        code_point;
    };

    // Reads the whole stream through one iterator, recording where each code point was handed out.
    // eof() is what the lexer relies on: the loop stops when the underlying stream is exhausted.
    template <typename char_t>
    auto read_all(std::basic_string_view<char_t> text, recording_package& pkg) -> std::vector<step>
    {
        std::vector<step> steps{};
        auto it = make_code_point_iterator(text.cbegin(), text.cend(), pkg);
        while (!it.eof())
        {
            steps.push_back({ pkg.position(), it.get() });
            ++it;
        }
        return steps;
    }

    void test_ascii()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"abc" }, pkg);
        check(steps.size() == 3, "ascii: three code points");
        check(steps[0].code_point == U'a' && steps[0].position.line == 1 && steps[0].position.column == 1,
            "ascii: a at 1:1");
        check(steps[1].code_point == U'b' && steps[1].position.column == 2, "ascii: b at 1:2");
        check(steps[2].code_point == U'c' && steps[2].position.column == 3, "ascii: c at 1:3");
        check(pkg.position().line == 1 && pkg.position().column == 4, "ascii: eof position is 1:4");
        check(pkg.error_count() == 0, "ascii: no error");
    }

    void test_multi_byte()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"\u03B1\u03B2" }, pkg);
        check(steps.size() == 2, "multi byte: two code points");
        check(steps[0].code_point == U'\u03B1' && steps[0].position.column == 1, "multi byte: alpha at 1:1");
        check(steps[1].code_point == U'\u03B2' && steps[1].position.column == 2, "multi byte: beta at 1:2");
        check(pkg.error_count() == 0, "multi byte: no error");
    }

    void test_line_breaks()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"a\nb" }, pkg);
        check(steps.size() == 3, "lf: three code points");
        check(steps[1].code_point == U'\n' && steps[1].position.line == 1, "lf: newline ends line 1");
        check(steps[2].code_point == U'b' && steps[2].position.line == 2 && steps[2].position.column == 1,
            "lf: b at 2:1");

        // CRLF is one line break: the CR moves to the next line and the LF after it does not move
        // again, so the character after the pair is at the start of the new line.
        recording_package crlf_pkg{};
        const auto crlf_steps = read_all(std::u8string_view{ u8"a\r\nb" }, crlf_pkg);
        check(crlf_steps.size() == 4, "crlf: four code points");
        check(crlf_steps[1].code_point == U'\r' && crlf_steps[1].position.line == 1, "crlf: cr at 1:2");
        check(crlf_steps[2].code_point == U'\n' && crlf_steps[2].position.line == 2, "crlf: lf at 2:1");
        check(crlf_steps[3].code_point == U'b' && crlf_steps[3].position.line == 2 &&
            crlf_steps[3].position.column == 1, "crlf: b at 2:1");
    }

    void test_bad_leading_byte()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"\xFF" u8"a" }, pkg);
        check(steps.size() == 1 && steps[0].code_point == U'a' && steps[0].position.column == 2,
            "bad leading: the byte is skipped and the following code point is at 1:2");
        check(pkg.error_count() == 1, "bad leading: one error");
        const auto code = std::get<unicode::utf8::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf8::decode_error_code::unsupported_leading,
            "bad leading: error code is unsupported_leading");
        check(pkg.errors[0].position.line == 1 && pkg.errors[0].position.column == 1,
            "bad leading: the error points at the rejected byte");
        check(pkg.errors[0].error_message ==
            u8"utf-8 decode error(1:1) unsupported UTF-8 leading code unit: 0xff, in the sequence at offset "
            u8"0x0000 (add 3 if the input starts with a BOM), 1 code unit was read",
            "bad leading: message names the byte and the offset");
    }

    void test_truncated_sequence()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"\xE4\xB8" }, pkg);
        check(steps.empty(), "truncated: no code point");
        check(pkg.error_count() == 1, "truncated: one error");
        const auto code = std::get<unicode::utf8::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf8::decode_error_code::unexpected_eof_offset2,
            "truncated: the decoder ran out of input after two code units");
        check(pkg.errors[0].error_message ==
            u8"utf-8 decode error(1:1) unexpected end of input while reading code unit 3 of the sequence at "
            u8"offset 0x0000 (add 3 if the input starts with a BOM)",
            "truncated: message says which code unit is missing");
    }

    void test_surrogate()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"\xED\xA0\x80" u8"a" }, pkg);
        check(steps.size() == 1 && steps[0].code_point == U'a' && steps[0].position.column == 2,
            "surrogate: the three bytes are skipped as one sequence");
        check(pkg.error_count() == 1, "surrogate: one error");
        const auto code = std::get<unicode::utf8::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf8::decode_error_code::invalid_code_point, "surrogate: invalid code point");
    }

    void test_overlong()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"\xC0\x80" u8"x" }, pkg);
        check(steps.size() == 1 && steps[0].code_point == U'x' && steps[0].position.column == 2,
            "overlong: the two bytes are skipped");
        check(pkg.error_count() == 1, "overlong: one error");
        const auto code = std::get<unicode::utf8::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf8::decode_error_code::non_shortest_sequence,
            "overlong: error code is non_shortest_sequence");
    }

    void test_incomplete_sequence_resync()
    {
        // The byte after the leading byte is not a trailing byte, so the decoder stops on it without
        // consuming it and the iterator does not skip anything: the next code point is that byte.
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"\xE4" "A" }, pkg);
        check(steps.size() == 1 && steps[0].code_point == U'A' && steps[0].position.column == 2,
            "incomplete: the offending byte is decoded as the next code point");
        check(pkg.error_count() == 1, "incomplete: one error");
        const auto code = std::get<unicode::utf8::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf8::decode_error_code::incomplete_sequence, "incomplete: error code");
        check(pkg.errors[0].error_message.find(u8"code unit: 0x41") != error_msg_string::npos,
            "incomplete: message names the non-trailing code unit");
    }

    void test_error_in_the_middle()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"a\xFF" u8"b" }, pkg);
        check(steps.size() == 2, "middle: two code points");
        check(steps[0].code_point == U'a' && steps[0].position.column == 1, "middle: a at 1:1");
        check(steps[1].code_point == U'b' && steps[1].position.column == 3,
            "middle: the rejected byte still occupies one column");
        check(pkg.errors[0].position.line == 1 && pkg.errors[0].position.column == 2,
            "middle: the error points at 1:2");
    }

    void test_error_at_line_start()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u8string_view{ u8"a\n\xFF" u8"b" }, pkg);
        check(steps.size() == 3, "line start: three code points");
        check(pkg.errors[0].position.line == 2 && pkg.errors[0].position.column == 1,
            "line start: the error is reported at 2:1");
    }

    void test_empty()
    {
        recording_package pkg{};
        const std::u8string_view text{ u8"" };
        auto it = make_code_point_iterator(text.cbegin(), text.cend(), pkg);
        check(it.eof(), "empty: eof immediately");
        check(it == text.cend(), "empty: equals the end of the stream");
        check(it.get() == 0, "empty: no code point was produced");
        check(pkg.position().line == 1 && pkg.position().column == 1, "empty: position stays 1:1");
        check(pkg.error_count() == 0, "empty: no error");
    }

    void test_utf16()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u16string_view{ u"a\U0001F600b" }, pkg);
        check(steps.size() == 3, "utf16: three code points");
        check(steps[1].code_point == 0x1F600 && steps[1].position.column == 2,
            "utf16: the surrogate pair is one code point at 1:2");
        check(steps[2].code_point == U'b' && steps[2].position.column == 3, "utf16: b at 1:3");
        check(pkg.error_count() == 0, "utf16: no error");
    }

    void test_utf16_lone_surrogate()
    {
        recording_package pkg{};
        const auto steps = read_all(std::u16string_view{ u"\xD800" u"A" }, pkg);
        check(steps.size() == 1 && steps[0].code_point == U'A', "utf16 lone: A is the next code point");
        check(pkg.error_count() == 1, "utf16 lone: one error");
        const auto code = std::get<unicode::utf16::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf16::decode_error_code::incomplete_sequence, "utf16 lone: error code");
        check(pkg.errors[0].error_message.find(u8"(add 2 if the input starts with a BOM)") !=
            error_msg_string::npos, "utf16 lone: the offset hint uses the code unit width");
    }

    void test_utf32_invalid_code_point()
    {
        recording_package pkg{};
        // The value is built by hand: a universal character name above the codespace would not
        // compile as a literal.
        const char32_t units[]{ U'a', static_cast<char32_t>(0x110000), U'x' };
        const auto steps = read_all(std::u32string_view{ units, 3 }, pkg);
        check(steps.size() == 2, "utf32: two code points");
        check(steps[0].code_point == U'a' && steps[0].position.column == 1, "utf32: a at 1:1");
        check(steps[1].code_point == U'x' && steps[1].position.column == 3, "utf32: x at 1:3");
        check(pkg.error_count() == 1, "utf32: one error");
        const auto code = std::get<unicode::utf32::decode_error_code>(pkg.errors[0].error_code);
        check(code == unicode::utf32::decode_error_code::non_code_point, "utf32: error code");
        check(pkg.errors[0].error_message.find(u8"0x00110000") != error_msg_string::npos,
            "utf32: message names the rejected value");
        check(pkg.errors[0].error_message.find(u8"offset 0x0004") != error_msg_string::npos,
            "utf32: the offset counts bytes, not code units");
    }

    void test_stop_from_function_package()
    {
        stopping_package pkg{};
        auto it = make_code_point_iterator(std::u8string_view{ u8"\xFF" u8"abc" }.cbegin(),
            std::u8string_view{ u8"\xFF" u8"abc" }.cend(), pkg);
        check(pkg.error_count() == 1, "stop package: the error was reported");
        check(it.stop_requested(), "stop package: the iterator stops when the handler stops");
        check(it.stop_reason() == stop_reason_code::parse_error_limit, "stop package: the reason is forwarded");
        check(!it.eof(), "stop package: the stream is not reported as exhausted");
    }

    void test_stop_from_code_unit_iterator()
    {
        const unicode::u8char_t units[]{ u8'a', u8'b', u8'c' };
        recording_package pkg{};
        auto it = make_code_point_iterator(counting_units{ units, units + 3, 0, 1, false },
            counting_units{ units + 3, units + 3, 0, 1, false }, pkg);
        check(it.get() == U'a', "stop units: the first code point is read before any advance");
        check(!it.stop_requested(), "stop units: nothing stops yet");
        ++it;
        check(it.stop_requested(), "stop units: the request is noticed after the advance");
        check(it.stop_reason() == stop_reason_code::user_cancelled, "stop units: the reason is forwarded");
    }

    void test_equality()
    {
        const std::u8string_view text{ u8"ab" };
        recording_package pkg{};
        auto it = make_code_point_iterator(text.cbegin(), text.cend(), pkg);
        check(!(it == text.cend()), "equality: a fresh iterator is not at the end");
        ++it;
        check(!(it == text.cend()), "equality: one advance is not the end");
        ++it;
        check(it == text.cend(), "equality: two advances reach the end");
        check(it.eof(), "equality: eof agrees with the comparison");
    }
}

int main()
{
    test_ascii();
    test_multi_byte();
    test_line_breaks();
    test_bad_leading_byte();
    test_truncated_sequence();
    test_surrogate();
    test_overlong();
    test_incomplete_sequence_resync();
    test_error_in_the_middle();
    test_error_at_line_start();
    test_empty();
    test_utf16();
    test_utf16_lone_surrogate();
    test_utf32_invalid_code_point();
    test_stop_from_function_package();
    test_stop_from_code_unit_iterator();
    test_equality();

    if (fails == 0)
    {
        std::printf("all checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
