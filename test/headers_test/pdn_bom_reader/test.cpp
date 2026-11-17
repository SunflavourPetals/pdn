#include <pdn/pdn_bom_reader.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

namespace
{
	int failures = 0;

	void check(bool ok, const char* what, int line)
	{
		if (!ok) { ++failures; std::printf("FAILED: %s (line %d)\n", what, line); }
	}

	const std::string utf8_bom{ "\xEF\xBB\xBF", 3 };
	const std::string utf16le_bom{ "\xFF\xFE", 2 };
	const std::string utf16be_bom{ "\xFE\xFF", 2 };
	const std::string utf32le_bom{ "\xFF\xFE\x00\x00", 4 };
	const std::string utf32be_bom{ "\x00\x00\xFE\xFF", 4 };

	// A byte input stream that is not a std::istream: the concept is structural,
	// so the BOM probe has to work with anything providing the required calls.
	struct byte_input
	{
		using char_type = char;

		std::string   data;
		std::size_t   pos{};
		std::streamsize counted{};
		bool          failed{};

		explicit byte_input(std::string bytes) : data(std::move(bytes)) {}

		void read(char* out, std::streamsize n)
		{
			const std::size_t available = data.size() - pos;
			const std::size_t take = available < static_cast<std::size_t>(n) ? available : static_cast<std::size_t>(n);
			for (std::size_t i{}; i != take; ++i) { out[i] = data[pos + i]; }
			pos += take;
			counted = static_cast<std::streamsize>(take);
			failed = take != static_cast<std::size_t>(n);
		}
		auto gcount() const -> std::streamsize { return counted; }
		auto fail() const -> bool { return failed; }
		auto rdstate() const -> std::ios_base::iostate
		{
			return failed ? std::ios_base::failbit : std::ios_base::goodbit;
		}
		void clear(std::ios_base::iostate) { failed = false; }
		void seekg(std::streamoff offset, std::ios_base::seekdir dir)
		{
			if (dir != std::ios_base::cur) { failed = true; return; }
			const auto target = static_cast<std::streamoff>(pos) + offset;
			if (target < 0 || static_cast<std::size_t>(target) > data.size()) { failed = true; return; }
			pos = static_cast<std::size_t>(target);
		}
		explicit operator bool() const { return !failed; }

		auto rest() const -> std::string { return data.substr(pos); }
	};

	struct seekg_only
	{
		using char_type = char;
		void seekg(std::streamoff, std::ios_base::seekdir) {}
	};

	struct wide_stream
	{
		using char_type = wchar_t;
		void seekg(std::streamoff, std::ios_base::seekdir) {}
	};
}

static_assert(pdn::unicode::concepts::ibyte_stream<std::istream>);
static_assert(pdn::unicode::concepts::ibyte_stream<std::ifstream>);
static_assert(pdn::unicode::concepts::ibyte_stream<std::istringstream>);
static_assert(pdn::unicode::concepts::ibyte_stream<byte_input>);
static_assert(pdn::unicode::concepts::ibyte_stream<std::istream&>);
static_assert(!pdn::unicode::concepts::ibyte_stream<seekg_only>);
static_assert(!pdn::unicode::concepts::ibyte_stream<wide_stream>);
static_assert(!pdn::unicode::concepts::ibyte_stream<std::wistream>);
static_assert(!pdn::unicode::concepts::ibyte_stream<int>);
static_assert(!pdn::unicode::concepts::ibyte_stream<std::string>);

// check_bom is constexpr and covers all byte orders; long marks win over short ones.
static_assert(pdn::unicode::check_bom(0, {}) == pdn::unicode::bom_type::no_bom);
static_assert(pdn::unicode::check_bom(1, { pdn::unicode::bom::byte_t(0xEF) }) == pdn::unicode::bom_type::no_bom);
static_assert(pdn::unicode::check_bom(2, { pdn::unicode::bom::byte_t(0xEF), pdn::unicode::bom::byte_t(0xBB) })
	== pdn::unicode::bom_type::no_bom);
static_assert(pdn::unicode::check_bom(3, { pdn::unicode::bom::byte_t(0xEF), pdn::unicode::bom::byte_t(0xBB),
	pdn::unicode::bom::byte_t(0xBF) }) == pdn::unicode::bom_type::utf8);
static_assert(pdn::unicode::check_bom(4, { pdn::unicode::bom::byte_t(0xEF), pdn::unicode::bom::byte_t(0xBB),
	pdn::unicode::bom::byte_t(0xBF), pdn::unicode::bom::byte_t(0x41) }) == pdn::unicode::bom_type::utf8);
static_assert(pdn::unicode::check_bom(2, { pdn::unicode::bom::byte_t(0xFF), pdn::unicode::bom::byte_t(0xFE) })
	== pdn::unicode::bom_type::utf16_le);
static_assert(pdn::unicode::check_bom(2, { pdn::unicode::bom::byte_t(0xFE), pdn::unicode::bom::byte_t(0xFF) })
	== pdn::unicode::bom_type::utf16_be);
