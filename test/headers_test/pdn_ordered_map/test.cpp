// Review target: pdn/include/pdn/pdn_ordered_map.h
//
// Coverage:
//   1. map semantics: operator[] insert/overwrite, find, contains, count, at, size, empty, clear,
//      iteration order;
//   2. heterogeneous lookup: keys are std::u8string, queried with std::u8string_view (how object
//      is used inside the library);
//   3. const correctness: return types of at/find on a const object;
//   4. the erase family and whole-container assignment, on the default container;
//   5. integration: with PDN_USER_OBJECT=::pdn::ordered_map, object is ordered_map and
//      serialization after parsing keeps the insertion order.
//
// Notes on the element type:
//   - it is std::pair<key_t, val_t>, not std::pair<const key_t, val_t>: the key stays mutable so
//     the elements are assignable, which is what sequence containers need for erase and for
//     operator= of the whole container. Unlike std::map/std::unordered_map there is no ordering or
//     hashing invariant that a mutated key could break (lookup is a linear scan).
//   - appending to the default std::deque invalidates every iterator but keeps references and
//     pointers to the existing elements valid; that is pinned down by test_append_keeps_references().

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#define PDN_USER_OBJECT ::pdn::ordered_map

#include <pdn/pdn_ordered_map.h>
#include <pdn/pdn_parse.h>
#include <pdn/pdn_serializer.h>
#include <pdn/pdn_type_config.h>

namespace
{
	using pdn::ordered_map;
	using key_t = std::u8string;

	using int_map = ordered_map<key_t, int>;

	// A mutable key keeps the element assignable; that is what lets the standard sequence
	// containers implement erase and whole-container assignment.
	static_assert(std::is_same_v<int_map::value_type, std::pair<key_t, int>>);
	static_assert(std::is_assignable_v<int_map::value_type&, int_map::value_type>);

	void test_map_semantics()
	{
		int_map m;
		assert(m.empty());
		assert(m.size() == 0);

		m[u8"b"] = 2;
		m[u8"a"] = 1;
		m[u8"c"] = 3;
		assert(!m.empty());
		assert(m.size() == 3);

		// Insertion order is iteration order.
		auto it = m.begin();
		assert(it->first == u8"b"); ++it;
		assert(it->first == u8"a"); ++it;
		assert(it->first == u8"c"); ++it;
		assert(it == m.end());

		// operator[] overwrites an existing key instead of appending.
		m[u8"a"] = 41;
		assert(m.size() == 3);
		assert(m.at(u8"a") == 41);

		// Heterogeneous lookup.
		assert(m.find(std::u8string_view{ u8"b" })->second == 2);
		assert(m.contains(std::u8string_view{ u8"c" }));
		assert(!m.contains(std::u8string_view{ u8"z" }));
		assert(m.count(std::u8string_view{ u8"c" }) == 1);
		assert(m.count(std::u8string_view{ u8"z" }) == 0);

		// Const correctness.
		const auto& cm = m;
		static_assert(std::is_same_v<decltype(cm.at(u8"b")), const int&>);
		static_assert(std::is_same_v<decltype(cm.find(u8"b")), int_map::const_iterator>);
		assert(cm.at(u8"b") == 2);
		assert(cm.find(std::u8string_view{ u8"b" })->second == 2);

		m.clear();
		assert(m.empty());
		assert(m.size() == 0);
	}

	void test_append_keeps_references()
	{
		int_map m;
		m[u8"a"] = 1;
		int& non_const_ref = m.at(u8"a");
		const int& ref = m.at(u8"a");
		const int* ptr = &ref;

		// Appending to a deque invalidates iterators but not references or pointers.
		m[u8"b"] = 2;
		assert(m.size() == 2);
		assert(&m.at(u8"a") == ptr);
		assert(ref == 1);
		non_const_ref = 11;
		assert(m.at(u8"a") == 11);
	}

	void test_erase()
	{
		int_map m;
		m[u8"a"] = 1;
		m[u8"b"] = 2;
		m[u8"c"] = 3;

		// Erase by iterator returns the successor.
		auto it = m.erase(m.begin());
		assert(m.size() == 2);
		assert(it->first == u8"b");

		// Erase by const iterator.
		auto cit = m.erase(m.cbegin());
		assert(m.size() == 1);
		assert(cit->first == u8"c");

		// Erase by key: 1 when present, 0 when absent.
		assert(m.erase(std::u8string_view{ u8"c" }) == 1);
		assert(m.empty());
		assert(m.erase(std::u8string_view{ u8"c" }) == 0);

		// Erase the range [first, last).
		m[u8"a"] = 1;
		m[u8"b"] = 2;
		m[u8"c"] = 3;
		auto last = m.cbegin();
		++last;
		++last; // points to c, so a and b are erased
		auto r = m.erase(m.cbegin(), last);
		assert(m.size() == 1);
		assert(r->first == u8"c");
		assert(r->second == 3);
	}

	void test_assign_and_sort()
	{
		int_map a;
		a[u8"b"] = 2;
		a[u8"a"] = 1;

		int_map b;
		b[u8"z"] = 0;
		b = a; // whole-container assignment assigns elements, which a const key would forbid
		assert(b.size() == 2);
		assert(b.at(u8"b") == 2);
		assert(b.begin()->first == u8"b"); // assignment copies the element order too

		// A mutable key also allows reordering the elements like any other sequence.
		std::sort(b.begin(), b.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
		assert(b.begin()->first == u8"a");
		assert(b.at(u8"b") == 2); // linear lookup does not depend on the position
	}

	void test_ordered_object_roundtrip()
	{
		using namespace std::string_view_literals;

		// With PDN_USER_OBJECT=::pdn::ordered_map in effect, object is ordered_map.
		static_assert(std::is_same_v<
			pdn::type::config::object<key_t, int>,
			ordered_map<key_t, int>>);
		static_assert(std::is_same_v<
			pdn::type::object<char8_t>,
			ordered_map<pdn::type::u8string, pdn::entity<char8_t>>>);

		auto src = u8R"(zeta 1 alpha 2 mid 3)"sv;
		auto r = pdn::parse(src, pdn::utf8_tag);
		assert(r.has_value());

		auto text = pdn::make_u8serializer().serialize(r->as_object());
		std::u8string_view out{ text.data(), text.size() };

		auto i_zeta  = out.find(u8"zeta");
		auto i_alpha = out.find(u8"alpha");
		auto i_mid   = out.find(u8"mid");
		assert(i_zeta != std::u8string_view::npos);
		assert(i_alpha != std::u8string_view::npos);
		assert(i_mid != std::u8string_view::npos);
		assert(i_zeta < i_alpha);
		assert(i_alpha < i_mid);
	}
}

int main()
{
	test_map_semantics();
	test_append_keeps_references();
	test_erase();
	test_assign_and_sort();
	test_ordered_object_roundtrip();
	return 0;
}
