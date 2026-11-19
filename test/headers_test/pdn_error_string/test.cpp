// Tests for pdn/include/pdn/pdn_error_string.h.
//
// The header has no runtime logic beyond a byte-for-byte copy, so most of the contract is
// pinned at compile time: the message character type, the two literals, and the fact that the
// reinterpret helper never transcodes. The runtime part checks the bytes that the message
// generators actually rely on (ASCII digits, UTF-8 fragments) and the append chain shown in
// the guide.

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include <pdn/pdn_error_string.h>

namespace
{
	using namespace pdn::literals::error_message_literals;

	// Error text is UTF-8 regardless of the character type used to parse the input.
	static_assert(::std::is_same_v<pdn::error_msg_char, char8_t>);
	static_assert(::std::is_same_v<pdn::error_msg_string, ::std::basic_string<char8_t>>);
	static_assert(::std::is_same_v<pdn::error_msg_string_view, ::std::basic_string_view<char8_t>>);

	// The two literals are reachable through the inline namespaces.
	static_assert(::std::is_same_v<decltype(u8"x"_em), pdn::error_msg_string>);
	static_assert(::std::is_same_v<decltype(u8"x"_emv), pdn::error_msg_string_view>);

	// "constexpr" on _em means the literal is usable in constant expressions. Storing the result
	// in a constexpr variable is different: libstdc++ keeps short strings in an internal buffer
	// addressed by a pointer into the string itself, which is not allowed in a constant
	// expression, so a namespace-scope "constexpr auto s = u8"..."_em;" does not compile today.
	static_assert(u8"hello"_em.size() == 5);
	static_assert(u8"hello"_em[0] == u8'h');
	static_assert(u8"hello"_em[4] == u8'o');

	constexpr auto greeting_v = u8"hello"_emv;
	static_assert(greeting_v.size() == 5);
	static_assert(greeting_v.front() == u8'h');
	static_assert(greeting_v.back() == u8'o');

	constexpr char utf8_chinese[] = "\xE4\xB8\xAD"; // U+4E2D, three UTF-8 bytes
	static_assert(sizeof(utf8_chinese) == 4);
}

int main()
{
	// A UTF-8 fragment copied as-is: same length, same bytes, nothing transcoded.
	const auto chinese = pdn::reinterpret_to_err_msg_str(::std::string_view{ utf8_chinese, 3 });
	assert(chinese.size() == 3);
	assert(chinese[0] == static_cast<char8_t>(0xE4));
	assert(chinese[1] == static_cast<char8_t>(0xB8));
	assert(chinese[2] == static_cast<char8_t>(0xAD));

	// _em at run time: the owned string carries exactly the literal's characters.
	const pdn::error_msg_string hello = u8"hello"_em;
	assert(hello.size() == 5);
	assert(hello == u8"hello");
	assert(hello.data()[0] == u8'h');

	// The pointer-pair overload behaves the same and handles the empty range.
	const pdn::error_msg_string empty = pdn::reinterpret_to_err_msg_str(utf8_chinese + 3, utf8_chinese + 3);
	assert(empty.empty());
	const pdn::error_msg_string same = pdn::reinterpret_to_err_msg_str(utf8_chinese, utf8_chinese + 3);
	assert(same == chinese);

	// ASCII text from std::to_string keeps its bytes: that is what the position prefix needs.
	const pdn::error_msg_string digits = pdn::reinterpret_to_err_msg_str(::std::to_string(20261113));
	assert(digits == u8"20261113");

	// The append chain used by default_error_message_generator (see the guide): the literals and
	// the helper compose into one message without any conversion step in between.
	pdn::error_msg_string message = u8"error"_em;
	message.append(u8"("_em)
	       .append(pdn::reinterpret_to_err_msg_str(::std::to_string(7)))
	       .append(u8":"_em)
	       .append(pdn::reinterpret_to_err_msg_str(::std::to_string(12)))
	       .append(u8") "_em);
	assert(message == u8"error(7:12) ");

	// An owning string converts to the view type without copying.
	const pdn::error_msg_string_view view{ message };
	assert(view.size() == message.size());
	assert(view == pdn::error_msg_string_view{ u8"error(7:12) " });

	return 0;
}
