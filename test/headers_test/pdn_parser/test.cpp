#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <pdn/pdn_code_point_iterator.h>
#include <pdn/pdn_constant_generator.h>
#include <pdn/pdn_error_message_generator.h>
#include <pdn/pdn_lexer.h>
#include <pdn/pdn_parse.h>
#include <pdn/pdn_parser.h>
#include <pdn/pdn_source_position_recorder.h>
#include <pdn/pdn_type_generator.h>

namespace
{
    using u8char_t = pdn::unicode::u8char_t;
    using u8sv     = ::std::u8string_view;

    ::std::size_t failures = 0;

    void check(const bool condition, const char* what)
    {
        if (!condition)
        {
            ++failures;
            ::std::cout << "FAILED: " << what << '\n';
        }
    }

    // Minimal function package: records the position, collects the diagnostics instead of printing
    // them, and provides the type and constant tables the parser asks for.
    struct package_base
    {
        pdn::source_position_recorder     recorder{};
        ::std::vector<pdn::error_message> errors{};
        ::std::size_t                     limit{ 1000 };
        bool                              stop{};
        pdn::stop_reason_code             reason{};

        auto position() const noexcept -> pdn::source_position { return recorder.position(); }
        void update(const pdn::unicode::code_point_t c) noexcept { recorder.update(c); }
        auto error_count() const noexcept -> ::std::size_t { return errors.size(); }
        bool stop_requested() const noexcept { return stop; }
        auto stop_reason() const noexcept -> pdn::stop_reason_code { return reason; }

        void handle_error(pdn::error_message msg)
        {
            errors.push_back(::std::move(msg));
            if (errors.size() >= limit)
            {
                stop = true;
                reason = pdn::stop_reason_code::parse_error_limit;
            }
        }

        static auto generate_error_message(pdn::raw_error_message raw) -> pdn::error_msg_string
        {
            return pdn::default_error_message_generator::generate_error_message(::std::move(raw));
        }
        static auto generate_type(const pdn::type::string<u8char_t>& iden) -> pdn::type_code
        {
            return pdn::type_generator_function<u8char_t>(iden);
        }
        static auto generate_constant(const pdn::unicode::u8string& iden) -> ::std::optional<pdn::entity<u8char_t>>
        {
            return pdn::constant_generator_std_function<u8char_t>(iden);
        }
    };

    auto lex_all(const u8sv src, package_base& fp) -> ::std::vector<pdn::token<u8char_t>>
    {
        auto cp_it = pdn::make_code_point_iterator(src.begin(), src.end(), fp);
        pdn::lexer<u8char_t, package_base, decltype(cp_it)> engine{ fp };
        auto it = pdn::make_token_iterator(::std::move(engine), ::std::move(cp_it), src.end());
        const auto end_it = pdn::make_token_iterator_end();
        ::std::vector<pdn::token<u8char_t>> out{};
        for (; it != end_it && out.size() < 4096; ++it)
        {
            out.push_back(*it);
        }
        return out;
    }

    struct outcome
    {
        pdn::parse_result<u8char_t>       result;
        ::std::vector<pdn::error_message> errors{};
        bool                              stopped{};
        pdn::stop_reason_code             reason{};
    };

    auto run(const u8sv src, const ::std::size_t limit = 1000) -> outcome
    {
        package_base fp{};
        fp.limit = limit;
        auto tokens = lex_all(src, fp);
        auto result = pdn::parse(tokens.begin(), tokens.end(), fp, u8char_t{});
        return outcome{ ::std::move(result), ::std::move(fp.errors), fp.stop, fp.reason };
    }

    auto object_of(const outcome& out) -> const pdn::type::object<u8char_t>&
    {
        return out.result.value().as_object();
    }

    auto field(const pdn::type::object<u8char_t>& o, const u8sv key) -> const pdn::entity<u8char_t>&
    {
        return o.find(pdn::type::string<u8char_t>{ key })->second;
    }

