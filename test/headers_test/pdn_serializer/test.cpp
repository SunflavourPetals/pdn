#include <cstdio>
#include <string>
#include <string_view>

#include <pdn/spdn.h>

using namespace std::string_view_literals;

namespace
{
	int fails = 0;

	auto as_std(const std::u8string& s) -> std::string
	{
		return std::string(reinterpret_cast<const char*>(s.data()), s.size());
	}

	auto parse_u8(std::string_view src, pdn::entity<char8_t>& e) -> bool
	{
		auto r = pdn::parse(std::u8string_view{ reinterpret_cast<const char8_t*>(src.data()), src.size() }, pdn::utf8_tag);
		if (!r)
		{
			return false;
		}
		e = std::move(r.value());
		return true;
	}

	auto serialize_u8(const pdn::entity<char8_t>& e) -> std::string
	{
		return as_std(pdn::make_u8serializer().serialize(pdn::as_object(e)));
	}

	auto expect_serialize(std::string_view src, std::string_view want) -> void
	{
		pdn::entity<char8_t> e;
		if (!parse_u8(src, e))
		{
			++fails;
			std::printf("FAILED parse [%s]\n", std::string(src).c_str());
			return;
		}
		const auto got = serialize_u8(e);
		if (got != want)
		{
			++fails;
			std::printf("FAILED serialize [%s]: want [%s] got [%s]\n",
				std::string(src).c_str(), std::string(want).c_str(), got.c_str());
		}
	}

	auto expect_round_trip(std::string_view src) -> void
	{
		pdn::entity<char8_t> a;
		if (!parse_u8(src, a))
		{
			++fails;
			std::printf("FAILED parse [%s]\n", std::string(src).c_str());
			return;
		}
		pdn::entity<char8_t> b;
		if (!parse_u8(serialize_u8(a), b))
		{
			++fails;
			std::printf("FAILED reparse of serialized [%s]\n", std::string(src).c_str());
			return;
		}
		if (!(a == b))
		{
			++fails;
			std::printf("FAILED round-trip [%s]\n", std::string(src).c_str());
		}
	}
}

