#ifndef PDN_Header_pdn_parse
#define PDN_Header_pdn_parse

#include <concepts>
#include <cassert>
#include <utility>
#include <cstddef>
#include <fstream>
#include <string>
#include <iterator>
#include <filesystem>

#include "pdn_unicode_base.h"

#include "pdn_bom_reader.h"
#include "pdn_byte_swap_chain.h"
#include "pdn_code_unit_iterator.h"
#include "pdn_code_point_iterator.h"
#include "pdn_lexer.h"
#include "pdn_parser.h"
#include "pdn_function_package.h"
#include "pdn_entity.h"
#include "pdn_parser_utility.h"
#include "pdn_stop_reason.h"
#include "pdn_parse_error_code.h"
#include "pdn_parse_result.h"

// The pdn::parse family: from a token stream, a UTF code unit stream or a byte stream
// (memory, std::istream, file, file name) to an entity. Each overload delegates to the
// next lower level, so the versions differ only in where the input comes from and which
// function packages the caller can supply. char_t names the encoding of the parsed
// entity: pass pdn::utf8_tag / utf16_tag / utf32_tag (or any char_t value) to deduce it.
//
namespace pdn::detail
{
	template <typename stream>
	concept stream_constructible_from_wchar_cstring = ::std::constructible_from<stream, const wchar_t*>;

	template <typename stream>
	concept stream_constructible_from_wchar_string = ::std::constructible_from<stream, const ::std::wstring&>;

	// The wide-character file stream constructors exist only where the platform provides
	// them; constraining the opener keeps the wstring / const wchar_t* parse overloads out
	// of overload resolution elsewhere.
	template <stream_constructible_from_wchar_cstring stream, typename dummy>
	stream open_stream_by_wchar_cstring(const wchar_t* filename, ::std::ios::openmode mode) { return stream(filename, mode); }

	template <stream_constructible_from_wchar_string stream, typename dummy>
	stream open_stream_by_wchar_string(const ::std::wstring& filename, ::std::ios::openmode mode) { return stream(filename, mode); }
}

// Encoding tags. Passing one of these as the last parse argument makes char_t deducible
// without spelling parse<char8_t>(...); the objects carry no data, only the character type.
namespace pdn::inline utf_tag
{
	inline constexpr unicode::u8char_t  utf8_tag{};
	inline constexpr unicode::u16char_t utf16_tag{};
	inline constexpr unicode::u32char_t utf32_tag{};
}