    void test_basic_declarations()
    {
        const auto r = run(u8"a: 1; b: \"x\"; c: 1.5; d: 'c'; e: @true");
        check(r.result.success(), "basic: success");
        check(r.errors.empty(), "basic: no diagnostics");
        check(object_of(r).size() == 5, "basic: five entities");
        check(field(object_of(r), u8"a").as_int() == 1, "basic: a");
        check(field(object_of(r), u8"b").as_string() == u8"x", "basic: b");
        check(field(object_of(r), u8"c").as_fp() == 1.5, "basic: c");
        check(field(object_of(r), u8"d").as_u8char().to_string_view() == u8"c", "basic: d");
        check(field(object_of(r), u8"e").as_bool(), "basic: e");

        const auto empty = run(u8"");
        check(empty.result.success(), "empty: success");
        check(object_of(empty).empty(), "empty: empty object");

        const auto comments = run(u8"// line\n/* block */");
        check(comments.result.success(), "comments: success");
        check(object_of(comments).empty(), "comments: empty object");
    }

    void test_type_annotations()
    {
        const auto ok = run(u8"a: i8 127; b: u8 255; c: f32 1.5; d: bool 2; e: string \"s\"; f: character 'x'");
        check(ok.result.success(), "cast ok: success");
        check(ok.errors.empty(), "cast ok: no diagnostics");
        check(field(object_of(ok), u8"a").as_int() == 127, "cast ok: i8");
        check(field(object_of(ok), u8"b").as_int() == 255, "cast ok: u8");
        check(field(object_of(ok), u8"c").as_fp() == 1.5, "cast ok: f32");
        check(field(object_of(ok), u8"d").as_bool(), "cast ok: bool 2 -> true");
        check(field(object_of(ok), u8"e").as_string() == u8"s", "cast ok: string");
        check(field(object_of(ok), u8"f").as_u8char().to_string_view() == u8"x", "cast ok: character");

        const auto zero = run(u8"a: bool 0");
        check(zero.result.success(), "cast zero: success");
        check(!field(object_of(zero), u8"a").as_bool(), "cast zero: false");

        const auto range = run(u8"a: i8 128");
        check(!range.result.success(), "cast range: failure");
        check(range.errors.size() == 1, "cast range: one diagnostic");
        check(range.errors[0].error_message.ends_with(u8"cannot cast 128 from i32 to i8: out of range [-128, 127]"),
            "cast range: message");

        const auto under = run(u8"a: u8 -1");
        check(under.errors.size() == 1, "cast under: one diagnostic");
        check(under.errors[0].error_message.ends_with(u8"cannot cast -1 from i32 to u8: out of range [0, 255]"),
            "cast under: message");

        const auto total = run(u8"a: i32 1.5");
        check(total.errors.size() == 1, "cast total: one diagnostic");
        check(total.errors[0].error_message.ends_with(u8"cannot cast 1.5 (f64) to i32"), "cast total: message");

        const auto unknown = run(u8"a: zzz 1");
        check(unknown.errors.size() == 1, "cast unknown: one diagnostic");
        check(unknown.errors[0].error_message.ends_with(u8"type \"zzz\" not found"), "cast unknown: message");
    }

    void test_lists()
    {
        const auto plain = run(u8"a: [1, 2, 3]");
        check(plain.result.success(), "list: success");
        const auto& list = field(object_of(plain), u8"a").as_list();
        check(list.size() == 3, "list: size");
        check(list[2].as_int() == 3, "list: third element");

        const auto trailing = run(u8"a: [1, 2,]");
        check(trailing.result.success(), "list trailing comma: success");
        check(field(object_of(trailing), u8"a").as_list().size() == 2, "list trailing comma: size");

        const auto missing_comma = run(u8"a: [1 2]");
        check(!missing_comma.result.success(), "list missing comma: failure");
        check(missing_comma.errors.size() == 1, "list missing comma: one diagnostic");
        check(missing_comma.errors[0].error_message.ends_with(u8"expected comma (,), but got integer-val with value 2"),
            "list missing comma: message");

        const auto missing_right = run(u8"a: [1, 2");
        check(!missing_right.result.success(), "list missing bracket: failure");
        check(missing_right.errors.size() == 1, "list missing bracket: one diagnostic");
        check(missing_right.errors[0].error_message.ends_with(u8"missing right bracket: ]"), "list missing bracket: message");
        check(missing_right.errors[0].position.line == 1 && missing_right.errors[0].position.column == 4,
            "list missing bracket: points at the opening bracket");

        const auto nested = run(u8"a: [[1], [2, [3]]]");
        check(nested.result.success(), "list nested: success");
        check(field(object_of(nested), u8"a").as_list()[1].as_list()[1].as_list()[0].as_int() == 3,
            "list nested: deep element");
    }

