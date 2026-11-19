// Tests the decoder/encoder pairing decided by convert_decision from two string types.
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <pdn/pdn_convert_decision.h>

namespace
{
	namespace u = pdn::unicode;

	using decision = u::convert_decision<std::u8string_view, std::u16string>;

	static_assert(std::is_same_v<decision::source_char, char8_t>);
	static_assert(std::is_same_v<decision::target_char, char16_t>);
	static_assert(std::is_same_v<decision::decode_result_type, u::utf8::decode_result>);
	static_assert(std::is_same_v<decision::encode_result_type, u::utf16::encode_result>);
	static_assert(noexcept(decision::encode(U'A')));

	// The source requirement is checked on the const iterators, because code_convert walks a
	// const source; a type that only offers a strong mutable iterator is rejected.
	struct forward_const_iterator
	{
		using iterator_concept = std::forward_iterator_tag;
		using value_type       = char8_t;
		using difference_type  = std::ptrdiff_t;
		using pointer          = const char8_t*;
		using reference        = const char8_t&;

		const char8_t* ptr{};

		reference operator*() const noexcept { return *ptr; }
		forward_const_iterator& operator++() noexcept
		{
			++ptr;
			return *this;
		}
		forward_const_iterator operator++(int) noexcept
		{
			auto temp = *this;
			++ptr;
			return temp;
		}
		friend bool operator==(forward_const_iterator, forward_const_iterator) = default;
	};

	struct mutable_random_access_source
	{
		using value_type = char8_t;
		using iterator   = char8_t*; // random access, but only on a mutable object
		forward_const_iterator begin() const noexcept { return {}; }
		forward_const_iterator end() const noexcept { return {}; }
	};

	// A source is a string-like sequence of the right code unit, not only a string.
	static_assert(u::concepts::convert_src<std::u8string_view>);
	static_assert(u::concepts::convert_src<std::u8string>);
	static_assert(u::concepts::convert_src<std::array<char8_t, 4>>);
	static_assert(u::concepts::convert_src<std::vector<char8_t>>);
	static_assert(u::concepts::convert_src<std::deque<char8_t>>);  // random access, not contiguous
	static_assert(!u::concepts::convert_src<std::list<char8_t>>);  // bidirectional only
	static_assert(!u::concepts::convert_src<const char8_t*>);      // no value_type at all
	static_assert(!u::concepts::convert_src<std::string>);         // wrong code unit
	static_assert(!u::concepts::convert_src<mutable_random_access_source>); // const iterators are weak
	static_assert(u::concepts::utf16_convert_src<std::u16string_view>);
	static_assert(!u::concepts::utf8_convert_src<std::u16string_view>);
	static_assert(!u::concepts::utf16_convert_src<std::u8string_view>);

	// A destination only needs insert(end(), first, last) over the same code unit type, so plain
	// containers qualify too while a type without insert (std::array) does not.
	static_assert(u::concepts::convert_des<std::u8string>);
	static_assert(u::concepts::convert_des<std::u16string>);
	static_assert(u::concepts::convert_des<std::vector<char8_t>>);
	static_assert(u::concepts::convert_des<std::deque<char8_t>>);
	static_assert(!u::concepts::convert_des<std::array<char8_t, 4>>); // no insert
	static_assert(!u::concepts::convert_des<std::string>);            // wrong code unit

	template <typename s, typename d>
	concept has_decision = requires { typename u::convert_decision<s, d>; };

	static_assert(has_decision<std::u8string_view, std::u16string>);
	static_assert(has_decision<std::u16string_view, std::u8string>);
	static_assert(!has_decision<int, std::u16string>);
	static_assert(!has_decision<std::u8string_view, int>);

	int failures = 0;

	void check(const bool condition, const char* what)
	{
		if (!condition)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	void test_decode_reach_flag()
	{
		const std::u8string_view text = u8"\u4E2D"; // three code units
		{
			auto begin = text.begin();
			const auto result = decision::decode<false>(begin, text.end());
			check(static_cast<bool>(result), "decode<false> succeeds");
			check(result.value() == U'\u4E2D', "decode<false> value");
			check(result.distance() == 2, "decode<false> reports two advances");
			check(begin == text.end() - 1, "decode<false> stops on the last code unit");
		}
		{
			auto begin = text.begin();
			const auto result = decision::decode<true>(begin, text.end());
			check(static_cast<bool>(result) && result.value() == U'\u4E2D', "decode<true> value");
			check(begin == text.end(), "decode<true> stops after the sequence");
		}
		{
			// same bytes, decoded through the charset decoder directly
			const std::u8string_view raw = u8"\u4E2D";
			const auto direct = u::utf8::decode<true>(raw.begin(), raw.end());
			check(direct.value() == decision::decode<true>(text.begin(), text.end()).value(),
				"delegates to utf8::decode");
		}
	}

	void test_encode_forwards()
	{
		const auto got  = decision::encode(U'\U0001F600');
		const auto want = u::utf16::encode(U'\U0001F600');
		check(got.size() == 2 && want.size() == 2, "surrogate pair has two code units");
		check(got.begin()[0] == want.begin()[0] && got.begin()[1] == want.begin()[1],
			"encode forwards to utf16::encode");
		check(!decision::encode(0xD800), "a surrogate is not encodable");
		check(!decision::encode(0x110000), "a value beyond U+10FFFF is not encodable");
		check(decision::encode(U'A').size() == 1, "BMP ASCII takes one code unit");
	}

	void test_round_trip()
	{
		const std::u8string_view text = u8"A\u4E2D\U0001F600"; // 1 + 3 + 4 code units
		std::u16string wide;
		auto begin = text.begin();
		int decoded = 0;
		while (begin != text.end())
		{
			const auto result = decision::decode<true>(begin, text.end());
			check(static_cast<bool>(result), "every code point of the sample decodes");
			if (!result)
			{
				break;
			}
			const auto encoded = decision::encode(result.value());
			check(static_cast<bool>(encoded), "every decoded code point encodes");
			if (!encoded)
			{
				break;
			}
			wide.append(encoded.begin(), encoded.end());
			++decoded;
		}
		check(decoded == 3, "three code points decoded");
		check(wide.size() == 1 + 1 + 2, "utf-16 output is 'A', U+4E2D and a surrogate pair");

		// and back through the opposite decision
		using back_t = u::convert_decision<std::u16string_view, std::u8string>;
		std::u8string narrow;
		auto wbegin = std::u16string_view{ wide }.begin();
		while (wbegin != std::u16string_view{ wide }.end())
		{
			const auto result = back_t::decode<true>(wbegin, std::u16string_view{ wide }.end());
			check(static_cast<bool>(result), "utf-16 code point decodes");
			if (!result)
			{
				break;
			}
			const auto encoded = back_t::encode(result.value());
			check(static_cast<bool>(encoded), "utf-8 encoding succeeds");
			if (!encoded)
			{
				break;
			}
			narrow.append(encoded.begin(), encoded.end());
		}
		check(narrow == text, "round trip reproduces the original utf-8 text");
	}
}

int main()
{
	test_decode_reach_flag();
	test_encode_forwards();
	test_round_trip();
	if (failures == 0)
	{
		std::printf("pdn_convert_decision: all checks passed\n");
	}
	return failures;
}