namespace pdn
{
	// for token iterator
	template <
		unicode::concepts::code_unit                  char_t,
		concepts::token_iterator<char_t>              it_t,
		concepts::function_package_for_parser<char_t> fn_pkg>
	[[nodiscard]] auto parse(it_t begin, auto end, fn_pkg& fp, char_t = {}) -> parse_result<char_t>
	{
		// a package or iterator that is already stopped never starts a parse
		if constexpr (concepts::stoppable<it_t>) if (begin.stop_requested()) [[unlikely]]
		{
			return parse_result<char_t>{ to_parse_error_code(begin.stop_reason()) };
		}
		parser<char_t, fn_pkg> par{ fp };
		auto result = par.parse(begin, end);

		if (par.stop_requested()) [[unlikely]]
		{
			return { to_parse_error_code(par.stop_reason()), ::std::move(result) };
		}
		if (fp.error_count()) [[unlikely]]
		{
			return { parse_error_code::parse_error, ::std::move(result) };
		}

		return parse_result<char_t>{ ::std::move(result) };
	}
	// for token iterator
	template <
		unicode::concepts::code_unit     char_t,
		concepts::token_iterator<char_t> it_t>
	[[nodiscard]] auto parse(it_t begin, auto end, char_t char_tag = {}) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(::std::move(begin), ::std::move(end), fp, char_tag);
	}
	// for code_unit iterator
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::utf_code_unit_iterator                   it_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		it_t               begin,
		auto               end,
		fn_pkg_for_cp_it&  cp_it_fp,
		fn_pkg_for_lexer&  lex_fp,
		fn_pkg_for_parser& par_fp,
		char_t             char_tag = {}) -> parse_result<char_t>
	{
		// a package or iterator that is already stopped never starts a parse
		if constexpr (concepts::stoppable<it_t>) if (begin.stop_requested()) [[unlikely]]
		{
			return parse_result<char_t>{ to_parse_error_code(begin.stop_reason()) };
		}
		auto cp_it = make_code_point_iterator(::std::move(begin), end, cp_it_fp);
		if constexpr (concepts::stoppable<decltype(cp_it)>) if (cp_it.stop_requested()) [[unlikely]]
		{
			return parse_result<char_t>{ to_parse_error_code(cp_it.stop_reason()) };
		}
		lexer<char_t, fn_pkg_for_lexer, decltype(cp_it)> lex{ lex_fp };
		auto token_it = make_token_iterator(::std::move(lex), ::std::move(cp_it), end);
		auto token_end = make_token_iterator_end();
		auto result = parse(::std::move(token_it), ::std::move(token_end), par_fp, char_tag);
		// The parser inspects only its own package. Errors recorded by the lexer or by the
		// code point iterator are folded into parse_error here.
		if (result && (lex_fp.error_count() || cp_it_fp.error_count())) [[unlikely]]
		{
			return { parse_error_code::parse_error, ::std::move(result.value()) };
		}
		return result;
	}
	// for code_unit iterator
	template <
		unicode::concepts::code_unit     char_t,
		concepts::utf_code_unit_iterator it_t>
	[[nodiscard]] auto parse(it_t begin, auto end, char_t char_tag = {}) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(::std::move(begin), ::std::move(end), fp, fp, fp, char_tag);
	}
	// for utf_code_unit_string_view
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::utf_code_unit_string_view                str_view_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		str_view_t const&  sv,
		fn_pkg_for_cp_it&  cp_it_fp,
		fn_pkg_for_lexer&  lex_fp,
		fn_pkg_for_parser& par_fp,
		char_t             char_tag = {}) -> parse_result<char_t>
	{
		return parse(::std::cbegin(sv), ::std::cend(sv), cp_it_fp, lex_fp, par_fp, char_tag);
	}
	// for utf_code_unit_string_view
	template <
		unicode::concepts::code_unit        char_t,
		concepts::utf_code_unit_string_view str_view_t>
	[[nodiscard]] auto parse(str_view_t const& sv, char_t char_tag = {}) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(sv, fp, fp, fp, char_tag);
	}
	// for input byte stream
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		::std::istream&      source_input_stream,
		unicode::encode_type source_encode_type,
		fn_pkg_for_cp_it&    cp_it_fp,
		fn_pkg_for_lexer&    lex_fp,
		fn_pkg_for_parser&   par_fp,
		char_t               char_tag = {},
		::std::size_t        buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		// The stream is read as-is: only the file stream overload below looks for a BOM, so
		// the caller decides where the input starts. A leading U+FEFF is not stripped here;
		// the lexer treats it as a separator, so UTF-8 input usually parses anyway.
		// bad -> null; good | fail | eof -> continue
		if (source_input_stream.bad()) [[unlikely]]
		{
			return parse_result<char_t>{ parse_error_code::bad_stream };
		}

		auto swap_chain = make_byte_swap_chain(source_input_stream, buffer_size); // requires source_file is opened and not bad
		auto byte_it = make_byte_iterator(::std::move(swap_chain));
		auto end = make_byte_iterator_end();
		using enum unicode::encode_type;
		switch (source_encode_type)
		{
		case utf8:     return parse(make_code_unit_iterator<utf8>    (::std::move(byte_it), end), end, cp_it_fp, lex_fp, par_fp, char_tag);
		case utf16_le: return parse(make_code_unit_iterator<utf16_le>(::std::move(byte_it), end), end, cp_it_fp, lex_fp, par_fp, char_tag);
		case utf16_be: return parse(make_code_unit_iterator<utf16_be>(::std::move(byte_it), end), end, cp_it_fp, lex_fp, par_fp, char_tag);
		case utf32_le: return parse(make_code_unit_iterator<utf32_le>(::std::move(byte_it), end), end, cp_it_fp, lex_fp, par_fp, char_tag);
		case utf32_be: return parse(make_code_unit_iterator<utf32_be>(::std::move(byte_it), end), end, cp_it_fp, lex_fp, par_fp, char_tag);
		}
		// The switch handles every encode_type enumerator, which is why no default: is written
		// (a future enumerator must make -Wswitch complain instead of silently falling back).
		// The tail only gives the function a return path where the assert is compiled out.
#ifdef __cpp_lib_unreachable
		::std::unreachable();
#else
		assert(0 && "[pdn] inner error in pdn::parse: unknown encode_type"); // unreachable
