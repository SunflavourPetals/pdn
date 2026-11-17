// Tests pdn_utf_code_convert.h: cross-width conversion, replacement of malformed input, and the
// error handler contract (offsets, "return true to stop", handlers may write into the target).
#include <pdn/pdn_utf_code_convert.h>

#include <array>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	namespace u = pdn::unicode;
	using u8s  = std::u8string;
	using u16s = std::u16string;
	using u32s = std::u32string;

	constexpr auto replacement = u8"\uFFFD";

	int failures = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	u8s bytes(std::initializer_list<unsigned> values)
	{
		u8s out;
		for (auto v : values)
		{
			out.push_back(static_cast<char8_t>(v));
		}
		return out;
	}

	struct calls
	{
		std::vector<std::size_t> decode_offsets;
		std::vector<std::size_t> encode_offsets;
		std::size_t stop_after = std::numeric_limits<std::size_t>::max();
	};

	// Handler used by the tests below: records the failure, appends '!', stops once told to.
	template <typename target_t>
	bool recording(target_t& out, std::size_t offset, std::vector<std::size_t>& bucket, calls& state)
	{
		bucket.push_back(offset);
		out.push_back(typename target_t::value_type('!'));
		return bucket.size() > state.stop_after;
	}

	template <typename target_t>
	std::pair<target_t, calls> convert(const auto& source, std::size_t stop_after)
	{
		calls state;
		state.stop_after = stop_after;
		auto out = u::code_convert<target_t>(
			source,
			[&state](target_t& o, auto, std::size_t off) { return recording(o, off, state.decode_offsets, state); },
			[&state](target_t& o, auto, std::size_t off) { return recording(o, off, state.encode_offsets, state); });
		return { out, state };
	}

	// Compile-time contract of the handler concept.
	struct good_handler
	{
		bool operator()(u8s&, u::utf8::decode_result, std::size_t) const { return false; }
	};
	struct wrong_return
	{
		void operator()(u8s&, u::utf8::decode_result, std::size_t) const {}
	};
	// int converts to std::size_t, so this one still satisfies the concept.
	struct narrowing_offset
	{
		bool operator()(u8s&, u::utf8::decode_result, int) const { return false; }
	};

	static_assert(u::detail::code_convert_handler<good_handler, u8s, u::utf8::decode_result>);
	static_assert(u::detail::code_convert_handler<bool (*)(u8s&, u::utf8::decode_result, std::size_t), u8s,
		u::utf8::decode_result>);
	static_assert(!u::detail::code_convert_handler<wrong_return, u8s, u::utf8::decode_result>);
	static_assert(u::detail::code_convert_handler<narrowing_offset, u8s, u::utf8::decode_result>);
	static_assert(!u::detail::code_convert_handler<wrong_return, u8s, u::utf16::decode_result>);

	void test_valid_input()
	{
		check(u::code_convert<u8s>(std::u8string_view{ u8"abc" }) == u8"abc", "utf-8 passes through");
		const u8s empty_source;
		check(u::code_convert<u8s>(std::u8string_view{ empty_source }).empty(), "empty source");
		const std::array<char8_t, 2> arr{ u8'h', u8'i' };
		check(u::code_convert<u16s>(arr) == u"hi", "std::array source");

		const u16s wide{ u'A', char16_t{ 0x4E2D }, char16_t{ 0xD83D }, char16_t{ 0xDE00 } };
		const auto to32 = u::code_convert<u32s>(std::u16string_view{ wide });
		check(to32.size() == 3 && to32[2] == U'\U0001F600', "utf-16 pair becomes one utf-32 value");
		check(u::code_convert<u16s>(std::u32string_view{ to32 }) == wide, "utf-32 -> utf-16");
		check(u::code_convert<u8s>(std::u16string_view{ wide }) == u8"A\u4E2D\U0001F600", "utf-16 -> utf-8");
		check(u::code_convert<u32s>(std::u8string_view{ u8"A\u4E2D\U0001F600" }) == to32, "utf-8 -> utf-32");
	}

	void test_replacement()
	{
		const auto invalid = u::code_convert<u8s>(std::u8string_view{ bytes({ 0xFF }) });
		check(invalid == u8"\uFFFD", "invalid byte becomes U+FFFD");
		check(invalid.size() == 3 && invalid[0] == 0xEF && invalid[1] == 0xBF && invalid[2] == 0xBD,
			"U+FFFD is EF BF BD in utf-8");
		check(u::code_convert<u16s>(std::u8string_view{ bytes({ 0xFF }) }) == u"\uFFFD",
			"U+FFFD in utf-16 is a single unit");
		check(u::code_convert<u32s>(std::u8string_view{ bytes({ 0xFF }) }) == U"\uFFFD",
			"U+FFFD in utf-32 is a single unit");

		// truncated, overlong, surrogate and 5-byte forms each yield exactly one replacement
		const u8s malformed[] = {
			bytes({ 0xE4, 0xB8 }),
			bytes({ 0xC0, 0x80 }),
			bytes({ 0xED, 0xA0, 0x80 }),
			bytes({ 0xF8, 0x80, 0x80, 0x80, 0x80 }),
		};
		for (const auto& m : malformed)
		{
			const auto out = u::code_convert<u8s>(std::u8string_view{ m });
			check(out == replacement, "malformed sequence becomes one replacement character");
		}
		// mixed input keeps the surrounding characters
		const u8s mixed = bytes({ 'A', 0xFF, 'B', 0xE4, 0xB8 });
		check(u::code_convert<u8s>(std::u8string_view{ mixed }) == u8"A" u8"\uFFFD" u8"B" u8"\uFFFD", "prefix and middle kept");
	}

	void test_progress_and_offsets()
	{
		// A handler that keeps saying "continue" must still terminate: the decoder consumes the
		// whole malformed sequence, so the loop advances. Each offset is the start of a failure.
		const u8s mixed = bytes({ 'A', 0xFF, 'B', 0xE4, 0xB8 });
		auto [out, state] = convert<u8s>(std::u8string_view{ mixed }, 1000);
		check(state.decode_offsets.size() == 2, "two failures reported");
		check(state.decode_offsets.size() >= 1 && state.decode_offsets[0] == 1, "first offset is 1");
		check(state.decode_offsets.size() >= 2 && state.decode_offsets[1] == 3, "second offset is 3");
		check(state.encode_offsets.empty(), "valid scalars never reach the encode handler");

		// stop on the first failure keeps the converted prefix
		auto [stop1, s1] = convert<u8s>(std::u8string_view{ mixed }, 0);
		check(s1.decode_offsets.size() == 1, "stopped after one failure");
		check(stop1 == bytes({ 'A', '!' }), "prefix plus handler output");

		// stop on the second failure
		auto [stop2, s2] = convert<u8s>(std::u8string_view{ mixed }, 1);
		check(s2.decode_offsets.size() == 2, "stopped after two failures");
		check(stop2 == bytes({ 'A', '!', 'B', '!' }), "both handler outputs kept");

		// a handler that appends nothing and always continues still terminates on every case
		for (const auto& m : { bytes({ 0xFF }), bytes({ 0xE4, 0xB8 }), bytes({ 0xFF, 0xFF, 0xFF }) })
		{
			std::size_t seen = 0;
			const auto outp = u::code_convert<u8s>(
				std::u8string_view{ m },
				[&seen](u8s&, auto, std::size_t) noexcept { return ++seen > 1000; },
				[&seen](u8s&, auto, std::size_t) noexcept { return ++seen > 1000; });
			check(seen <= 3, "at most one failure per malformed byte");
			check(outp.empty(), "no output when the handler appends nothing");
		}
	}

	void test_utf16_and_utf32_errors()
	{
		const u16s lone_lead{ char16_t{ 0xD83D } };
		const u16s lone_trail{ char16_t{ 0xDE00 } };
		const u16s lead_in_middle{ u'A', char16_t{ 0xD83D }, u'B' };
		check(u::code_convert<u8s>(std::u16string_view{ lone_lead }) == replacement, "lone lead surrogate");
		check(u::code_convert<u8s>(std::u16string_view{ lone_trail }) == replacement, "lone trail surrogate");
		check(u::code_convert<u8s>(std::u16string_view{ lead_in_middle }) == u8"A" u8"\uFFFD" u8"B",
			"lead in the middle keeps the tail");

		check(u::code_convert<u8s>(std::u32string_view{ u32s{ char32_t{ 0xD800 } } }) == replacement,
			"utf-32 surrogate");
		check(u::code_convert<u8s>(std::u32string_view{ u32s{ char32_t{ 0x110000 } } }) == replacement,
			"utf-32 value beyond U+10FFFF");
		check(u::code_convert<u32s>(std::u32string_view{ u32s{ char32_t{ 0xD800 } } }) == u32s{ U'\uFFFD' },
			"utf-32 -> utf-32 replacement");
	}

	void test_vector_destination()
	{
		// A destination only needs insert(end(), first, last), so a plain container works too.
		const auto text = u::code_convert<std::vector<char8_t>>(std::u8string_view{ u8"h\u00E9" });
		check(text.size() == 3 && text[0] == u8'h' && text[1] == 0xC3 && text[2] == 0xA9,
			"std::vector<char8_t> destination");

		const auto raw      = bytes({ 0xFF });
		const auto replaced = u::code_convert<std::vector<char8_t>>(std::u8string_view{ raw });
		check(replaced.size() == 3 && replaced[0] == 0xEF && replaced[1] == 0xBF && replaced[2] == 0xBD,
			"replacement inserted into a vector");
	}
}

int main()
{
	test_valid_input();
	test_replacement();
	test_progress_and_offsets();
	test_utf16_and_utf32_errors();
	test_vector_destination();
	if (failures == 0)
	{
		std::printf("pdn_utf_code_convert: all checks passed\n");
	}
	return failures;
}
