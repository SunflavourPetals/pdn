#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <pdn/pdn_ordered_map.h>
#define PDN_USER_OBJECT ::pdn::ordered_map
#include <pdn/spdn.h>

namespace
{
	std::size_t failures = 0;

	void check(bool condition, const std::string& what)
	{
		if (!condition)
		{
			++failures;
			std::cout << "FAILED: " << what << '\n';
		}
	}

	void error_test(const std::filesystem::path& test_root)
	{
		using path = std::filesystem::path;

		const auto error_root = test_root / "error_test";

		const std::array<path, 11> lexical_errors
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

		const std::array<path, 1> syntax_errors
		{
			"syntax_error_test.spdn",
		};

		const std::array<path, 12> utf_errors
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

		const auto check_fixture = [](const path& filename)
		{
			const auto name = filename.string();
			check(std::filesystem::exists(filename), name + ": fixture does not exist");

			const auto result = pdn::parse(filename, pdn::utf8_tag);
			check(result.failed(), name + ": expected a failed parse");
			check(result.error_code() == pdn::parse_error_code::parse_error,
				name + ": expected parse_error, got " + result.what());
		};

		for (const auto& name : lexical_errors)
		{
			check_fixture(error_root / "lexical_error" / name);
		}
		for (const auto& name : syntax_errors)
		{
			check_fixture(error_root / "syntax_error" / name);
		}
		for (const auto& name : utf_errors)
		{
			check_fixture(error_root / name);
		}
	}

	void serialization_test(const std::filesystem::path& test_root)
	{
		const auto feature_dir = test_root / "feature_test";
		const auto feature_filename = feature_dir / "feature_test.spdn";
		const auto serialize_filename = feature_dir / "serialize_test.spdn";
		const auto null_filename = feature_dir / "null_test.spdn";

		const auto parsed = pdn::parse(feature_filename, pdn::utf8_tag);
		check(parsed.success(), std::string{ "feature_test.spdn: " } + parsed.what());
		if (parsed.failed())
		{
			return;
		}

		const auto& object = parsed.value().as_object();
		check(object.size() == 15, "feature_test.spdn: expected 15 top-level members");

		const auto serialized = pdn::make_u8serializer().serialize(object);

		std::ofstream out(serialize_filename, std::ios::binary | std::ios::trunc);
		check(static_cast<bool>(out),
			"serialize_test.spdn: failed to open output file " + serialize_filename.string());
		if (out)
		{
			out.write(reinterpret_cast<const char*>(serialized.data()),
				static_cast<std::streamsize>(serialized.size()));
			out.close();
			check(static_cast<bool>(out), "serialize_test.spdn: failed to write output file");
		}

		const auto serialized_view = std::u8string_view{ serialized.data(), serialized.size() };
		const auto round_trip = pdn::parse(serialized_view, pdn::utf8_tag);
		check(round_trip.success(), std::string{ "round-trip parse: " } + round_trip.what());
		if (round_trip.success())
		{
			check(round_trip.value().as_object().size() == 15,
				"round-trip parse: expected 15 top-level members");
		}

		const auto null_result = pdn::parse(null_filename, pdn::utf8_tag);
		check(null_result.success(), std::string{ "null_test.spdn: " } + null_result.what());
		if (null_result.success())
		{
			check(null_result.value().as_object().empty(),
				"null_test.spdn: expected an empty object");
		}
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: test_pdn_feature_test <test-data-dir>\n";
		return 2;
	}

	const std::filesystem::path test_root = std::filesystem::weakly_canonical(argv[1]);
	if (!std::filesystem::exists(test_root / "error_test"))
	{
		std::cerr << "test data directory not found: " << test_root << '\n';
		return 2;
	}

	error_test(test_root);
	serialization_test(test_root);

	if (failures != 0)
	{
		std::cout << failures << " check(s) failed\n";
		return 1;
	}

	std::cout << "all feature tests passed\n";
	return 0;
}
