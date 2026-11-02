#include <iostream>
#include <string>
#include <exception>
#include <filesystem>
#include <fstream>
#include <array>

#define PDN_ENABLE_ORDERED_MAP
#include "spdn.h"

// for compile test
#include "pdn_source_position_and_newline_recorder.h"

std::ostream & operator<<(std::ostream & os, const pdn::type::u8string & s)
{
	return os << std::string_view{ reinterpret_cast<const char*>(s.c_str()), s.size() };
}

void error_test()
{
	using path = std::filesystem::path;

	auto error_test_parse = [](path filename, bool out_to_file = false, bool serialize_test = false)
	{
		try
		{
			struct myfp : public pdn::default_function_package<char8_t>
			{
				myfp(path p) : default_function_package(p.u8string()), of{ p.parent_path() / p.stem().concat("_err_out.txt") } {}
				void handle_error(const pdn::error_message& msg)
				{
					default_function_package::handle_error(msg, of);
				}
				std::ofstream of;
			};
			auto fp = out_to_file ? std::make_unique<myfp>(filename) : nullptr;
			auto e = out_to_file
				? pdn::parse(filename, *fp, *fp, *fp, pdn::utf8_tag)
				: pdn::parse(filename, pdn::utf8_tag);
			if (!e.has_value())
			{
				std::cout << filename << ": parsing result has no value\n";
			}
			if (!std::filesystem::exists(filename))
			{
				std::cout << filename << ": file does not exist\n";
			}
			if (serialize_test && e.has_value())
			{
				auto serialize_filename = filename.parent_path() / filename.stem().concat("_serialize_test.spdn.txt");
				std::ofstream ofs(serialize_filename, std::ios::binary);
				if (ofs)
				{
					auto serialized = pdn::make_u8serializer().serialize(e->as_object());
					ofs << serialized;
				}
				else
				{
					std::cerr << "serialization_test: failed to open output file serialize_test.spdn\n";
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "error test terminated: " << filename << " - " << e.what() << "\n";
		}
		catch (...)
		{
			std::cerr << "error test failed with unknown exception: " << filename << "\n";
		}
	};

	auto error_test_root = path{ "..\\test\\error_test" };
	auto error_test_lexical_path = error_test_root / "lexical_error";
	auto error_test_syntax_path = error_test_root / "syntax_error";
	auto error_test_lexical_errors = std::array<path, 11>
	{
		"lexical_error_test.spdn",
		"lexical_error_test_character_missing_terminating_character.spdn",
		"lexical_error_test_d_seq_error_cannot_find_end_sign_1.spdn",
		"lexical_error_test_d_seq_error_cannot_find_end_sign_2.spdn",
		"lexical_error_test_escape_error_x_used_with_no_following_hex_digits.spdn",
		"lexical_error_test_identifier_raw_string_missing_terminating_sequence.spdn",
		"lexical_error_test_identifier_string_missing_terminating_character.spdn",
		"lexical_error_test_raw_string_missing_terminating_sequence.spdn",
		"lexical_error_test_string_missing_terminating_character.spdn",
		"lexical_error_test_unterminated_block_comment.spdn",
		"lexical_error_test_unterminated_nested_block_comment.spdn",
	};
	auto error_test_syntax_errors = std::array<path, 1>
	{
		"syntax_error_test.spdn",
	};
	auto error_test_utf_errors = std::array<path, 12>
	{
		"UTF-8_error/utf-8_decode_error_test.spdn",
		"UTF-8_error/utf-8_decode_error_test_eof-2.spdn",
		"UTF-8_error/utf-8_decode_error_test_eof-3.spdn",
		"UTF-8_error/utf-8_decode_error_test_eof-4.spdn",
		"UTF-8_error/utf-8_decode_error_test_eof-5.spdn",
		"UTF-8_error/utf-8_decode_error_test_eof-6.spdn",
		"UTF-16-BE_error/utf-16-be_decode_error_test.spdn",
		"UTF-16-BE_error/utf-16-be_decode_error_test-eof.spdn",
		"UTF-16-LE_error/utf-16-le_decode_error_test.spdn",
		"UTF-16-LE_error/utf-16-le_decode_error_test-eof.spdn",
		"UTF-32-BE_error/utf-32-be_decode_error_test.spdn",
		"UTF-32-LE_error/utf-32-le_decode_error_test.spdn",
	};

	std::system("chcp 65001");

	for (auto path : error_test_lexical_errors)
	{
		error_test_parse(error_test_lexical_path / path);
	}

	for (auto path : error_test_syntax_errors)
	{
		error_test_parse(error_test_syntax_path / path);
	}

	for (auto path : error_test_utf_errors)
	{
		error_test_parse(error_test_root / path);
	}
}

void serialization_test()
{
	std::cout << "serialization_test begin: parsing and serializing feature_test.spdn\n";
	using namespace std::string_view_literals;
	auto feature_test_path = std::filesystem::path{ "..\\test\\feature_test.spdn" };
	auto e_opt = pdn::parse(feature_test_path, pdn::utf8_tag);

	if (e_opt)
	{
		std::ofstream ofs("..\\test\\serialize_test.spdn", std::ios::binary);
		if (ofs)
		{
			auto serialized = pdn::make_u8serializer().serialize(e_opt->as_object());
			ofs << serialized;
		}
		else
		{
			std::cerr << "serialization_test: failed to open output file serialize_test.spdn\n";
		}
	}
	else
	{
		std::cerr << "serialization_test: parsing failed for " << feature_test_path << "\n";
	}
	std::cout << "serialization_test end\n";
}

template <typename t, bool stop_reasonable_v = true>
void inner_stoppable_static_assert_test()
{
	static_assert(pdn::concepts::stoppable<t>);
	static_assert(pdn::concepts::const_stoppable<t>);
	static_assert(!pdn::concepts::nonconst_stoppable<t>);

	if constexpr (stop_reasonable_v)
	{
		static_assert(pdn::concepts::stop_reasonable<t>);
		static_assert(pdn::concepts::const_stop_reasonable<t>);
		static_assert(!pdn::concepts::nonconst_stop_reasonable<t>);
	}
}

int main() try
{
	inner_stoppable_static_assert_test<pdn::swap_chain<>>();
	inner_stoppable_static_assert_test<pdn::swap_chain<std::ifstream>>();
	using byte_it_i = pdn::byte_iterator_from_swap_chain<std::istream, std::default_delete<char[]>>;
	inner_stoppable_static_assert_test<byte_it_i>();
	using byte_it_if = pdn::byte_iterator_from_swap_chain<std::ifstream, std::default_delete<char[]>>;
	inner_stoppable_static_assert_test<byte_it_if>();
	using cu_it_iu8 = pdn::code_unit_iterator<pdn::unicode::encode_type::utf8, byte_it_i>;
	using cu_it_iu16le = pdn::code_unit_iterator<pdn::unicode::encode_type::utf16_le, byte_it_i>;
	using cu_it_iu16be = pdn::code_unit_iterator<pdn::unicode::encode_type::utf16_be, byte_it_i>;
	using cu_it_iu32le = pdn::code_unit_iterator<pdn::unicode::encode_type::utf32_le, byte_it_i>;
	using cu_it_iu32be = pdn::code_unit_iterator<pdn::unicode::encode_type::utf32_be, byte_it_i>;
	//using cu_it_iunknown = pdn::code_unit_iterator<pdn::unicode::encode_type::unknown, byte_it_i>;
	using cu_it_ifu8 = pdn::code_unit_iterator<pdn::unicode::encode_type::utf8, byte_it_if>;
	using cu_it_ifu16le = pdn::code_unit_iterator<pdn::unicode::encode_type::utf16_le, byte_it_if>;
	using cu_it_ifu16be = pdn::code_unit_iterator<pdn::unicode::encode_type::utf16_be, byte_it_if>;
	using cu_it_ifu32le = pdn::code_unit_iterator<pdn::unicode::encode_type::utf32_le, byte_it_if>;
	using cu_it_ifu32be = pdn::code_unit_iterator<pdn::unicode::encode_type::utf32_be, byte_it_if>;
	//using cu_it_ifunknown = pdn::code_unit_iterator<pdn::unicode::encode_type::unknown, byte_it_if>;
	inner_stoppable_static_assert_test<cu_it_iu8>();
	inner_stoppable_static_assert_test<cu_it_iu16le>();
	inner_stoppable_static_assert_test<cu_it_iu16be>();
	inner_stoppable_static_assert_test<cu_it_iu32le>();
	inner_stoppable_static_assert_test<cu_it_iu32be>();
	inner_stoppable_static_assert_test<cu_it_ifu8>();
	inner_stoppable_static_assert_test<cu_it_ifu16le>();
	inner_stoppable_static_assert_test<cu_it_ifu16be>();
	inner_stoppable_static_assert_test<cu_it_ifu32le>();
	inner_stoppable_static_assert_test<cu_it_ifu32be>();

	// error_test();
	serialization_test();
	using namespace std::string_view_literals;
	auto spdn_content = u8R"(hello: "Hello world!")"sv;
	auto e_opt = pdn::parse(spdn_content, pdn::utf8_tag);
	if (!e_opt)
	{
		return 0;
	}
	auto& e = *e_opt;
	std::cout << e[u8"hello"sv].as_string() << "\n";
	std::cout << pdn::make_u8serializer().serialize(e.as_object()) << "\n";
}
catch (std::exception& e)
{
	std::cerr << e.what() << "\n";
}
catch (...)
{
	std::cerr << "unknown exception\n";
}