#endif
		return parse_result<char_t>{ parse_error_code::parse_error }; // unreachable
	}
	// for input byte stream
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(
		::std::istream&      source_input_stream,
		unicode::encode_type source_encode_type,
		char_t               char_tag = {},
		::std::size_t        buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(source_input_stream, source_encode_type, fp, fp, fp, char_tag, buffer_size);
	}
	// for file stream
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		::std::ifstream&   source_file,
		fn_pkg_for_cp_it&  cp_it_fp,
		fn_pkg_for_lexer&  lex_fp,
		fn_pkg_for_parser& par_fp,
		char_t             char_tag = {},
		::std::size_t      buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		// !is_open | bad -> null; good | fail | eof -> continue
		if (!source_file.is_open()) [[unlikely]]
		{
			return parse_result<char_t>{ parse_error_code::open_file_failed };
		}
		else if (source_file.bad()) [[unlikely]]
		{
			return parse_result<char_t>{ parse_error_code::bad_stream };
		}
		// clear fail bit -> good | eof
		source_file.clear(source_file.rdstate() & std::ios::eofbit);
		auto bom_code = unicode::read_bom(source_file); // read source_file and maybe seekg
		// bad | (fail & !eof) -> null; good | (fail & eof) -> continue
		if (source_file.bad()) [[unlikely]]
		{
			return parse_result<char_t>{ parse_error_code::bad_stream };
		}
		else if (source_file.fail() && !source_file.eof()) [[unlikely]]
		{
			return parse_result<char_t>{ parse_error_code::stream_operation_failed };
		}
		source_file.clear(source_file.rdstate() & std::ios::eofbit); // clear fail bit -> good | eof
		auto enc = unicode::to_encode_type(bom_code);
		return parse(static_cast<::std::istream&>(source_file), enc, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for file stream
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(
		::std::ifstream& source_file,
		char_t           char_tag = {},
		::std::size_t    buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(source_file, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		const ::std::string& filename,
		fn_pkg_for_cp_it&    cp_it_fp,
		fn_pkg_for_lexer&    lex_fp,
		fn_pkg_for_parser&   par_fp,
		char_t               char_tag = {},
		::std::size_t        buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(
		const ::std::string& filename,
		char_t               char_tag = {},
		::std::size_t        buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{ ::std::filesystem::path{ filename }.u8string() };
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		const char* const    filename,
		fn_pkg_for_cp_it&    cp_it_fp,
		fn_pkg_for_lexer&    lex_fp,
		fn_pkg_for_parser&   par_fp,
		char_t               char_tag = {},
		::std::size_t        buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(
		const char* const filename,
		char_t            char_tag = {},
		::std::size_t     buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{ ::std::filesystem::path{ filename }.u8string() };
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		const ::std::wstring& filename,
		fn_pkg_for_cp_it&     cp_it_fp,
		fn_pkg_for_lexer&     lex_fp,
		fn_pkg_for_parser&    par_fp,
		char_t                char_tag = {},
		::std::size_t         buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		auto source_file = detail::open_stream_by_wchar_string<::std::ifstream, char_t>(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(
		const ::std::wstring& filename,
		char_t                char_tag = {},
		::std::size_t         buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{ ::std::filesystem::path{ filename }.u8string() };
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(
		const wchar_t* const  filename,
		fn_pkg_for_cp_it&     cp_it_fp,
		fn_pkg_for_lexer&     lex_fp,
		fn_pkg_for_parser&    par_fp,
		char_t                char_tag = {},
		::std::size_t         buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		auto source_file = detail::open_stream_by_wchar_cstring<::std::ifstream, char_t>(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(
		const wchar_t* const  filename,
		char_t                char_tag = {},
		::std::size_t         buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{ ::std::filesystem::path{ filename }.u8string() };
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <
		unicode::concepts::code_unit                       char_t,
		concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
		concepts::function_package_for_lexer               fn_pkg_for_lexer,
		concepts::function_package_for_parser<char_t>      fn_pkg_for_parser,
		::std::same_as<::std::filesystem::path>            path_t>
	[[nodiscard]] auto parse(
		const path_t&      filename,
		fn_pkg_for_cp_it&  cp_it_fp,
		fn_pkg_for_lexer&  lex_fp,
		fn_pkg_for_parser& par_fp,
		char_t             char_tag = {},
		::std::size_t      buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t, ::std::same_as<::std::filesystem::path> path_t>
	[[nodiscard]] auto parse(
		const path_t& filename,
		char_t        char_tag = {},
		::std::size_t buffer_size = detail::default_swap_chain_buffer_size) -> parse_result<char_t>
	{
		default_function_package<char_t> fp{ filename.u8string() };
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
}

#endif
