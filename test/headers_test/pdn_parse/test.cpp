// Tests for the pdn::parse overload family.
//
// The overloads differ only in where the input comes from: a token stream, a UTF code unit
// stream (iterator pair or string view), a byte stream, a file stream or a file name. Every
// version returns parse_result<char_t> and the tests below check the delegated layers still
// behave: code unit level builds the code point iterator and the lexer, the byte level takes
// an explicit encoding, the file level strips the BOM, and the name level reports a missing
// file as open_file_failed instead of throwing.

#include <pdn/pdn_parse.h>

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
	using u8char_t  = pdn::unicode::u8char_t;
	using u16char_t = pdn::unicode::u16char_t;
	using u32char_t = pdn::unicode::u32char_t;
	using u8sv      = ::std::u8string_view;

	::std::size_t failures = 0;

	void check(const bool condition, const char* what)
	{
		if (!condition)
		{
			++failures;
			::std::cout << "FAILED: " << what << '\n';
		}
	}

	// Minimal function package: records the position, collects the diagnostics instead of
	// printing them, and can stop parsing after the first one with a chosen reason.
	template <typename char_t, ::std::size_t max_depth = 256>
	struct package
	{
		pdn::source_position_recorder      recorder{};
		::std::vector<pdn::error_message>  errors{};
		::std::size_t                      limit{ 1000 };
		bool                               stop{};
		pdn::stop_reason_code              reason{};
		pdn::stop_reason_code              stop_on_error{ pdn::stop_reason_code::parse_error_limit };

		auto position() const noexcept -> pdn::source_position { return recorder.position(); }
		void update(const pdn::unicode::code_point_t c) noexcept { recorder.update(c); }
		auto error_count() const noexcept -> ::std::size_t { return errors.size(); }
		bool stop_requested() const noexcept { return stop; }
		auto stop_reason() const noexcept -> pdn::stop_reason_code { return reason; }

		static constexpr auto nesting_depth_limit() noexcept -> ::std::size_t { return max_depth; }

		void handle_error(pdn::error_message msg)
		{
			errors.push_back(::std::move(msg));
			if (errors.size() >= limit)
			{
				stop = true;
				reason = stop_on_error;
			}
		}

		static auto generate_error_message(pdn::raw_error_message raw) -> pdn::error_msg_string
		{
			return pdn::default_error_message_generator::generate_error_message(::std::move(raw));
		}
		static auto generate_type(const pdn::type::string<char_t>& iden) -> pdn::type_code
		{
			return pdn::type_generator_function<char_t>(iden);
		}
		static auto generate_constant(const pdn::unicode::u8string& iden) -> ::std::optional<pdn::entity<char_t>>
		{
			return pdn::constant_generator_std_function<char_t>(iden);
		}
	};

	void write_bytes(const ::std::filesystem::path& path, const void* data, const ::std::size_t size)
	{
		::std::ofstream out(path, ::std::ios::binary | ::std::ios::trunc);
		out.write(static_cast<const char*>(data), static_cast<::std::streamsize>(size));
		check(out.good(), "write test file");
	}

	// ---------------------------------------------------------------- string views

	void test_string_view_utf8()
	{
		const u8sv src = u8R"(a: 1; b: "x")";
		auto result = pdn::parse(src, pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code{}, "utf8 view succeeds");
		check(result ? result.value().as_object().size() == 2 : false, "utf8 view has two entities");
		check(result ? result.value().as_object().contains(u8"a") : false, "utf8 view keeps the key");

		auto explicit_char_t = pdn::parse<u8char_t>(src);
		check(explicit_char_t.error_code() == pdn::parse_error_code{}, "explicit char_t works");
		check(::std::same_as<decltype(result), pdn::parse_result<u8char_t>>, "view returns parse_result<char_t>");
	}

	void test_string_view_utf16_utf32()
	{
		const ::std::u16string_view src16 = u"a: 1";
		auto result16 = pdn::parse(src16, pdn::utf16_tag);
		check(result16.error_code() == pdn::parse_error_code{}, "utf16 view succeeds");
		check(::std::same_as<decltype(result16), pdn::parse_result<u16char_t>>, "utf16 view returns parse_result<char16_t>");
		check(result16 ? result16.value().as_object().contains(u"a") : false, "utf16 view keeps the key");

		const ::std::u32string_view src32 = U"a: 1";
		auto result32 = pdn::parse(src32, pdn::utf32_tag);
		check(result32.error_code() == pdn::parse_error_code{}, "utf32 view succeeds");
		check(result32 ? result32.value().as_object().contains(U"a") : false, "utf32 view keeps the key");
	}

	void test_code_unit_iterators()
	{
		const u8sv src = u8"a: [1, 2]";
		auto result = pdn::parse(::std::cbegin(src), ::std::cend(src), pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code{}, "iterator pair succeeds");
		check(result ? result.value().as_object().size() == 1 : false, "iterator pair parsed one entity");
	}

	// ---------------------------------------------------------------- token stream

	void test_token_iterator()
	{
		package<u8char_t> fp{};
		const u8sv src = u8"a: 1";
		auto cp_it = pdn::make_code_point_iterator(::std::cbegin(src), ::std::cend(src), fp);
		pdn::lexer<u8char_t, package<u8char_t>, decltype(cp_it)> engine{ fp };
		auto it = pdn::make_token_iterator(::std::move(engine), ::std::move(cp_it), ::std::cend(src));
		auto end = pdn::make_token_iterator_end();
		auto result = pdn::parse(::std::move(it), ::std::move(end), fp, pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code{}, "token iterator succeeds");
		check(result ? result.value().as_object().contains(u8"a") : false, "token iterator keeps the key");
	}

	// ---------------------------------------------------------------- byte streams

	void test_istream()
	{
		{
			::std::istringstream in{ ::std::string{ "a: 1" } };
			auto result = pdn::parse(in, pdn::unicode::encode_type::utf8, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "utf8 istream succeeds");
			check(result ? result.value().as_object().size() == 1 : false, "utf8 istream parsed one entity");
		}
		{
			// "a: 1" as UTF-16LE bytes, no BOM: the explicit encoding starts at the first code unit.
			const unsigned char bytes[] = { 'a', 0, ':', 0, ' ', 0, '1', 0 };
			:: std::istringstream in{ ::std::string{ reinterpret_cast<const char*>(bytes), sizeof(bytes) } };
			auto result = pdn::parse(in, pdn::unicode::encode_type::utf16_le, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "utf16le istream succeeds");
		}
		{
			::std::istringstream in{ ::std::string{ "a: 1" } };
			auto result = pdn::parse(in, pdn::unicode::encode_type::utf8, pdn::utf8_tag, 0);
			check(result.error_code() == pdn::parse_error_code{}, "buffer size 0 falls back to the default");
		}
		{
			::std::istringstream in{ ::std::string{ "a: 1" } };
			in.setstate(::std::ios::badbit);
			auto result = pdn::parse(in, pdn::unicode::encode_type::utf8, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code::bad_stream, "a bad stream stops with bad_stream");
		}
	}

	void test_ifstream_bom()
	{
		const auto dir = ::std::filesystem::current_path();
		const auto plain = dir / "pdn_parse_utf8.spdn";
		const auto bom8 = dir / "pdn_parse_utf8_bom.spdn";
		const auto bom16 = dir / "pdn_parse_utf16_bom.spdn";
		const auto empty = dir / "pdn_parse_empty.spdn";

		const char plain_bytes[] = "a: 1";
		const unsigned char bom8_bytes[] = { 0xEF, 0xBB, 0xBF, 'a', ':', ' ', '1' };
		const unsigned char bom16_bytes[] = { 0xFF, 0xFE, 'a', 0, ':', 0, ' ', 0, '1', 0 };
		write_bytes(plain, plain_bytes, sizeof(plain_bytes) - 1);
		write_bytes(bom8, bom8_bytes, sizeof(bom8_bytes));
		write_bytes(bom16, bom16_bytes, sizeof(bom16_bytes));
		write_bytes(empty, "", 0);

		{
			::std::ifstream in(plain, ::std::ios::binary);
			auto result = pdn::parse(in, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "file stream without BOM succeeds");
			check(result ? result.value().as_object().size() == 1 : false, "plain file parsed one entity");
		}
		{
			::std::ifstream in(bom8, ::std::ios::binary);
			auto result = pdn::parse(in, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "utf8 BOM file succeeds");
			check(result ? result.value().as_object().contains(u8"a") : false, "utf8 BOM is not data");
		}
		{
			::std::ifstream in(bom16, ::std::ios::binary);
			auto result = pdn::parse(in, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "utf16 BOM file succeeds");
			check(result ? result.value().as_object().contains(u8"a") : false, "utf16 BOM selects the encoding");
		}
		{
			::std::ifstream in(empty, ::std::ios::binary);
			auto result = pdn::parse(in, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "empty file succeeds");
			check(result ? result.value().as_object().empty() : false, "empty file has no entity");
		}
		{
			::std::ifstream in(dir / "pdn_parse_does_not_exist.spdn", ::std::ios::binary);
			auto result = pdn::parse(in, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code::open_file_failed, "a closed stream stops with open_file_failed");
		}

		::std::filesystem::remove(plain);
		::std::filesystem::remove(bom8);
		::std::filesystem::remove(bom16);
		::std::filesystem::remove(empty);
	}

	void test_filename_overloads()
	{
		const auto path = ::std::filesystem::current_path() / "pdn_parse_named.spdn";
		const char bytes[] = "a: 1";
		write_bytes(path, bytes, sizeof(bytes) - 1);

		{
			auto result = pdn::parse(path.string(), pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "std::string file name succeeds");
		}
		{
			auto result = pdn::parse(path.string().c_str(), pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "const char* file name succeeds");
		}
		{
			auto result = pdn::parse(path, pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "filesystem::path file name succeeds");
		}
		{
			auto result = pdn::parse(::std::filesystem::current_path() / "pdn_parse_absent.spdn", pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code::open_file_failed, "a missing file stops with open_file_failed");
		}
		if constexpr (pdn::detail::stream_constructible_from_wchar_string<::std::ifstream>)
		{
			auto result = pdn::parse(::std::filesystem::current_path().wstring() + L"/pdn_parse_named.spdn", pdn::utf8_tag);
			check(result.error_code() == pdn::parse_error_code{}, "wstring file name succeeds where supported");
		}

		::std::filesystem::remove(path);
	}

	// ---------------------------------------------------------------- custom packages and stops

	void test_custom_packages()
	{
		const u8sv src = u8"a: 1";
		package<u8char_t> cp_it_fp{};
		package<u8char_t> lex_fp{};
		package<u8char_t> par_fp{};
		auto result = pdn::parse(src, cp_it_fp, lex_fp, par_fp, pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code{}, "three distinct packages succeed");
		check(cp_it_fp.error_count() == 0 && lex_fp.error_count() == 0 && par_fp.error_count() == 0, "no diagnostics");
	}

	void test_lexer_error_is_folded()
	{
		// The parser accepts the unterminated string (the lexer reports the missing quote and
		// still hands out the text), so only the fold in the code unit overload turns the
		// result into a failure.
		const u8sv src = u8R"(a: "unterminated)";
		package<u8char_t> cp_it_fp{};
		package<u8char_t> lex_fp{};
		package<u8char_t> par_fp{};
		auto result = pdn::parse(src, cp_it_fp, lex_fp, par_fp, pdn::utf8_tag);
		check(lex_fp.error_count() > 0, "lexer reported the unterminated string");
		check(result.error_code() == pdn::parse_error_code::parse_error, "lexer errors become parse_error");
	}

	void test_user_cancelled()
	{
		package<u8char_t> fp{};
		fp.limit = 1;
		fp.stop_on_error = pdn::stop_reason_code::user_cancelled;
		const u8sv src = u8"a: ]";
		auto result = pdn::parse(src, fp, fp, fp, pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code::user_cancelled, "a package stop reason reaches parse_result");
	}

	void test_nesting_depth_stop()
	{
		package<u8char_t, 2> fp{};
		const u8sv src = u8"a: [[[1]]]";
		auto result = pdn::parse(src, fp, fp, fp, pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code::nesting_depth_exceed, "the package nesting limit stops parsing");
		check(fp.error_count() == 0, "the depth stop is a stop reason, not a diagnostic");
	}

	void test_filename_is_recorded()
	{
		pdn::default_function_package<u8char_t> fp{ u8"file.spdn" };
		// generate_error_message() renders the text only; the "<filename>: " prefix is added
		// by handle_error, so route the diagnostic through a stream to observe it.
		const pdn::error_message msg{
			pdn::lexical_error_code::unacceptable_character,
			pdn::source_position{},
			u8"detail text"
		};
		::std::ostringstream out{};
		fp.handle_error(msg, out);
		check(out.str().starts_with("file.spdn: "), "the default package prefixes the file name");
	}

	void test_error_file_reports_parse_error()
	{
		const auto path = ::std::filesystem::current_path() / "pdn_parse_bad.spdn";
		const char bytes[] = "a: ";
		write_bytes(path, bytes, sizeof(bytes) - 1);
		package<u8char_t> fp{};
		auto result = pdn::parse(path.string(), fp, fp, fp, pdn::utf8_tag);
		check(result.error_code() == pdn::parse_error_code::parse_error, "a bad file reports parse_error");
		check(fp.error_count() > 0, "the bad file produced a diagnostic");
		::std::filesystem::remove(path);
	}
}

int main()
{
	test_string_view_utf8();
	test_string_view_utf16_utf32();
	test_code_unit_iterators();
	test_token_iterator();
	test_istream();
	test_ifstream_bom();
	test_filename_overloads();
	test_custom_packages();
	test_lexer_error_is_folded();
	test_user_cancelled();
	test_nesting_depth_stop();
	test_filename_is_recorded();
	test_error_file_reports_parse_error();

	if (failures == 0)
	{
		::std::cout << "all pdn::parse checks passed\n";
		return 0;
	}
	::std::cout << failures << " check(s) failed\n";
	return 1;
}
