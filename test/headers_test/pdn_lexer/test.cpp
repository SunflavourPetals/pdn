#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <pdn/pdn_code_point_iterator.h>
#include <pdn/pdn_error_message_generator.h>
#include <pdn/pdn_lexer.h>
#include <pdn/pdn_source_position_recorder.h>
#include <pdn/pdn_token.h>

namespace
{
    using u8char_t = pdn::unicode::u8char_t;
    using u8sv     = ::std::u8string_view;
    using token_t  = pdn::token<u8char_t>;

    ::std::size_t failures = 0;

    void check(const bool condition, const char* what)
    {
        if (!condition)
        {
            ++failures;
            ::std::cout << "FAILED: " << what << '\n';
        }
    }

    template <typename T>
    auto value_of(const token_t& tk) -> T
    {
        return ::std::get<T>(tk.value);
    }

    auto string_of(const token_t& tk) -> pdn::unicode::u8string
    {
        return *::std::get<pdn::proxy<pdn::type::string<u8char_t>>>(tk.value);
    }

    auto id_of(const token_t& tk) -> pdn::unicode::u8string
    {
        return ::std::get<pdn::detail::at_iden_string_proxy>(tk.value).get_id();
    }

    auto at(const token_t& tk, const ::std::size_t line, const ::std::size_t column) -> bool
    {
        return tk.position.line == line && tk.position.column == column;
    }

    // Minimal function package: records the position, collects the diagnostics instead of
    // printing them, and renders messages through the library's generator.
    struct package_base
    {
        pdn::source_position_recorder     recorder{};
        ::std::vector<pdn::error_message> errors{};

        auto position() const noexcept -> pdn::source_position { return recorder.position(); }
        void update(const pdn::unicode::code_point_t c) noexcept { recorder.update(c); }
        auto error_count() const noexcept -> ::std::size_t { return errors.size(); }
        void handle_error(const pdn::error_message& msg) { errors.push_back(msg); }
        void handle_error(pdn::error_message&& msg) { errors.push_back(::std::move(msg)); }
        static auto generate_error_message(pdn::raw_error_message raw) -> pdn::error_msg_string
        {
            return pdn::default_error_message_generator::generate_error_message(::std::move(raw));
        }
    };

    struct lex_result
    {
        ::std::vector<token_t>            tokens{};
        ::std::vector<pdn::error_message> errors{};
    };

    // Drives the lexer through its token iterator until eof. The iterator is capped to keep a
    // misbehaving lexer from hanging the test.
    auto lex(const u8sv src) -> lex_result
    {
        package_base fp{};
        auto cp_it = pdn::make_code_point_iterator(src.begin(), src.end(), fp);
        pdn::lexer<u8char_t, package_base, decltype(cp_it)> engine{ fp };
        auto it = pdn::make_token_iterator(::std::move(engine), ::std::move(cp_it), src.end());
        auto end_it = pdn::make_token_iterator_end();
        lex_result out{};
        for (; it != end_it; ++it)
        {
            out.tokens.push_back(*it);
            if (out.tokens.size() > 64) { break; }
        }
        out.errors = ::std::move(fp.errors);
        return out;
    }

    // Stoppable package: asks the lexer to stop with parse_error_limit on the first diagnostic.
    struct stop_package : package_base
    {
        bool                  stop{};
        pdn::stop_reason_code reason{};

        bool stop_requested() const noexcept { return stop; }
        auto stop_reason() const noexcept -> pdn::stop_reason_code { return reason; }
        void request_stop(const pdn::stop_reason_code r) noexcept
        {
            if (!stop)
            {
                stop = true;
                reason = r;
            }
        }
        void handle_error(const pdn::error_message& msg)
        {
            package_base::handle_error(msg);
            request_stop(pdn::stop_reason_code::parse_error_limit);
        }
        void handle_error(pdn::error_message&& msg)
        {
            package_base::handle_error(::std::move(msg));
            request_stop(pdn::stop_reason_code::parse_error_limit);
        }
    };

    void test_empty_input()
    {
        package_base fp{};
        const u8sv src = u8"";
        auto cp_it = pdn::make_code_point_iterator(src.begin(), src.end(), fp);
        pdn::lexer<u8char_t, package_base, decltype(cp_it)> engine{ fp };
        const auto tk = engine.get_token(cp_it, src.end());
        check(tk.code == pdn::pdn_token_code::eof, "empty: eof token");
        check(at(tk, 1, 1), "empty: eof at 1:1");
        check(fp.errors.empty(), "empty: no diagnostics");
    }

