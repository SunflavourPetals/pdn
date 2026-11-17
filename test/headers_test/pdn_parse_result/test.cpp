#include <pdn/pdn_parse_result.h>

#include <cstddef>
#include <cstdio>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
	int fails = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			++fails;
			std::printf("FAILED: %s\n", what);
		}
	}

	using char_t   = pdn::unicode::u8char_t;
	using result_t = pdn::parse_result<char_t>;
	using entity_t = pdn::entity<char_t>;
	using u8sv     = ::std::u8string_view;
	using i32      = pdn::type::i32;

	// The result type mirrors optional's interface: the entity type is fixed by char_t, the class is
	// copyable and movable like the entity it holds, and it is not default constructible.
	static_assert(::std::is_same_v<result_t::entity_type, entity_t>);
	static_assert(::std::is_copy_constructible_v<result_t>);
	static_assert(::std::is_copy_assignable_v<result_t>);
	static_assert(::std::is_move_constructible_v<result_t>);
	static_assert(::std::is_move_assignable_v<result_t>);
	static_assert(!::std::is_default_constructible_v<result_t>);
	static_assert(::std::is_constructible_v<result_t, pdn::parse_error_code>);
	static_assert(::std::is_constructible_v<result_t, entity_t&&>);
	static_assert(::std::is_constructible_v<result_t, pdn::parse_error_code, entity_t&&>);
	static_assert(!::std::is_convertible_v<pdn::parse_error_code, result_t>); // explicit
	static_assert(!::std::is_convertible_v<entity_t&&, result_t>);            // explicit
	static_assert(::std::is_same_v<decltype(::std::declval<result_t&>().operator*()), entity_t&>);
	static_assert(::std::is_same_v<decltype(::std::declval<const result_t&>().operator*()), const entity_t&>);
	static_assert(::std::is_same_v<decltype(::std::declval<result_t&&>().operator*()), entity_t&&>);
	static_assert(::std::is_same_v<decltype(::std::declval<const result_t&&>().operator*()), const entity_t&&>);
	static_assert(::std::is_same_v<decltype(::std::declval<result_t&>().operator->()), entity_t*>);
	static_assert(::std::is_same_v<decltype(::std::declval<const result_t&>().operator->()), const entity_t*>);

	struct code_name
	{
		pdn::parse_error_code code;
		const char*            name;
	};

	const code_name code_names[] = {
		{ pdn::parse_error_code::parse_error,             "parse_error"             },
		{ pdn::parse_error_code::parse_error_limit,       "parse_error_limit"       },
		{ pdn::parse_error_code::nesting_depth_exceed,    "nesting_depth_exceed"    },
		{ pdn::parse_error_code::open_file_failed,        "open_file_failed"        },
		{ pdn::parse_error_code::bad_stream,              "bad_stream"              },
		{ pdn::parse_error_code::stream_operation_failed, "stream_operation_failed" },
		{ pdn::parse_error_code::incomplete_code_unit,    "incomplete_code_unit"    },
		{ pdn::parse_error_code::user_cancelled,          "user_cancelled"          },
	};


	void test_success()
	{
		result_t r{ entity_t{ i32{ 7 } } };
		check(r.success(), "success");
		check(r.has_value(), "has_value");
		check(static_cast<bool>(r), "operator bool");
		check(!r.failed(), "not failed");
		check(r.error_code() == pdn::parse_error_code{}, "the zero code means success");
		check(::std::string_view{ r.what() } == "success", "what() names the zero code");
		check(r.value().as_int() == 7, "value");
		check((*r).as_int() == 7, "operator*");
		check(r->as_int() == 7, "operator->");
		check(&r.value() == &*r, "value, operator* and operator-> reach the same entity");
	}

	void test_failure()
	{
		for (const auto& [code, name] : code_names)
		{
			result_t r{ code };
			check(r.failed(), "failed");
			check(!r.success(), "not success");
			check(!r.has_value(), "no value");
			check(!static_cast<bool>(r), "operator bool is false");
			check(r.error_code() == code, "error_code");
			check(::std::string_view{ r.what() } == name, "what() names the code");
			// value(), operator* and operator-> assert success() here: the release build fallback
			// (the default i32 entity) is checked by the NDEBUG target instead.
		}
	}

	void test_two_argument_constructor()
	{
		// pdn_parse.h returns this shape when the token stream ended cleanly but the function package
		// reported errors: a failure that still carries what was parsed. value() asserts on a failed
		// result, so only the failure bookkeeping is observable here.
		result_t r{ pdn::parse_error_code::parse_error, entity_t{ i32{ 42 } } };
		check(r.failed(), "two-argument construction is a failure");
		check(!r.has_value(), "two-argument construction carries no value");
		check(r.error_code() == pdn::parse_error_code::parse_error, "error_code is kept");
	}
	void test_what_for_unknown_value()
	{
		const result_t r{ static_cast<pdn::parse_error_code>(99) };
		check(r.what() != nullptr, "what() is never null");
		check(::std::string_view{ r.what() } == "unknown_parse_error_code", "what() for a value outside the enumerators");
		check(r.failed(), "an unknown value is still a failure");
	}

	void test_const_access()
	{
		const result_t r{ entity_t{ i32{ 5 } } };
		const result_t& cref = r;
		check(cref.value().as_int() == 5, "const value");
		check((*cref).as_int() == 5, "const operator*");
		check(cref->as_int() == 5, "const operator->");
	}

	void test_rvalue_access()
	{
		result_t r{ entity_t{ i32{ 11 } } };
		entity_t moved = *::std::move(r);
		check(moved.as_int() == 11, "operator* on an rvalue moves the entity out");
	}

	void test_copy_is_independent()
	{
		using object_t = pdn::type::object<char_t>;
		object_t obj;
		obj.emplace(u8sv{ u8"k" }, entity_t{ pdn::make_proxy<pdn::type::string<char_t>>(u8sv{ u8"v" }) });
		result_t r{ entity_t{ pdn::make_proxy<object_t>(::std::move(obj)) } };
		check(r.value()[u8sv{ u8"k" }].as_string() == u8"v", "the entity was moved into the result");

		result_t copy = r;
		copy.value()[u8sv{ u8"k" }] = pdn::make_proxy<pdn::type::string<char_t>>(u8sv{ u8"changed" });
		check(r.value()[u8sv{ u8"k" }].as_string() == u8"v", "copying the result deep-copies the entity");
		check(copy.value()[u8sv{ u8"k" }].as_string() == u8"changed", "the copy is independent");

		result_t moved = ::std::move(copy);
		check(moved.value()[u8sv{ u8"k" }].as_string() == u8"changed", "moving keeps the entity");
	}

	void test_assignment()
	{
		result_t r{ entity_t{ i32{ 1 } } };
		r = result_t{ pdn::parse_error_code::user_cancelled };
		check(r.failed() && r.error_code() == pdn::parse_error_code::user_cancelled, "assigning a failure");
		r = result_t{ entity_t{ i32{ 2 } } };
		check(r.success() && r.value().as_int() == 2, "assigning a success");

		const result_t other{ entity_t{ i32{ 3 } } };
		r = other;
		check(r.value().as_int() == 3, "copy assignment");
	}
}

int main()
{
	test_success();
	test_failure();
	test_two_argument_constructor();
	test_what_for_unknown_value();
	test_const_access();
	test_rvalue_access();
	test_copy_is_independent();
	test_assignment();
	if (fails == 0)
	{
		std::printf("all pdn_parse_result checks passed\n");
	}
	return fails == 0 ? 0 : 1;
}
