// Tests for pdn/include/pdn/pdn_error_code_variant.h and
// pdn/include/pdn/pdn_error_code_variant_to_error_msg_string.h.
//
// What is pinned down here:
//   * error_code_variant has exactly five alternatives, the three decode error codes plus
//     lexical_error_code and syntax_error_code, and no encoding error code: encoders only see
//     code points that came out of a decoder, i.e. valid scalars, so encoding cannot fail;
//   * every alternative has a distinct category label, and the label is plain ASCII with no
//     surrounding space, because the default message generator writes "label(line:column) "
//     by appending to it;
//   * the string overload returns the same text as the view overload.

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <pdn/pdn_error_code_variant.h>
#include <pdn/pdn_error_code_variant_to_error_msg_string.h>

// Only to name the encode error codes that the variant must not contain.
#include <pdn/pdn_utf8_encoder.h>
#include <pdn/pdn_utf16_encoder.h>
#include <pdn/pdn_utf32_encoder.h>

namespace
{
	using pdn::error_code_variant;
	using pdn::error_code_variant_to_error_msg_string;
	using pdn::error_code_variant_to_error_msg_string_view;
	using namespace std::string_view_literals;

	int failures = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	// True when t is one of the alternatives of error_code_variant, whatever its index is.
	template <typename t, std::size_t... i>
	constexpr bool has_alternative(std::index_sequence<i...>) noexcept
	{
		return ((std::same_as<std::variant_alternative_t<i, error_code_variant>, t>) || ...);
	}

	inline constexpr auto alternative_indices =
		std::make_index_sequence<std::variant_size_v<error_code_variant>>{};

	constexpr bool has_ascii_only(std::u8string_view s) noexcept
	{
		for (const char8_t c : s)
		{
			if (c < 0x20 || c > 0x7E)
			{
				return false;
			}
		}
		return true;
	}

	std::string as_string(std::u8string_view s)
	{
		return std::string(reinterpret_cast<const char*>(s.data()), s.size());
	}

	void test_variant_shape()
	{
		static_assert(std::variant_size_v<error_code_variant> == 5, "unexpected number of alternatives");
		static_assert(has_alternative<pdn::unicode::utf8::decode_error_code>(alternative_indices));
		static_assert(has_alternative<pdn::unicode::utf16::decode_error_code>(alternative_indices));
		static_assert(has_alternative<pdn::unicode::utf32::decode_error_code>(alternative_indices));
		static_assert(has_alternative<pdn::lexical_error_code>(alternative_indices));
		static_assert(has_alternative<pdn::syntax_error_code>(alternative_indices));

		// Encoding a valid scalar cannot fail, so encode codes must stay out of the variant.
		static_assert(!has_alternative<pdn::unicode::utf8::encode_error_code>(alternative_indices));
		static_assert(!has_alternative<pdn::unicode::utf16::encode_error_code>(alternative_indices));
		static_assert(!has_alternative<pdn::unicode::utf32::encode_error_code>(alternative_indices));

		// All alternatives are enums, so copying and moving a variant cannot throw and it can
		// never become valueless; the message generator relies on visit() always succeeding.
		static_assert(std::is_nothrow_copy_constructible_v<error_code_variant>);
		static_assert(std::is_nothrow_move_constructible_v<error_code_variant>);
		static_assert(std::is_copy_assignable_v<error_code_variant>);

		const error_code_variant code{ pdn::lexical_error_code::unacceptable_character };
		check(!code.valueless_by_exception(), "variant must not be valueless");
		check(std::holds_alternative<pdn::lexical_error_code>(code), "wrong alternative held");
	}

	struct label_case
	{
		error_code_variant code;
		std::u8string_view  label;
	};

	void test_labels()
	{
		const std::array cases{
			label_case{ pdn::unicode::utf8::decode_error_code::invalid_code_point,  u8"utf-8 decode error"sv },
			label_case{ pdn::unicode::utf16::decode_error_code::invalid_code_point, u8"utf-16 decode error"sv },
			label_case{ pdn::unicode::utf32::decode_error_code::invalid_code_point, u8"utf-32 decode error"sv },
			label_case{ pdn::lexical_error_code::unacceptable_character,            u8"lexical error"sv },
			label_case{ pdn::syntax_error_code::expect_colon,                       u8"syntax error"sv },
		};

		static_assert(cases.size() == std::variant_size_v<error_code_variant>);

		for (const auto& c : cases)
		{
			const std::u8string_view view = error_code_variant_to_error_msg_string_view(c.code);
			check(view == c.label, "category label mismatch");

			const std::string text = as_string(view);
			check(text == as_string(c.label), "view label must be ASCII text");
			check(!view.empty(), "category label must not be empty");
			check(view.front() != u8' ' && view.back() != u8' ', "category label must not be padded");
			check(view.front() >= u8'a' && view.front() <= u8'z', "category label must start with a lowercase letter");
			check(has_ascii_only(view), "category label must be ASCII only");

			const std::u8string copied = error_code_variant_to_error_msg_string(c.code);
			check(std::u8string_view{ copied } == c.label, "string overload must agree with the view overload");
		}

		// The labels are prefixes of the same sentence, so they have to be distinguishable.
		for (std::size_t i = 0; i < cases.size(); ++i)
		{
			for (std::size_t j = i + 1; j < cases.size(); ++j)
			{
				check(error_code_variant_to_error_msg_string_view(cases[i].code) !=
					  error_code_variant_to_error_msg_string_view(cases[j].code),
					  "category labels must be pairwise distinct");
			}
		}
	}
}

int main()
{
	test_variant_shape();
	test_labels();

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}

	std::printf("all checks passed\n");
	return 0;
}