auto main() -> int
{
	// A type specification is written exactly where implicit typing would change the type.
	expect_serialize("a: 1", "a: 1;\n");
	expect_serialize("a: i8 1", "a: i8 1;\n");
	expect_serialize("a: i32 -2147483648", "a: i32 -2147483648;\n");
	expect_serialize("a: 2147483648", "a: i64 2147483648;\n");
	expect_serialize("a: @i64_min", "a: @i64_min;\n");
	expect_serialize("a: u8 255", "a: u8 255;\n");
	expect_serialize("a: u64 18446744073709551615", "a: u64 18446744073709551615;\n");
	expect_serialize("a: 1.5", "a: 1.5;\n");
	expect_serialize("a: f32 1.5", "a: f32 1.5;\n");
	expect_serialize("a: @inf", "a: @inf;\n");
	expect_serialize("a: @NaN", "a: @NaN;\n");
	expect_serialize("a: @pi", "a: @pi;\n");
	expect_serialize("a: @true", "a: @true;\n");
	expect_serialize("a: 'x'", "a: 'x';\n");
	expect_serialize("a: \"hi\"", "a: \"hi\";\n");

	// Empty object -> empty text (an empty file).
	expect_serialize("", "");

	// Identifier quoting.
	expect_serialize("a_b: 1", "a_b: 1;\n");
	expect_serialize("`a b`: 1", "`a b`: 1;\n");

	// Containers: list elements use the "type: value" form; members are indented one per line.
	expect_serialize("l: [i8: 1]", "l: [\n\ti8: 1,\n];\n");
	expect_serialize("a: {b: 1}", "a: {\n\tb: 1;\n};\n");

	// Style knobs.
	{
		pdn::entity<char8_t> e;
		if (!parse_u8("a: {b: [1, 2]}", e))
		{
			++fails;
		}
		else
		{
			const auto& obj = pdn::as_object(e);
			const auto text = [&](const auto& serializer) { return as_std(serializer.serialize(obj)); };

			const auto default_text = text(pdn::make_u8serializer());
			if (default_text != "a: {\n\tb: [\n\t\t1,\n\t\t2,\n\t];\n};\n")
			{
				++fails;
				std::printf("FAILED default style [%s]\n", default_text.c_str());
			}

			const auto compact = text(pdn::make_serializer<char8_t>(pdn::serialize_tab::no_tab, pdn::serialize_last_semi::no_semi));
			if (compact != "a: {\nb: [\n1,\n2,\n]\n}\n")
			{
				++fails;
				std::printf("FAILED no_tab/no_semi style [%s]\n", compact.c_str());
			}

			const auto spaced = text(pdn::make_serializer<char8_t>(pdn::serialize_tab::space_2, pdn::serialize_last_semi::semi_yes, pdn::serialize_separator::sep_space));
			if (spaced != "a: {\n  b: [\n    1,\n    2,\n  ];\n};\n")
			{
				++fails;
				std::printf("FAILED space_2/sep_space style [%s]\n", spaced.c_str());
			}

			const auto one_line = as_std(pdn::make_u8serializer().serialize(obj, false, true));
			if (one_line != "a: {b: [1, 2, ]; }; ")
			{
				++fails;
				std::printf("FAILED line_feed=false [%s]\n", one_line.c_str());
			}

			// A named list/object value is not terminated when use_last_sep_for_lo is false.
			const auto no_lo_sep = as_std(pdn::make_u8serializer().serialize(obj, true, false));
			if (no_lo_sep != "a: {\n\tb: [\n\t\t1,\n\t\t2,\n\t]\n}\n")
			{
				++fails;
				std::printf("FAILED use_last_sep_for_lo=false [%s]\n", no_lo_sep.c_str());
			}
		}
	}

	// Text that cannot be represented: invalid code units become U+FFFD.
	{
		pdn::type::character<char8_t> bad_char{};
		const char8_t bad[] = { char8_t(0xFF) };
		bad_char = pdn::type::character<char8_t>{ bad, 1 };
		pdn::type::object<char8_t> obj;
		obj.emplace(u8"c", pdn::entity<char8_t>{ bad_char });
		const auto text = as_std(pdn::make_u8serializer().serialize(obj));
		if (text != "c: '\xEF\xBF\xBD';\n")
		{
			++fails;
			std::printf("FAILED invalid character [%s]\n", text.c_str());
		}
	}
	{
		pdn::type::object<char8_t> obj;
		obj.emplace(u8"s", pdn::entity<char8_t>{ pdn::make_proxy<pdn::type::u8string>(pdn::type::u8string{ reinterpret_cast<const char8_t*>("a\xFF"), 2 }) });
		const auto text = as_std(pdn::make_u8serializer().serialize(obj));
		if (text != "s: \"a\xEF\xBF\xBD\";\n")
		{
			++fails;
			std::printf("FAILED invalid string [%s]\n", text.c_str());
		}
	}

	// Other character types.
	{
		auto r = pdn::parse(::std::u16string_view{ u"a: \"\u4E2D\"" }, pdn::utf16_tag);
		if (!r)
		{
			++fails;
			std::printf("FAILED u16 parse\n");
		}
		else
		{
			const auto text = pdn::unicode::code_convert<pdn::type::u8string>(pdn::make_u16serializer().serialize(pdn::as_object(r.value())));
			if (as_std(text) != "a: \"\xE4\xB8\xAD\";\n")
			{
				++fails;
				std::printf("FAILED u16 serialize [%s]\n", as_std(text).c_str());
			}
		}
	}
	{
		auto r = pdn::parse(::std::u32string_view{ U"a: 1" }, pdn::utf32_tag);
		if (!r)
		{
			++fails;
			std::printf("FAILED u32 parse\n");
		}
		else
		{
			const auto text = pdn::unicode::code_convert<pdn::type::u8string>(pdn::make_u32serializer().serialize(pdn::as_object(r.value())));
			if (as_std(text) != "a: 1;\n")
			{
				++fails;
				std::printf("FAILED u32 serialize [%s]\n", as_std(text).c_str());
			}
		}
	}

	// Round trips.
	expect_round_trip("a: 1\nb: -7\nc: 1.5\nd: @true\ne: 'x'\nf: \"hi\"");
	expect_round_trip("a: i8 -128\nb: u64 18446744073709551615\nc: @i64_min\nd: f32 0.5\ne: -@inf");
	expect_round_trip("s: \"tab\\there\\nline\"\nt: \"quote\\\" and backslash\\\\\"");
	expect_round_trip("l: [1, i8: 2, \"three\", 'c', [4], {k: 5}]");
	expect_round_trip("o: {a: 1; b: {c: [2, 3]}}");
	expect_round_trip("`weird key`: 1\n`0digit`: 2");

	if (fails != 0)
	{
		std::printf("%d check(s) failed\n", fails);
		return 1;
	}
	std::printf("all serializer checks passed\n");
	return 0;
}
