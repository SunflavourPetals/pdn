// Review test for pdn_code_unit_iterator.h.
// The header turns a sequence of bytes into a sequence of code units. The interesting parts
// are: endianness (both directions, utf-16 and utf-32), what happens when the last unit is
// cut short (stop_reason_code::incomplete_code_unit), how eof() is answered for eof()-driven
// and begin/end-driven sources, and that utf-8 is a straight pass-through.

#include <pdn/pdn_code_unit_iterator.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>

namespace
{
	int failures = 0;

	template <typename t, typename u>
	constexpr bool same = ::std::is_same_v<t, u>;

	using pdn::code_unit_iterator;
	using pdn::unicode::encode_type;
	using pdn::unicode::u8char_t;
	using pdn::unicode::u16char_t;
	using pdn::unicode::u32char_t;
	using pdn::stop_reason_code;

	// eof()-driven byte source, the shape byte_iterator_from_byte_swap_chain has.
	struct bytes
	{
		const ::std::uint8_t* p{};
		const ::std::uint8_t* e{};
		bool eof() const noexcept { return p == e; }
		const ::std::uint8_t& operator*() const noexcept { return *p; }
		bytes& operator++() noexcept { ++p; return *this; }
	};

	// Same, but it reports a stop after a given number of increments, with a reason.
	struct stopping_bytes
	{
		const ::std::uint8_t* p{};
		const ::std::uint8_t* e{};
		int stop_after{};
		int steps{};
		bool eof() const noexcept { return p == e; }
		const ::std::uint8_t& operator*() const noexcept { return *p; }
		stopping_bytes& operator++() noexcept { ++p; ++steps; return *this; }
		bool stop_requested() const noexcept { return steps >= stop_after; }
		stop_reason_code stop_reason() const noexcept { return stop_reason_code::bad_stream; }
	};

	struct chars8
	{
		const char8_t* p{};
		const char8_t* e{};
		bool eof() const noexcept { return p == e; }
		const char8_t& operator*() const noexcept { return *p; }
		chars8& operator++() noexcept { ++p; return *this; }
	};

	template <encode_type et, typename source_t>
	auto read_one(source_t source) -> decltype(auto)
	{
		code_unit_iterator<et, source_t, source_t> it{ ::std::move(source) };
		return *it;
	}

	void check(bool ok, const char* what, int line)
	{
		if (!ok)
		{
			::std::printf("FAILED (line %d): %s\n", line, what);
			++failures;
		}
	}

#define CHECK(expr) check((expr), #expr, __LINE__)
}

