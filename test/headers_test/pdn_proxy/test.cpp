// Tests for pdn/include/pdn/pdn_proxy.h.
//
// proxy<T> is the owned-pointer wrapper entity uses so that string/list/object alternatives cost
// one pointer instead of a whole container. The contract worth pinning: it is never empty after
// construction, copying is a deep copy, moving empties the source (the guide documents that), the
// const accessors hand out const pointers, and no public constructor accepts a raw pointer.

#include <pdn/pdn_proxy.h>

#include <concepts>
#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	struct no_default
	{
		explicit no_default(int v) : value{ v } {}
		int value;
	};

	template <typename t>
	concept equality_comparable = requires(const t& a, const t& b) { { a == b } -> ::std::same_as<bool>; };

	static_assert(::std::same_as<pdn::proxy<int>::element_type, int>);
	static_assert(::std::same_as<pdn::proxy<int>::pointer, int*>);
	static_assert(::std::same_as<pdn::proxy<int>::const_pointer, const int*>);
	static_assert(::std::same_as<pdn::proxy<int>::deleter_type, ::std::default_delete<int>>);

	// One unique_ptr's worth of storage; that is the reason the class exists.
	static_assert(sizeof(pdn::proxy<int>) == sizeof(::std::unique_ptr<int>));
	static_assert(sizeof(pdn::proxy<::std::string>) == sizeof(::std::unique_ptr<::std::string>));

	// No way to build an empty or externally owned proxy.
	static_assert(!::std::is_constructible_v<pdn::proxy<int>, int*>);
	static_assert(!::std::is_constructible_v<pdn::proxy<int>, ::std::unique_ptr<int>>);
	static_assert(!::std::is_convertible_v<pdn::proxy<int>, bool>); // operator bool is explicit
	static_assert(::std::is_constructible_v<bool, pdn::proxy<int>>);
	static_assert(::std::is_nothrow_move_constructible_v<pdn::proxy<int>>);
	static_assert(::std::is_nothrow_move_assignable_v<pdn::proxy<int>>);
	static_assert(::std::is_copy_constructible_v<pdn::proxy<int>>);

	// No comparison operators: entity compares the pointees itself.
	static_assert(!equality_comparable<pdn::proxy<int>>);

	// remove_proxy strips cv-qualifiers and references first, then the proxy wrapper itself.
	static_assert(::std::same_as<pdn::remove_proxy_t<pdn::proxy<int>>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<const pdn::proxy<int>>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<volatile pdn::proxy<int>>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<const volatile pdn::proxy<int>>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<int>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<::std::string>, ::std::string>);
	static_assert(::std::same_as<pdn::remove_proxy_t<pdn::proxy<int>&>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<const pdn::proxy<int>&>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<pdn::proxy<int>&&>, int>);
	static_assert(::std::same_as<pdn::remove_proxy_t<const int&>, int>);
}

int main()
{
	int failures = 0;
	const auto check = [&failures](bool ok, const char* what)
	{
		if (!ok)
		{
			::std::printf("FAILED: %s\n", what);
			++failures;
		}
	};

	{
		pdn::proxy<int> p;
		check(static_cast<bool>(p), "a default proxy is not empty");
		check(p.get() != nullptr, "get() is not null");
		check(*p == 0, "the pointee is value-initialised");
	}
	{
		pdn::proxy<int> a;
		*a = 7;
		pdn::proxy<int> b{ a };
		check(b.get() != a.get(), "a copy owns a distinct object");
		check(*b == 7, "a copy carries the value over");
		*b = 9;
		check(*a == 7, "writing the copy does not touch the source");
	}
	{
		pdn::proxy<int> a;
		*a = 5;
		pdn::proxy<int> b{ ::std::move(a) };
		check(*b == 5, "a move transfers the value");
		check(!static_cast<bool>(a), "a moved-from proxy is empty");
		a = pdn::proxy<int>{};
		check(static_cast<bool>(a) && *a == 0, "assignment restores a moved-from proxy");
	}
	{
		pdn::proxy<int> a;
		*a = 1;
		pdn::proxy<int> b;
		*b = 2;
		b = a;
		check(*b == 1 && b.get() != a.get(), "copy assignment is a deep copy");
		b = ::std::move(a);
		check(*b == 1, "move assignment transfers the value");
	}
	{
		pdn::proxy<int> a;
		*a = 3;
		pdn::proxy<int>& alias = a;
		a = ::std::move(alias); // self-move must not destroy the object
		check(static_cast<bool>(a) && *a == 3, "self-move-assignment keeps the object");
		a = alias; // self-copy
		check(static_cast<bool>(a) && *a == 3, "self-copy-assignment keeps the value");
	}
	{
		pdn::proxy<int> a;
		*a = 11;
		const pdn::proxy<int>& cp = a;
		static_assert(::std::same_as<decltype(cp.get()), const int*>);
		static_assert(::std::same_as<decltype(*cp), const int&>);
		static_assert(::std::same_as<decltype(cp.operator->()), const int*>);
		check(*cp == 11 && cp.get() != nullptr, "const accessors read through");
	}
	{
		auto s = pdn::make_proxy<::std::string>(3, 'x');
		check(*s == "xxx", "make_proxy forwards constructor arguments");
		auto e = pdn::make_proxy<::std::string>();
		check(e->empty(), "make_proxy default-constructs the pointee");
		auto nd = pdn::make_proxy<no_default>(42);
		check(nd->value == 42, "make_proxy works without a default constructor");
	}
	{
		pdn::proxy<int> a;
		*a = 1;
		pdn::proxy<int> b;
		*b = 2;
		using ::std::swap;
		swap(a, b);
		check(*a == 2 && *b == 1, "std::swap works through the move operations");
	}

	return failures;
}