    void test_identifiers()
    {
        const auto r = lex(u8"abc");
        check(r.tokens.size() == 1, "identifier: token count");
        check(r.tokens.front().code == pdn::pdn_token_code::identifier, "identifier: code");
        check(string_of(r.tokens.front()) == u8"abc", "identifier: text");
        check(at(r.tokens.front(), 1, 1), "identifier: position");

        const auto at_id = lex(u8"@i32_max");
        check(at_id.tokens.size() == 1, "at identifier: token count");
        check(at_id.tokens.front().code == pdn::pdn_token_code::at_identifier, "at identifier: code");
        check(id_of(at_id.tokens.front()) == u8"i32_max", "at identifier: text");

        const auto inf = lex(u8"\u221e");
        check(inf.tokens.size() == 1, "infinity: token count");
        check(inf.tokens.front().code == pdn::pdn_token_code::at_identifier, "infinity: code");
        check(id_of(inf.tokens.front()) == u8"infinity", "infinity: text");

        const auto inf_at = lex(u8"@\u221e");
        check(inf_at.tokens.size() == 2, "at infinity: token count");
        check(inf_at.tokens[0].code == pdn::pdn_token_code::at_sign, "at infinity: at sign");
        check(inf_at.tokens[1].code == pdn::pdn_token_code::at_identifier, "at infinity: identifier");
        check(id_of(inf_at.tokens[1]) == u8"infinity", "at infinity: text");
    }

    void test_integers()
    {
        const auto dec = lex(u8"42");
        check(value_of<pdn::type::i32>(dec.tokens.front()) == 42, "integer: decimal 42");

        const auto big = lex(u8"2147483648");
        check(value_of<pdn::type::i64>(big.tokens.front()) == 2147483648, "integer: i64 overflow");

        const auto huge = lex(u8"18446744073709551615");
        check(value_of<pdn::type::u64>(huge.tokens.front()) == 18446744073709551615ull, "integer: u64 max");

        const auto hex = lex(u8"0x2a");
        check(value_of<pdn::type::i32>(hex.tokens.front()) == 42, "integer: hex 0x2a");

        const auto bin = lex(u8"0b1010");
        check(value_of<pdn::type::i32>(bin.tokens.front()) == 10, "integer: binary 0b1010");

        const auto oct = lex(u8"0777");
        check(value_of<pdn::type::i32>(oct.tokens.front()) == 511, "integer: octal 0777");

        const auto bad_oct = lex(u8"09");
        check(bad_oct.errors.size() == 1, "integer: 09 is reported");
        check(value_of<pdn::type::i32>(bad_oct.tokens.front()) == 9, "integer: 09 reads as decimal");

        const auto sep = lex(u8"1'000");
        check(value_of<pdn::type::i32>(sep.tokens.front()) == 1000, "integer: separator");
    }

    void test_floats()
    {
        const auto dec = lex(u8"3.5");
        check(dec.tokens.front().code == pdn::pdn_token_code::literal_floating_point, "float: code");
        check(value_of<pdn::type::f64>(dec.tokens.front()) == 3.5, "float: 3.5");

        const auto exp = lex(u8"1e3");
        check(value_of<pdn::type::f64>(exp.tokens.front()) == 1000.0, "float: 1e3");

        const auto hex = lex(u8"0x1p4");
        check(value_of<pdn::type::f64>(hex.tokens.front()) == 16.0, "float: hex 0x1p4");

        const auto neg = lex(u8"-1.5");
        check(neg.tokens.size() == 2, "float: negative token count");
        check(neg.tokens[0].code == pdn::pdn_token_code::minus, "float: minus token");
        check(value_of<pdn::type::f64>(neg.tokens[1]) == 1.5, "float: magnitude");
    }

    void test_characters_and_strings()
    {
        const auto ch = lex(u8"'a'");
        check(ch.tokens.front().code == pdn::pdn_token_code::literal_character, "character: code");
        check(value_of<pdn::type::character<u8char_t>>(ch.tokens.front()).to_string_view() == u8"a", "character: a");

        const auto esc_ch = lex(u8"'\\n'");
        check(value_of<pdn::type::character<u8char_t>>(esc_ch.tokens.front()).to_string_view() == u8"\n", "character: escaped newline");

        const auto str = lex(u8"\"hi\"");
        check(str.tokens.front().code == pdn::pdn_token_code::literal_string, "string: code");
        check(string_of(str.tokens.front()) == u8"hi", "string: text");

        const auto esc = lex(u8"\"a\\tb\"");
        check(string_of(esc.tokens.front()) == u8"a\tb", "string: tab escape");

        const auto uni = lex(u8"\"\\u0041\"");
        check(string_of(uni.tokens.front()) == u8"A", "string: \\u0041");

        const auto oct = lex(u8"\"\\o{101}\"");
        check(string_of(oct.tokens.front()) == u8"A", "string: \\o{101}");
    }

