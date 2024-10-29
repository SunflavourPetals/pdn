#ifndef PDN_Header_pdn_parse
#define PDN_Header_pdn_parse

#include <concepts>
#include <type_traits>
#include <fstream>
#include <string>
#include <codecvt>

#include "pdn_unicode_base.h"
#include "pdn_exception.h"

#include "pdn_parser.h"
#include "pdn_dom.h"

namespace pdn::dev_util
{
	template <typename type, typename char_t>
	concept token_iterator = requires (type it)
	{
		{ *it } -> ::std::convertible_to<token<char_t>>;
		++it;
	};
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
	template <unicode::concepts::unicode_code_unit          char_t,
	          dev_util::token_iterator<char_t>              it_t,
	          dev_util::function_package_for_parser<char_t> fn_pkg>
	auto parse(it_t begin, auto end, fn_pkg& fp, char_t = {}) -> dom<char_t>
	{
		experimental::parser<char_t, fn_pkg> par{ fp };
		return par.parse(begin, end);
	}
	// for token iterator
	template <unicode::concepts::unicode_code_unit char_t,
	          dev_util::token_iterator<char_t>     it_t>
	auto parse(it_t begin, auto end, char_t target_char_v = {}) -> dom<char_t>
	{
		using fn_pkg = default_function_package<char_t>;
		fn_pkg fp{};
		return parse(::std::move(begin), ::std::move(end), fp, target_char_v);
	}
	// for code_unit iterator
	template <unicode::concepts::unicode_code_unit               char_t,
	          dev_util::utf_code_unit_iterator                   it_t,
	          dev_util::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          dev_util::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          dev_util::function_package_for_parser<char_t>      fn_pkg_for_parser>
	auto parse(it_t               begin,
	           auto               end,
	           fn_pkg_for_cp_it&  cp_it_fp,
	           fn_pkg_for_lexer&  lex_fp,
	           fn_pkg_for_parser& par_fp,
	           char_t             target_char_v = {}) -> dom<char_t>
	{
		experimental::lexer<char_t, fn_pkg_for_lexer>   lex{ lex_fp };
		auto cp_it     = make_code_point_iterator(begin, end, cp_it_fp);
		auto token_it  = experimental::make_token_iterator(lex, cp_it, end);
		auto token_end = experimental::make_end_token_iterator(token_it);
		return parse(::std::move(token_it), ::std::move(token_end), par_fp, target_char_v);
	}
	// for code_unit iterator
	template <unicode::concepts::unicode_code_unit char_t,
	          dev_util::utf_code_unit_iterator     it_t>
	auto parse(it_t begin, auto end, char_t target_char_v = {}) -> dom<char_t>
	{
		using fn_pkg = default_function_package<char_t>;
		fn_pkg fp{};
		return parse(::std::move(token_it), ::std::move(token_end), fp, fp, fp, target_char_v);
	}
	// for file stream
	template <unicode::concepts::unicode_code_unit               char_t,
	          dev_util::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          dev_util::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          dev_util::function_package_for_parser<char_t>      fn_pkg_for_parser>
	auto parse(::std::ifstream&   source_file,
	           fn_pkg_for_cp_it&  cp_it_fp,
	           fn_pkg_for_lexer&  lex_fp,
	           fn_pkg_for_parser& par_fp,
	           char_t             target_char_v = {},
	           ::std::size_t      buffer_size = 1024) -> dom<char_t>
	{
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
		default:        throw  inner_error{ "[pdn] inner error when process bom_type" };
		}
		return 0;
	}
	// for file stream
	template <unicode::concepts::unicode_code_unit char_t>
	auto parse(::std::ifstream& source_file, char_t target_char_v = {}, ::std::size_t buffer_size = 1024) -> dom<char_t>
	{
		using fn_pkg = default_function_package<char_t>;
		fn_pkg fp{};
		return parse(::std::move(token_it), ::std::move(token_end), fp, fp, fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::unicode_code_unit               char_t,
	          dev_util::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          dev_util::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          dev_util::function_package_for_parser<char_t>      fn_pkg_for_parser>
	auto parse(const ::std::string& filename,
               fn_pkg_for_cp_it&    cp_it_fp,
	           fn_pkg_for_lexer&    lex_fp,
	           fn_pkg_for_parser&   par_fp,
	           char_t               target_char_v = {},
	           ::std::size_t        buffer_size = 1024) -> dom<char_t>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		if (!source_file.is_open())
		{
			using namespace ::std::string_literals;
			throw failed_in_open_file_error{ "failed in open file \""s + filename + "\""s };
		}
		return parse(source_file, cp_it_fp, lex_fp, par_fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::unicode_code_unit char_t>
	auto parse(const ::std::string& filename, char_t target_char_v = {}, ::std::size_t buffer_size = 1024) -> dom<char_t>
	{
		using fn_pkg = default_function_package<char_t>;
		fn_pkg fp{};
		return parse(filename, fp, fp, fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::unicode_code_unit               char_t,
	          dev_util::function_package_for_code_point_iterator fn_pkg_for_cp_it,
	          dev_util::function_package_for_lexer<char_t>       fn_pkg_for_lexer,
	          dev_util::function_package_for_parser<char_t>      fn_pkg_for_parser>
	auto parse(const ::std::wstring& filename,
               fn_pkg_for_cp_it&     cp_it_fp,
	           fn_pkg_for_lexer&     lex_fp,
	           fn_pkg_for_parser&    par_fp,
	           char_t                target_char_v = {},
	           ::std::size_t         buffer_size = 1024) -> dom<char_t>
	{
		::std::ifstream source_file(filename, ::std::ios::in | ::std::ios::binary);
		if (!source_file.is_open())
		{
			auto& facet = std::use_facet<::std::codecvt<wchar_t, char, ::std::mbstate_t>>(::std::locale());
			::std::mbstate_t mb{};
			::std::string external(filename.size() * facet.max_length(), '\0');
			const wchar_t* from_next{};
			char* to_next{};
			facet.out(mb,
			          filename.data(),
			          filename.data() + filename.size(),
			          from_next,
			          &external[0],
			          &external[external.size()],
			          to_next);
			// skip result checking
			external.resize(to_next - &external[0]);
			using namespace ::std::string_literals;
			throw failed_in_open_file_error{ "failed in open file \""s + external + "\""s };
		}
		return parse(source_file, cp_it_fp, lex_fp, par_fp, target_char_v, buffer_size);
	}
	// for filename
	template <unicode::concepts::unicode_code_unit char_t>
	auto parse(const ::std::wstring& filename, char_t target_char_v = {}, ::std::size_t buffer_size = 1024) -> dom<char_t>
	{
		using fn_pkg = default_function_package<char_t>;
		fn_pkg fp{};
		return parse(filename, fp, fp, fp, target_char_v, buffer_size);
	}
}

#endif
