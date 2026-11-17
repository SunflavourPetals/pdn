// Review target: pdn/include/pdn/pdn_ordered_map.h
//
// Coverage:
//   1. map semantics: operator[] insert/overwrite, find, contains, count, at, size, empty, clear,
//      iteration order;
//   2. heterogeneous lookup: keys are std::u8string, queried with std::u8string_view (how object
//      is used inside the library);
//   3. const correctness: return types of at/find on a const object;
//   4. the erase family: by iterator, by const iterator, by range, by key;
//   5. integration: under PDN_ENABLE_ORDERED_MAP, object is ordered_map, and serialization after
//      parsing keeps the insertion order.
//
// Note: erase compiles here only because the default container was changed from std::deque to
//     std::list. deque must move-assign every element after the erased position, but the element
//     is std::pair<const key_t, val_t>, whose operator= is deleted, so on the deque version any
//     instantiation of erase failed to compile.

#define PDN_ENABLE_ORDERED_MAP

#include <pdn/pdn_ordered_map.h>
#include <pdn/pdn_parse.h>
#include <pdn/pdn_serializer.h>
#include <pdn/pdn_type_config.h>

#include <cassert>
#include <string>
#include <string_view>
#include <type_traits>

namespace
{
	using pdn::type::detail::ordered_map;
	using key_t = std::u8string;

	void test_map_semantics()
	{
		ordered_map<key_t, int> m;
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
		static_assert(std::is_same_v<decltype(cm.find(u8"b")), ordered_map<key_t, int>::const_iterator>);
		assert(cm.at(u8"b") == 2);
		assert(cm.find(std::u8string_view{ u8"b" })->second == 2);

		m.clear();
		assert(m.empty());
		assert(m.size() == 0);
	}

	void test_erase()
	{
		ordered_map<key_t, int> m;
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

	void test_ordered_object_roundtrip()
	{
		using namespace std::string_view_literals;

		// With PDN_ENABLE_ORDERED_MAP in effect, object is ordered_map.
		static_assert(std::is_same_v<
			pdn::type::config::object<key_t, int>,
			pdn::type::detail::ordered_map<key_t, int>>);

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
	test_erase();
	test_ordered_object_roundtrip();
	return 0;
}