    void test_raw_strings()
    {
        const auto raw = lex(u8"@\"(a\\tb)\"");
        check(raw.tokens.front().code == pdn::pdn_token_code::literal_string, "raw string: code");
        check(string_of(raw.tokens.front()) == u8"a\\tb", "raw string: no escape processing");

        const auto delimited = lex(u8"@\"##(body)##\"");
        check(string_of(delimited.tokens.front()) == u8"body", "raw string: delimited");
    }

    void test_comments()
    {
        const auto line = lex(u8"// x\n42");
        check(line.tokens.size() == 1, "line comment: token count");
        check(value_of<pdn::type::i32>(line.tokens.front()) == 42, "line comment: value");
        check(at(line.tokens.front(), 2, 1), "line comment: position");

        const auto block = lex(u8"/* c */42");
        check(value_of<pdn::type::i32>(block.tokens.front()) == 42, "block comment: value");
        check(at(block.tokens.front(), 1, 8), "block comment: position");

        const auto nested = lex(u8"/* </ /> */7");
        check(nested.tokens.size() == 1, "nested comment: token count");
        check(value_of<pdn::type::i32>(nested.tokens.front()) == 7, "nested comment: value");
        check(at(nested.tokens.front(), 1, 12), "nested comment: position");

        const auto nested_bad = lex(u8"/* </ ");
        check(nested_bad.errors.size() == 1, "nested comment: unterminated is reported");
    }

    void test_punctuation_and_positions()
    {
        const auto brackets = lex(u8"[]");
        check(brackets.tokens.size() == 2, "brackets: token count");
        check(brackets.tokens[0].code == pdn::pdn_token_code::left_brackets, "brackets: left");
        check(brackets.tokens[1].code == pdn::pdn_token_code::right_brackets, "brackets: right");
        check(at(brackets.tokens[0], 1, 1) && at(brackets.tokens[1], 1, 2), "brackets: positions");

        const auto lines = lex(u8"a\r\nb");
        check(lines.tokens.size() == 2, "crlf: token count");
        check(at(lines.tokens[0], 1, 1), "crlf: first line");
        check(at(lines.tokens[1], 2, 1), "crlf: second line");
    }

    void test_error_recovery()
    {
        const auto unterminated = lex(u8"\"abc");
        check(unterminated.errors.size() == 1, "unterminated string: one diagnostic");
        check(!unterminated.errors.empty()
              && ::std::string_view{ reinterpret_cast<const char*>(unterminated.errors.front().error_message.data()),
                                     unterminated.errors.front().error_message.size() }
                     .find("missing its terminating double quote") != ::std::string_view::npos,
              "unterminated string: message");

        const auto bad_comment = lex(u8"/* x");
        check(bad_comment.errors.size() == 1, "unterminated comment: one diagnostic");

        const auto bad_number = lex(u8"0x 1");
        check(bad_number.errors.size() == 1, "bad hex: one diagnostic");
        check(bad_number.tokens.size() >= 2, "bad hex: lexing continues");
        check(value_of<pdn::type::i32>(bad_number.tokens[1]) == 1, "bad hex: following integer");
    }

    void test_token_iterator()
    {
        package_base fp{};
        const u8sv src = u8"abc";
        auto cp_it = pdn::make_code_point_iterator(src.begin(), src.end(), fp);
        pdn::lexer<u8char_t, package_base, decltype(cp_it)> engine{ fp };
        auto it = pdn::make_token_iterator(::std::move(engine), ::std::move(cp_it), src.end());
        auto end_it = pdn::make_token_iterator_end();

        check(it != end_it, "iterator: not eof yet");
        check((*it).code == pdn::pdn_token_code::identifier, "iterator: first token");
        check((*it).code == pdn::pdn_token_code::identifier, "iterator: lookahead is stable");
        ++it;
        check(it == end_it, "iterator: reaches end");
    }

    void test_stop_propagation()
    {
        stop_package fp{};
        const u8sv src = u8"\"abc";
        auto cp_it = pdn::make_code_point_iterator(src.begin(), src.end(), fp);
        pdn::lexer<u8char_t, stop_package, decltype(cp_it)> engine{ fp };
        (void)engine.get_token(cp_it, src.end());
        check(engine.stop_requested(), "stop: requested");
        check(engine.stop_reason() == pdn::stop_reason_code::parse_error_limit, "stop: reason");
        check(fp.errors.size() == 1, "stop: one diagnostic");
    }
}

int main()
{
    test_empty_input();
    test_identifiers();
    test_integers();
    test_floats();
    test_characters_and_strings();
    test_raw_strings();
    test_comments();
    test_punctuation_and_positions();
    test_error_recovery();
    test_token_iterator();
    test_stop_propagation();

    if (failures != 0)
    {
        ::std::cout << failures << " check(s) failed\n";
        return 1;
    }
    ::std::cout << "all lexer checks passed\n";
    return 0;
}