static_assert(pdn::unicode::check_bom(4, { pdn::unicode::bom::byte_t(0xFF), pdn::unicode::bom::byte_t(0xFE),
	pdn::unicode::bom::byte_t(0x00), pdn::unicode::bom::byte_t(0x00) }) == pdn::unicode::bom_type::utf32_le);
static_assert(pdn::unicode::check_bom(4, { pdn::unicode::bom::byte_t(0x00), pdn::unicode::bom::byte_t(0x00),
	pdn::unicode::bom::byte_t(0xFE), pdn::unicode::bom::byte_t(0xFF) }) == pdn::unicode::bom_type::utf32_be);
static_assert(pdn::unicode::check_bom(4, { pdn::unicode::bom::byte_t(0xFF), pdn::unicode::bom::byte_t(0xFE),
	pdn::unicode::bom::byte_t(0x41), pdn::unicode::bom::byte_t(0x00) }) == pdn::unicode::bom_type::utf16_le);
static_assert(pdn::unicode::check_bom(4, { pdn::unicode::bom::byte_t(0x41), pdn::unicode::bom::byte_t(0x00),
	pdn::unicode::bom::byte_t(0x00), pdn::unicode::bom::byte_t(0x00) }) == pdn::unicode::bom_type::no_bom);

namespace
{
	struct bom_case
	{
		const char*                    name;
		std::string                    bytes;
		pdn::unicode::bom_type         want;
		std::filesystem::path          file;
	};

	void test_detect_and_rewind()
	{
		using pdn::unicode::bom_type;
		using pdn::unicode::to_byte_size;

		const bom_case cases[] = {
			{ "empty", "", bom_type::no_bom, {} },
			{ "one byte", "a", bom_type::no_bom, {} },
			{ "three bytes", "abc", bom_type::no_bom, {} },
			{ "four bytes no bom", "abcd", bom_type::no_bom, {} },
			{ "utf8 bom + A", utf8_bom + "A", bom_type::utf8, {} },
			{ "utf8 bom only", utf8_bom, bom_type::utf8, {} },
			{ "utf16le bom + A0", utf16le_bom + std::string("\x41\x00", 2), bom_type::utf16_le, {} },
			{ "utf16be bom + 0A", utf16be_bom + std::string("\x00\x41", 2), bom_type::utf16_be, {} },
			{ "utf16le bom only", utf16le_bom, bom_type::utf16_le, {} },
			{ "utf16le bom + 3rd", utf16le_bom + std::string("\x00", 1), bom_type::utf16_le, {} },
			{ "utf32le bom + A", utf32le_bom + "A", bom_type::utf32_le, {} },
			{ "utf32be bom + A", utf32be_bom + "A", bom_type::utf32_be, {} },
			{ "utf32le bom only", utf32le_bom, bom_type::utf32_le, {} },
			{ "utf32be bom only", utf32be_bom, bom_type::utf32_be, {} },
		};

		// Write next to the test executable (the build tree): the system temp
		// directory is not guaranteed to be writable for the test process.
		const std::filesystem::path path = "pdn_bom_reader_test.bin";

		for (auto const& c : cases)
		{
			{
				std::ofstream out(path, std::ios::binary | std::ios::trunc);
				out.write(c.bytes.data(), static_cast<std::streamsize>(c.bytes.size()));
				check(out.good(), "write test file", __LINE__);
			}

			std::ifstream in(path, std::ios::binary);
			check(in.is_open(), "open test file", __LINE__);
			const bom_type got = pdn::unicode::read_bom(in);
			const std::string rest{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>{} };

			check(got == c.want, c.name, __LINE__);
			check(rest == c.bytes.substr(to_byte_size(c.want)), c.name, __LINE__);
		}

		std::filesystem::remove(path);
	}
}

int main()
{
	using pdn::unicode::bom_type;

	// A structural byte stream: same results, no std::istream involved.
	{
		byte_input in{ utf16le_bom + std::string("\x41\x00\x42\x00", 4) };
		const bom_type got = pdn::unicode::read_bom(in);
		check(got == bom_type::utf16_le, "byte_input bom", __LINE__);
		check(in.rest() == std::string("\x41\x00\x42\x00", 4), "byte_input rewind", __LINE__);
	}
	{
		byte_input in{ "abcd" };
		const bom_type got = pdn::unicode::read_bom(in);
		check(got == bom_type::no_bom, "byte_input no bom", __LINE__);
		check(in.rest() == "abcd", "byte_input no bom rewind", __LINE__);
	}

	// A stream that is not good on entry is left alone and reported as no_bom.
	{
		std::istringstream in("abc");
		in.setstate(std::ios::badbit);
		const bom_type got = pdn::unicode::read_bom(in);
		check(got == bom_type::no_bom, "bad stream", __LINE__);
		check(in.bad(), "bad stream untouched", __LINE__);
	}

	// A stream already at eof yields no_bom and stays at eof.
	{
		std::istringstream in("");
		in.setstate(std::ios::eofbit);
		const bom_type got = pdn::unicode::read_bom(in);
		check(got == bom_type::no_bom, "eof stream", __LINE__);
	}

	test_detect_and_rewind();

	std::printf(failures == 0 ? "pdn_bom_reader: ok\n" : "pdn_bom_reader: FAILED\n");
	return failures == 0 ? 0 : 1;
}