    void test_objects()
    {
        const auto plain = run(u8"a: {x: 1; y: \"z\"}");
        check(plain.result.success(), "object: success");
        const auto& o = field(object_of(plain), u8"a").as_object();
        check(o.size() == 2, "object: size");
        check(field(o, u8"x").as_int() == 1, "object: x");
        check(field(o, u8"y").as_string() == u8"z", "object: y");

        const auto comma = run(u8"a: {x: 1, y: 2}");
        check(!comma.result.success(), "object comma: failure");
        check(comma.errors.size() == 1, "object comma: one diagnostic");
        check(comma.errors[0].error_message.ends_with(u8"expected named entity definition, but got ,"),
            "object comma: message");

        const auto missing_right = run(u8"a: {x: 1");
        check(!missing_right.result.success(), "object missing curly: failure");
        check(missing_right.errors.size() == 1, "object missing curly: one diagnostic");
        check(missing_right.errors[0].error_message.ends_with(u8"missing right curly bracket: }"),
            "object missing curly: message");

        const auto duplicate = run(u8"a: {x: 1; x: 2}");
        check(!duplicate.result.success(), "object duplicate: failure");
        check(duplicate.errors.size() == 1, "object duplicate: one diagnostic");
        check(duplicate.errors[0].error_message.ends_with(u8"entity \"x\" is redefined in this scope"),
            "object duplicate: message");
    }

    void test_strings_and_constants()
    {
        const auto concat = run(u8"a: \"x\" \"y\" \"z\"");
        check(concat.result.success(), "concat: success");
        check(field(object_of(concat), u8"a").as_string() == u8"xyz", "concat: joined");

        const auto pi = run(u8"a: @pi");
        check(pi.result.success(), "at: success");
        check(field(object_of(pi), u8"a").as_fp() > 3.14 && field(object_of(pi), u8"a").as_fp() < 3.15,
            "at: pi value");

        const auto missing = run(u8"a: @nope");
        check(!missing.result.success(), "at missing: failure");
        check(missing.errors.size() == 1, "at missing: one diagnostic");
        check(missing.errors[0].error_message.ends_with(u8"at value \"@nope\" not found"), "at missing: message");
        check(missing.errors[0].position.line == 1 && missing.errors[0].position.column == 4,
            "at missing: reports the at-identifier position");
    }

    void test_unary_operations()
    {
        const auto negative = run(u8"a: -5");
        check(negative.result.success(), "unary minus: success");
        check(field(object_of(negative), u8"a").as_int() == -5, "unary minus: value");

        const auto double_negative = run(u8"a: --5");
        check(double_negative.result.success(), "unary double minus: success");
        check(field(object_of(double_negative), u8"a").as_int() == 5, "unary double minus: value");

        const auto float_negative = run(u8"a: -1.5");
        check(float_negative.result.success(), "unary float: success");
        check(field(object_of(float_negative), u8"a").as_fp() == -1.5, "unary float: value");

        const auto string_negative = run(u8"a: -\"x\"");
        check(!string_negative.result.success(), "unary string: failure");
        check(string_negative.errors.size() == 1, "unary string: one diagnostic");
        check(string_negative.errors[0].error_message.ends_with(u8"invalid unary operation: operator- on string with value \"x\""),
            "unary string: message");

        const auto unsigned_negative = run(u8"a: u8 -1");
        check(!unsigned_negative.result.success(), "unary unsigned: failure");
        check(unsigned_negative.errors.size() == 1, "unary unsigned: one diagnostic");
        check(unsigned_negative.errors[0].error_message.ends_with(u8"cannot cast -1 from i32 to u8: out of range [0, 255]"),
            "unary unsigned: message");
    }

