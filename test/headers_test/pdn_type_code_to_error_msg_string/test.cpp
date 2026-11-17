#include <pdn/pdn_type_code_to_error_msg_string.h>
#include <pdn/pdn_type_generator.h>

#include <array>
#include <cassert>
#include <cstddef>

namespace
{
	struct type_name_case
	{
		pdn::type_code             code;
		pdn::error_msg_string_view name;
	};

	constexpr ::std::array<type_name_case, 16> type_names{ {
		{ pdn::type_code::unknown,   u8"unknown"   },
		{ pdn::type_code::i8,        u8"i8"        },
		{ pdn::type_code::i16,       u8"i16"       },
		{ pdn::type_code::i32,       u8"i32"       },
		{ pdn::type_code::i64,       u8"i64"       },
		{ pdn::type_code::u8,        u8"u8"        },
		{ pdn::type_code::u16,       u8"u16"       },
		{ pdn::type_code::u32,       u8"u32"       },
		{ pdn::type_code::u64,       u8"u64"       },
		{ pdn::type_code::f32,       u8"f32"       },
		{ pdn::type_code::f64,       u8"f64"       },
		{ pdn::type_code::boolean,   u8"boolean"   },
		{ pdn::type_code::character, u8"character" },
		{ pdn::type_code::string,    u8"string"    },
		{ pdn::type_code::list,      u8"list"      },
		{ pdn::type_code::object,    u8"object"    },
	} };
}

// The view overload is a constant expression and cannot throw.
static_assert(pdn::type_code_to_error_msg_string_view(pdn::type_code::i8) == u8"i8");
static_assert(pdn::type_code_to_error_msg_string_view(pdn::type_code::character) == u8"character");
static_assert(noexcept(pdn::type_code_to_error_msg_string_view(pdn::type_code::object)));
// An out of range code degrades to the sentinel spelling instead of trapping.
static_assert(pdn::type_code_to_error_msg_string_view(static_cast<pdn::type_code>(99)) == u8"unknown");

int main()
{
	// 1. Every code maps to exactly its canonical spelling; the string overload copies the view verbatim.
	for (const auto& c : type_names)
	{
		const auto view = pdn::type_code_to_error_msg_string_view(c.code);
		assert(view == c.name);
		assert(view.size() == c.name.size());
		assert(pdn::type_code_to_error_msg_string(c.code) == c.name);
	}

	// 2. The spellings are pure ASCII: they are spliced into diagnostics and serialized text.
	for (const auto& c : type_names)
	{
		for (const auto ch : c.name)
		{
			assert(static_cast<unsigned>(ch) < 0x80);
		}
	}

	// 3. Every PDN type spelling parses back to the same type_code. This is not a contract of this
	//    header: it is the coupling the serializer depends on when it splices these names into output.
	for (const auto& c : type_names)
	{
		if (c.code == pdn::type_code::unknown)
		{
			continue;
		}
		const auto parsed =
			pdn::default_type_generator<char8_t>::generate_type(pdn::error_msg_string{ c.name });
		assert(parsed == c.code);
	}

	// 4. The sentinel spelling is not a PDN type name either: it parses back to unknown.
	assert(pdn::default_type_generator<char8_t>::generate_type(pdn::error_msg_string{ u8"unknown" }) ==
		   pdn::type_code::unknown);
	assert(pdn::type_code_to_error_msg_string(static_cast<pdn::type_code>(-1)) == u8"unknown");
}