int main()
{
	// ---------------------------------------------------------------- alias dispatch
	static_assert(same<code_unit_iterator<encode_type::utf8, const char8_t*, const char8_t*>,
	                   pdn::detail::u8_code_unit_iterator<const char8_t*>>);
	static_assert(same<code_unit_iterator<encode_type::utf16_le, bytes, bytes>,
	                   pdn::detail::mb_code_unit_iterator<encode_type::utf16_le, bytes, bytes>>);
	static_assert(same<code_unit_iterator<encode_type::utf32_be, bytes, bytes>,
	                   pdn::detail::mb_code_unit_iterator<encode_type::utf32_be, bytes, bytes>>);
	static_assert(same<typename code_unit_iterator<encode_type::utf16_le, bytes, bytes>::char_type, u16char_t>);
	static_assert(same<typename code_unit_iterator<encode_type::utf16_le, bytes, bytes>::code_unit_type, u16char_t>);
	static_assert(same<typename code_unit_iterator<encode_type::utf32_le, bytes, bytes>::code_unit_type, u32char_t>);
	static_assert(same<typename code_unit_iterator<encode_type::utf8, const char8_t*, const char8_t*>::code_unit_type, u8char_t>);
	// Not a real iterator: no iterator traits on purpose.
	static_assert(same<typename code_unit_iterator<encode_type::utf16_le, bytes, bytes>::iterator_concept, void>);
	// The one-argument construction is explicit, so a byte source never turns into a code unit
	// iterator behind the caller's back.
	static_assert(::std::is_constructible_v<code_unit_iterator<encode_type::utf16_le, bytes, bytes>, bytes>);
	static_assert(!::std::is_convertible_v<bytes, code_unit_iterator<encode_type::utf16_le, bytes, bytes>>);

	// ---------------------------------------------------------------- assembly, both endians
	{
		const ::std::uint8_t le16[] = { 0x3D, 0xD8 };       // U+D83D in utf-16 le
		const ::std::uint8_t be16[] = { 0xD8, 0x3D };       // U+D83D in utf-16 be
		CHECK(read_one<encode_type::utf16_le>(bytes{ le16, le16 + 2 }) == 0xD83D);
		CHECK(read_one<encode_type::utf16_be>(bytes{ be16, be16 + 2 }) == 0xD83D);
	}
	{
		const ::std::uint8_t le32[] = { 0x00, 0xF6, 0x01, 0x00 }; // U+1F600 le
		const ::std::uint8_t be32[] = { 0x00, 0x01, 0xF6, 0x00 }; // U+1F600 be
		CHECK(read_one<encode_type::utf32_le>(bytes{ le32, le32 + 4 }) == 0x1F600);
		CHECK(read_one<encode_type::utf32_be>(bytes{ be32, be32 + 4 }) == 0x1F600);
	}
	{
		// A value with the top bit of the last byte set: the shift must stay in the unsigned
		// domain here, otherwise this is signed overflow.
		const ::std::uint8_t le32[] = { 0x00, 0x00, 0x00, 0xFF };
		const ::std::uint8_t be32[] = { 0xFF, 0x00, 0x00, 0x00 };
		CHECK(read_one<encode_type::utf32_le>(bytes{ le32, le32 + 4 }) == 0xFF000000);
		CHECK(read_one<encode_type::utf32_be>(bytes{ be32, be32 + 4 }) == 0xFF000000);
	}

	// ---------------------------------------------------------------- truncated last unit
	{
		const ::std::uint8_t cut16[] = { 0x3D };
		code_unit_iterator<encode_type::utf16_le, bytes, bytes> it{ bytes{ cut16, cut16 + 1 } };
		CHECK(static_cast<::std::uint32_t>(*it) == 0x3D); // the byte that was there is kept
		CHECK(it.eof());
		CHECK(it.stop_requested());
		CHECK(it.stop_reason() == stop_reason_code::incomplete_code_unit);
	}
	{
		const ::std::uint8_t cut32[] = { 0x00, 0x00, 0xFF }; // three of four bytes
		code_unit_iterator<encode_type::utf32_be, bytes, bytes> it{ bytes{ cut32, cut32 + 3 } };
		CHECK(it.stop_requested());
		CHECK(it.stop_reason() == stop_reason_code::incomplete_code_unit);
	}

	// ---------------------------------------------------------------- empty source
	{
		code_unit_iterator<encode_type::utf16_le, bytes, bytes> it{ bytes{ nullptr, nullptr } };
		CHECK(it.eof());
		CHECK(!it.stop_requested());
		CHECK(it.stop_reason() == stop_reason_code{}); // no stop was requested
	}

	// ---------------------------------------------------------------- stop of the byte source
	{
		const ::std::uint8_t two[] = { 0x3D, 0xD8 };
		stopping_bytes source{ two, two + 2, 1, 0 };
		code_unit_iterator<encode_type::utf16_le, stopping_bytes, stopping_bytes> it{ source };
		CHECK(static_cast<::std::uint32_t>(*it) == 0x3D); // stopped after the first byte
		CHECK(it.stop_requested());
		CHECK(it.stop_reason() == stop_reason_code::bad_stream);
	}

	// ---------------------------------------------------------------- begin/end source
	{
		const ::std::uint8_t two_units[] = { 0x41, 0x00, 0x42, 0x00 }; // "AB" in utf-16 le
		code_unit_iterator<encode_type::utf16_le, const ::std::uint8_t*, const ::std::uint8_t*>
			it{ two_units, two_units + 4 };
		using it_t = decltype(it);
		static_assert(same<it_t, pdn::detail::mb_code_unit_iterator<encode_type::utf16_le,
		                                                             const ::std::uint8_t*,
		                                                             const ::std::uint8_t*>>);
		CHECK(*it == 0x0041);
		CHECK(!it.eof());
		++it;
		CHECK(*it == 0x0042);
		CHECK(!it.eof()); // still standing on the last byte of the second unit
		++it;
		CHECK(it.eof());
		CHECK(!it.stop_requested()); // eof between units is not an error
	}
	{
		// A half unit at the end of a begin/end source is reported, not silently dropped.
		const ::std::uint8_t half[] = { 0x41 };
		code_unit_iterator<encode_type::utf16_le, const ::std::uint8_t*, const ::std::uint8_t*>
			it{ half, half + 1 };
		CHECK(*it == 0x0041);
		CHECK(it.stop_requested());
		CHECK(it.stop_reason() == stop_reason_code::incomplete_code_unit);
	}

	// ---------------------------------------------------------------- utf-8 pass-through
	{
		const char8_t text[] = u8"abc";
		pdn::detail::u8_code_unit_iterator<chars8> it{ chars8{ text, text + 3 } };
		static_assert(same<decltype(*it), const char8_t&>); // the source hands out references
		CHECK(*it == u8'a');
		++it;
		CHECK(*it == u8'b');
		++it;
		CHECK(*it == u8'c');
		++it;
		CHECK(it.eof());
	}
	{
		// With a pointer the iterator still hands out a reference: the source dereferences to
		// an lvalue, so the cheap path is taken.
		const char8_t text[] = u8"xy";
		pdn::detail::u8_code_unit_iterator<const char8_t*> it{ text };
		static_assert(same<decltype(*it), const char8_t&>);
		CHECK(*it == u8'x');
		CHECK(!(it == text + 2));
		++it;
		CHECK(*it == u8'y');
		++it;
		CHECK(it == text + 2);
	}

	// ---------------------------------------------------------------- factory
	{
		const ::std::uint8_t bytes16[] = { 0x41, 0x00 };
		auto it16 = pdn::make_code_unit_iterator<encode_type::utf16_le>(bytes{ bytes16, bytes16 + 2 },
		                                                                bytes{ bytes16, bytes16 + 2 });
		CHECK(*it16 == 0x0041);
		auto it32 = pdn::make_code_unit_iterator<encode_type::utf32_be>(bytes{ bytes16 + 0, bytes16 + 0 },
		                                                                bytes{ bytes16, bytes16 + 0 });
		CHECK(it32.eof());
		const char8_t text[] = u8"z";
		auto it8 = pdn::make_code_unit_iterator<encode_type::utf8>(chars8{ text, text + 1 },
		                                                          chars8{ text, text + 1 });
		CHECK(*it8 == u8'z');
	}

	if (failures != 0)
	{
		::std::printf("%d check(s) failed\n", failures);
		return 1;
	}
	::std::printf("all code unit iterator checks passed\n");
	return 0;
}