    void test_redefinition_and_recovery()
    {
        const auto redefine = run(u8"a: 1; a: 2");
        check(!redefine.result.success(), "redefine: failure");
        check(redefine.errors.size() == 1, "redefine: one diagnostic");
        check(redefine.errors[0].error_message.ends_with(u8"entity \"a\" is redefined in this scope"),
            "redefine: message");
        // The partial value of a failed parse is only readable in release builds (value() asserts).

        const auto recover = run(u8"a: ; b: 2");
        check(!recover.result.success(), "recover: failure");
        check(!recover.errors.empty(), "recover: diagnostics reported");
        check(recover.errors.size() == 1, "recover: exactly one diagnostic");
        check(recover.errors.back().error_message.ends_with(u8"expected expression, but got ;"),
            "recover: later entity was parsed after the diagnostic");

        const auto missing = run(u8"a");
        check(!missing.result.success(), "missing expression: failure");
        check(missing.errors.size() == 1, "missing expression: one diagnostic");
        check(missing.errors[0].error_message.ends_with(u8"expected expression, but got eof"),
            "missing expression: message");
    }

    void test_nesting_depth()
    {
        auto deep = [](const ::std::size_t n)
        {
            ::std::u8string s = u8"a: ";
            s.append(n, u8'[');
            s += u8"1";
            s.append(n, u8']');
            return s;
        };
        const auto allowed = run(deep(256));
        check(allowed.result.success(), "depth 256: success");

        const auto rejected = run(deep(257));
        check(!rejected.result.success(), "depth 257: failure");
        check(rejected.result.error_code() == pdn::parse_error_code::nesting_depth_exceed,
            "depth 257: stop reason");
        check(rejected.errors.empty(), "depth 257: no diagnostic");
    }

    void test_error_limit_and_reuse()
    {
        const auto limited = run(u8"a: ; b: ; c: ;", 1);
        check(!limited.result.success(), "limit: failure");
        check(limited.result.error_code() == pdn::parse_error_code::parse_error_limit, "limit: stop reason");
        check(limited.errors.size() == 1, "limit: only the first diagnostic");

        package_base fp{};
        fp.limit = 1;
        auto bad = lex_all(u8"a: 1; a: 2", fp);
        pdn::parser<u8char_t, package_base> par{ fp };
        par.parse(bad.begin(), bad.end());
        check(par.stop_requested(), "reuse: first run stopped");
        check(par.stop_reason() == pdn::stop_reason_code::parse_error_limit, "reuse: first run reason");

        fp.errors.clear();
        fp.stop = false;
        fp.reason = {};
        auto good = lex_all(u8"b: 2", fp);
        const auto value = par.parse(good.begin(), good.end());
        check(!par.stop_requested(), "reuse: stop state reset");
        check(fp.errors.empty(), "reuse: no diagnostics");
        const auto& o = value.as_object();
        check(o.size() == 1, "reuse: one entity");
        check(field(o, u8"b").as_int() == 2, "reuse: parsed value");
    }

}

int main()
{
    test_basic_declarations();
    test_type_annotations();
    test_lists();
    test_objects();
    test_strings_and_constants();
    test_unary_operations();
    test_redefinition_and_recovery();
    test_nesting_depth();
    test_error_limit_and_reuse();

    if (failures != 0)
    {
        ::std::cout << failures << " check(s) failed\n";
        return 1;
    }
    ::std::cout << "all parser checks passed\n";
    return 0;
}
