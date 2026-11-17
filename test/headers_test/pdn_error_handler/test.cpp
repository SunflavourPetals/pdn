#include <pdn/pdn_error_handler.h>
#include <pdn/pdn_parse.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace
{
	using namespace pdn;

	int fails = 0;

#define CHECK(condition)                                                             \
	do                                                                               \
	{                                                                                \
		if (!(condition))                                                            \
		{                                                                            \
			++fails;                                                                 \
			::std::printf("FAILED: %s (line %d)\n", #condition, __LINE__);           \
		}                                                                            \
	} while (false)

	auto msg_of(::std::u8string_view text, source_position pos = { 3, 7 }) -> error_message
	{
		return error_message{ lexical_error_code::unacceptable_character, pos, unicode::u8string{ text } };
	}

	auto to_std(const unicode::u8string& s) -> ::std::string
	{
		return ::std::string{ reinterpret_cast<const char*>(s.data()), s.size() };
	}

	template <typename fn_t>
	auto capture_cerr(fn_t&& what) -> ::std::string
	{
		::std::ostringstream cap{};
		auto* const old = ::std::cerr.rdbuf(cap.rdbuf());
		what();
		::std::cerr.rdbuf(old);
		return cap.str();
	}

	void test_default_error_handler()
	{
		default_error_handler h{};
		CHECK(h.error_count() == 0u);

		::std::ostringstream out{};
		h.handle_error(msg_of(u8"m1"), out);
		h.handle_error(msg_of(u8"m2"), out);
		CHECK(h.error_count() == 2u);
		CHECK(out.str() == "m1\nm2\n");

		const auto printed_before = out.str().size();
		h.handle_error(); // counts only, prints nothing
		CHECK(h.error_count() == 3u);
		CHECK(out.str().size() == printed_before);

		h.clear();
		CHECK(h.error_count() == 0u);

		const auto to_cerr = capture_cerr([&] { h.handle_error(msg_of(u8"to-cerr")); });
		CHECK(to_cerr == "to-cerr\n");
		CHECK(h.error_count() == 1u);
	}

	void test_threshold_error_handler()
	{
		default_threshold_error_handler h{};
		CHECK(h.limit == default_threshold_error_handler::default_limit);
		CHECK(h.limit == 100u);
		CHECK(!h.stop_requested());
		CHECK(h.stop_reason() == stop_reason_code{});

		::std::ostringstream out{};
		h.handle_error(msg_of(u8"a"), out);
		CHECK(h.error_count() == 1u);
		CHECK(!h.stop_requested());

		default_threshold_error_handler two{ 2u };
		::std::ostringstream out2{};
		two.handle_error(msg_of(u8"a"), out2);
		CHECK(!two.stop_requested());
		two.handle_error(msg_of(u8"b"), out2);
		CHECK(two.error_count() == 2u);
		CHECK(two.stop_requested());
		CHECK(two.stop_reason() == stop_reason_code::parse_error_limit);
		CHECK(out2.str() == "a\nb\n");

		two.clear();
		CHECK(two.error_count() == 0u);
		CHECK(!two.stop_requested());
		CHECK(two.stop_reason() == stop_reason_code{});

		default_threshold_error_handler zero{ 0u };
		::std::ostringstream out3{};
		zero.handle_error(msg_of(u8"x"), out3);
		CHECK(zero.error_count() == 1u);
		CHECK(zero.stop_requested());
		CHECK(zero.stop_reason() == stop_reason_code::parse_error_limit);

		default_threshold_error_handler no_message{ 1u };
		no_message.handle_error();
		CHECK(no_message.error_count() == 1u);
		CHECK(no_message.stop_requested());

		// the limit may be changed while a handler is in use
		two.limit = 5u;
		::std::ostringstream out5{};
		for (int i = 0; i != 4; ++i)
		{
			two.handle_error(msg_of(u8"z"), out5);
		}
		CHECK(!two.stop_requested());
		two.handle_error(msg_of(u8"z"), out5);
		CHECK(two.stop_requested());
	}

	void test_error_msg_for_filename()
	{
		const auto sample = msg_of(u8"boom");
		const auto named = detail::error_msg_for_filename(unicode::u8string{ u8"a.spdn" }, sample);
		CHECK(to_std(named.error_message) == "a.spdn: boom");
		CHECK(named.error_code == sample.error_code);
		CHECK(named.position == sample.position);

		const auto empty_name = detail::error_msg_for_filename(unicode::u8string{}, msg_of(u8"boom"));
		CHECK(to_std(empty_name.error_message) == ": boom");

		const auto empty_text = detail::error_msg_for_filename(unicode::u8string{ u8"f" }, msg_of(u8""));
		CHECK(to_std(empty_text.error_message) == "f: ");
	}

	void test_end_to_end()
	{
		// string input: no file name, so the package does not add a "<file>: " prefix
		const auto string_printed = capture_cerr([] {
			const auto r = pdn::parse(::std::u8string_view{ u8"name \"abc" }, pdn::utf8_tag);
			CHECK(!r);
		});
		CHECK(string_printed.find("lexical error(1:6)") == 0u);
		CHECK(string_printed.rfind(": ", 0) != 0u);

		// file input: the package prefixes the file name
		constexpr const char* file_name = "pdn_error_handler_unterminated.spdn";
		{
			::std::ofstream f{ file_name, ::std::ios::binary };
			f << "name \"abc";
		}
		const auto file_printed = capture_cerr([] {
			const auto r = pdn::parse("pdn_error_handler_unterminated.spdn", pdn::utf8_tag);
			CHECK(!r);
		});
		CHECK(file_printed.rfind("pdn_error_handler_unterminated.spdn: ", 0) == 0u);
		::std::remove(file_name);
	}

	void test_package_integration()
	{
		default_function_package<char8_t> fp{};
		CHECK(fp.error_threshold() == 100u);
		fp.reset_error_threshold(1u);
		CHECK(fp.error_threshold() == 1u);
		CHECK(!fp.stop_requested());

		::std::ostringstream out{};
		fp.handle_error(msg_of(u8"e1"), out);
		CHECK(fp.error_count() == 1u);
		CHECK(fp.stop_requested());
		CHECK(fp.stop_reason() == stop_reason_code::parse_error_limit);

		fp.clear_error();
		CHECK(fp.error_count() == 0u);
		CHECK(!fp.stop_requested());
	}
}

int main()
{
	static_assert(concepts::error_handler<default_error_handler>);
	static_assert(concepts::error_handler<default_threshold_error_handler>);
	static_assert(concepts::stoppable<default_threshold_error_handler>);
	static_assert(::std::same_as<default_threshold_error_handler::error_count_t, ::std::size_t>);
	static_assert(default_threshold_error_handler::default_limit == 100u);

	test_default_error_handler();
	test_threshold_error_handler();
	test_error_msg_for_filename();
	test_end_to_end();
	test_package_integration();

	if (fails == 0)
	{
		::std::printf("pdn_error_handler ok\n");
		return 0;
	}
	::std::printf("pdn_error_handler: %d failure(s)\n", fails);
	return 1;
}
