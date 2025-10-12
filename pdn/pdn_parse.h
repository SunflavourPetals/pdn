#ifndef PDN_Header_pdn_parse
#define PDN_Header_pdn_parse

#include <type_traits>
#include <cassert>
#include <utility>
#include <cstddef>
#include <fstream>
#include <string>
#include <optional>

#include "pdn_unicode_base.h"

#include "pdn_bom_reader.h"
#include "pdn_swap_chain.h"
#include "pdn_code_unit_iterator.h"
#include "pdn_code_point_iterator.h"
#include "pdn_lexer.h"
#include "pdn_parser.h"
#include "pdn_function_package.h"
#include "pdn_data_entity.h"
#include "pdn_parser_utility.h"

namespace pdn
{
	inline constexpr unicode::u8char_t  utf8_tag{};
	inline constexpr unicode::u16char_t utf16_tag{};
	inline constexpr unicode::u32char_t utf32_tag{};

	// for token iterator
	template <unicode::concepts::code_unit                  char_t,
	          concepts::token_iterator<char_t>              it_t,
	          concepts::function_package_for_parser<char_t> fn_pkg>
	[[nodiscard]] auto parse(it_t begin, auto end, fn_pkg& fp, char_t = {}) -> data_entity<char_t>
	{
		parser<char_t, fn_pkg> par{ fp };
		return par.parse(begin, end);
	}
	// for token iterator
	template <unicode::concepts::code_unit     char_t,
	          concepts::token_iterator<char_t> it_t>
	[[nodiscard]] auto parse(it_t begin, auto end, char_t char_tag = {}) -> data_entity<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(::std::move(begin), ::std::move(end), fp, char_tag);
	}
	// for code_unit iterator
	template <unicode::concepts::code_unit                       char_t,
	          concepts::utf_code_unit_iterator                   it_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(it_t               begin,
	                         auto               end,
	                         fn_pkg_for_cp_it&  cp_it_fp,
	                         fn_pkg_for_lexer&  lex_fp,
	                         fn_pkg_for_parser& par_fp,
	                         char_t             char_tag = {}) -> data_entity<char_t>
	{
		lexer<char_t, fn_pkg_for_lexer> lex{ lex_fp };
		auto cp_it     = make_code_point_iterator(begin, end, cp_it_fp);
		auto token_it  = make_token_iterator(lex, cp_it, end);
		auto token_end = make_end_token_iterator(token_it);
		return parse(::std::move(token_it), ::std::move(token_end), par_fp, char_tag);
	}
	// for code_unit iterator
	template <unicode::concepts::code_unit     char_t,
	          concepts::utf_code_unit_iterator it_t>
	[[nodiscard]] auto parse(it_t begin, auto end, char_t char_tag = {}) -> data_entity<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(::std::move(begin), ::std::move(end), fp, fp, fp, char_tag);
	}
	// for utf_code_unit_string_view
	template <unicode::concepts::code_unit                       char_t,
	          concepts::utf_code_unit_string_view                str_view_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(str_view_t         sv,
	                         fn_pkg_for_cp_it&  cp_it_fp,
	                         fn_pkg_for_lexer&  lex_fp,
	                         fn_pkg_for_parser& par_fp,
	                         char_t             char_tag = {}) -> data_entity<char_t>
	{
		lexer<char_t, fn_pkg_for_lexer> lex{ lex_fp };
		auto cp_it     = make_code_point_iterator(sv.cbegin(), sv.cend(), cp_it_fp);
		auto token_it  = make_token_iterator(lex, cp_it, sv.cend());
		auto token_end = make_end_token_iterator(token_it);
		return parse(::std::move(token_it), ::std::move(token_end), par_fp, char_tag);
	}
	// for utf_code_unit_string_view
	template <unicode::concepts::code_unit        char_t,
	          concepts::utf_code_unit_string_view str_view_t>
	[[nodiscard]] auto parse(str_view_t sv, char_t char_tag = {}) -> data_entity<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(sv, fp, fp, fp, char_tag);
	}
	// for file stream
	template <unicode::concepts::code_unit                       char_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(::std::ifstream&   source_file,
	                         fn_pkg_for_cp_it&  cp_it_fp,
	                         fn_pkg_for_lexer&  lex_fp,
	                         fn_pkg_for_parser& par_fp,
	                         char_t             char_tag = {},
	                         ::std::size_t      buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		if (!source_file.is_open() || source_file.bad())
		{
			return ::std::nullopt;
		}
		auto bom_t = unicode::read_bom(source_file);
		auto enc   = unicode::to_encode_type(bom_t);
		auto sw    = make_swap_chain(source_file, buffer_size);
		using enum unicode::encode_type;

		switch (enc)
		{
		case utf8:     return parse(make_code_unit_iterator<utf8>    (sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, char_tag);
		case utf16_le: return parse(make_code_unit_iterator<utf16_le>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, char_tag);
		case utf16_be: return parse(make_code_unit_iterator<utf16_be>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, char_tag);
		case utf32_le: return parse(make_code_unit_iterator<utf32_le>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, char_tag);
		case utf32_be: return parse(make_code_unit_iterator<utf32_be>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, char_tag);
		default:        assert(0 && "[pdn] inner error in pdn::parse: unknown bom_type");
		}
		return ::std::nullopt;
	}
	// for file stream
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(::std::ifstream& source_file,
	                         char_t           char_tag = {},
	                         ::std::size_t    buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(source_file, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit                       char_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(const ::std::string& filename,
                             fn_pkg_for_cp_it&    cp_it_fp,
	                         fn_pkg_for_lexer&    lex_fp,
	                         fn_pkg_for_parser&   par_fp,
	                         char_t               char_tag = {},
	                         ::std::size_t        buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const ::std::string& filename,
	                         char_t               char_tag = {},
	                         ::std::size_t        buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit                       char_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(const char* const    filename,
                             fn_pkg_for_cp_it&    cp_it_fp,
	                         fn_pkg_for_lexer&    lex_fp,
	                         fn_pkg_for_parser&   par_fp,
	                         char_t               char_tag = {},
	                         ::std::size_t        buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const char* const filename,
	                         char_t            char_tag = {},
	                         ::std::size_t     buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit                       char_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(const ::std::wstring& filename,
                             fn_pkg_for_cp_it&     cp_it_fp,
	                         fn_pkg_for_lexer&     lex_fp,
	                         fn_pkg_for_parser&    par_fp,
	                         char_t                char_tag = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const ::std::wstring& filename,
	                         char_t                char_tag = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit                       char_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(const wchar_t* const  filename,
	                         fn_pkg_for_cp_it&     cp_it_fp,
	                         fn_pkg_for_lexer&     lex_fp,
	                         fn_pkg_for_parser&    par_fp,
	                         char_t                char_tag = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, char_tag, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const wchar_t* const  filename,
	                         char_t                char_tag = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<data_entity<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, char_tag, buffer_size);
	}
}

#endif
