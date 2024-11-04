#ifndef PDN_Header_pdn_parse
#define PDN_Header_pdn_parse

#include <type_traits>
#include <concepts>
#include <fstream>
#include <string>
#include <optional>

#include "pdn_unicode_base.h"
#include "pdn_exception.h"

#include "pdn_bom_reader.h"
#include "pdn_swap_chain.h"
#include "pdn_code_unit_iterator.h"
#include "pdn_code_point_iterator.h"
#include "pdn_lexer.h"
#include "pdn_parser.h"
#include "pdn_function_package.h"
#include "pdn_dom.h"

namespace pdn::dev_util
{
	template <typename type, typename target_type>
	concept remove_cvref_same_as = ::std::is_same_v<::std::remove_cvref_t<type>, ::std::remove_cvref_t<target_type>>;
	template <typename type>
	concept utf_8_code_unit_iterator = requires (type it)
	{
		{ *it } -> remove_cvref_same_as<unicode::utf_8_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_16_code_unit_iterator = requires (type it)
	{
		{ *it } -> remove_cvref_same_as<unicode::utf_16_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_32_code_unit_iterator = requires (type it)
	{
		{ *it } -> remove_cvref_same_as<unicode::utf_32_code_unit_t>;
		++it;
	};
	template <typename type>
	concept utf_code_unit_iterator
		 = utf_8_code_unit_iterator<type>
		|| utf_16_code_unit_iterator<type>
		|| utf_32_code_unit_iterator<type>;
}

namespace pdn
{
	inline constexpr unicode::utf_8_code_unit_t  to_utf_8{};
	inline constexpr unicode::utf_16_code_unit_t to_utf_16{};
	inline constexpr unicode::utf_32_code_unit_t to_utf_32{};

	// for token iterator
	template <unicode::concepts::code_unit                  char_t,
	          dev_util::token_iterator<char_t>              it_t,
	          concepts::function_package_for_parser<char_t> fn_pkg>
	[[nodiscard]] auto parse(it_t begin, auto end, fn_pkg& fp, char_t = {}) -> dom<char_t>
	{
		parser<char_t, fn_pkg> par{ fp };
		return par.parse(begin, end);
	}
	// for token iterator
	template <unicode::concepts::code_unit         char_t,
	          dev_util::token_iterator<char_t>     it_t>
	[[nodiscard]] auto parse(it_t begin, auto end, char_t target_char_v = {}) -> dom<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(::std::move(begin), ::std::move(end), fp, target_char_v);
	}
	// for code_unit iterator
	template <unicode::concepts::code_unit                       char_t,
	          dev_util::utf_code_unit_iterator                   it_t,
	          concepts::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          concepts::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          concepts::function_package_for_parser<char_t>      fn_pkg_for_parser>
	[[nodiscard]] auto parse(it_t               begin,
	                         auto               end,
	                         fn_pkg_for_cp_it&  cp_it_fp,
	                         fn_pkg_for_lexer&  lex_fp,
	                         fn_pkg_for_parser& par_fp,
	                         char_t             target_char_v = {}) -> dom<char_t>
	{
		lexer<char_t, fn_pkg_for_lexer> lex{ lex_fp };
		auto cp_it     = make_code_point_iterator(begin, end, cp_it_fp);
		auto token_it  = make_token_iterator(lex, cp_it, end);
		auto token_end = make_end_token_iterator(token_it);
		return parse(::std::move(token_it), ::std::move(token_end), par_fp, target_char_v);
	}
	// for code_unit iterator
	template <unicode::concepts::code_unit         char_t,
	          dev_util::utf_code_unit_iterator     it_t>
	[[nodiscard]] auto parse(it_t begin, auto end, char_t target_char_v = {}) -> dom<char_t>
	{
		default_function_package<char_t> fp{};
		return parse(::std::move(begin), ::std::move(end), fp, fp, fp, target_char_v);
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
	                         char_t             target_char_v = {},
	                         ::std::size_t      buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		if (!source_file.is_open() || source_file.bad())
		{
			return ::std::nullopt;
		}
		auto bom_type           = unicode::read_bom(source_file);
		auto source_encode_type = unicode::utility::to_encode_type(bom_type);
		auto source_swap_chain  = make_swap_chain(source_file, buffer_size);
		using enum unicode::encode_type;

		auto& sw = source_swap_chain;
		switch (source_encode_type)
		{
		case utf_8:     return parse(make_code_unit_iterator<utf_8>    (sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, target_char_v);
		case utf_16_le: return parse(make_code_unit_iterator<utf_16_le>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, target_char_v);
		case utf_16_be: return parse(make_code_unit_iterator<utf_16_be>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, target_char_v);
		case utf_32_le: return parse(make_code_unit_iterator<utf_32_le>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, target_char_v);
		case utf_32_be: return parse(make_code_unit_iterator<utf_32_be>(sw.current(), sw.end()), sw.end(), cp_it_fp, lex_fp, par_fp, target_char_v);
		default:        throw  inner_error{ "[pdn] inner error in pdn::parse: unknown bom_type" };
		}
		return ::std::nullopt;
	}
	// for file stream
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(::std::ifstream& source_file,
	                         char_t           target_char_v = {},
	                         ::std::size_t    buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(source_file, fp, fp, fp, target_char_v, buffer_size);
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
	                         char_t               target_char_v = {},
	                         ::std::size_t        buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const ::std::string& filename,
	                         char_t               target_char_v = {},
	                         ::std::size_t        buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, target_char_v, buffer_size);
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
	                         char_t               target_char_v = {},
	                         ::std::size_t        buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const char* const filename,
	                         char_t            target_char_v = {},
	                         ::std::size_t     buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, target_char_v, buffer_size);
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
	                         char_t                target_char_v = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const ::std::wstring& filename,
	                         char_t                target_char_v = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, target_char_v, buffer_size);
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
	                         char_t                target_char_v = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		return parse(source_file, cp_it_fp, lex_fp, par_fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::code_unit char_t>
	[[nodiscard]] auto parse(const wchar_t* const  filename,
	                         char_t                target_char_v = {},
	                         ::std::size_t         buffer_size = 1024) -> ::std::optional<dom<char_t>>
	{
		default_function_package<char_t> fp{};
		return parse(filename, fp, fp, fp, target_char_v, buffer_size);
	}
}

#endif
