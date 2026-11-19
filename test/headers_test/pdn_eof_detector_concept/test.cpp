// Review test for pdn_eof_detector_concept.h.
// The concept is purely structural: it asks for a const-callable eof() that returns exactly
// bool. These checks pin down what that admits and what it rejects, and that the library's
// eof() versus begin/end dispatch keeps working for both kinds of source.

#include <istream>
#include <sstream>
#include <string>

#include <pdn/pdn_eof_detector_concept.h>

namespace
{
	using pdn::concepts::eof_detector;

	struct const_bool
	{
		bool eof() const noexcept { return true; }
	};

	struct non_const
	{
		bool eof() { return true; }
	};

	struct returns_int
	{
		int eof() const { return 1; }
	};

	struct returns_pointer
	{
		void* eof() const { return nullptr; }
	};

	struct explicit_flag
	{
		explicit operator bool() const { return true; }
	};

	struct returns_explicit_flag
	{
		explicit_flag eof() const { return {}; }
	};

	struct returns_bool_ref
	{
		bool value = true;
		const bool& eof() const { return value; }
	};

	struct static_eof
	{
		static bool eof() { return true; }
	};

	struct no_eof
	{
		bool end() const { return true; }
	};

	// Accepted: a const-callable eof() returning exactly bool.
	static_assert(eof_detector<const_bool>);
	static_assert(eof_detector<static_eof>);
	// Purely structural, so standard stream types satisfy it as well.
	static_assert(eof_detector<::std::istringstream>);

	// Rejected: not const-callable, or anything whose result is merely convertible to bool.
	static_assert(!eof_detector<non_const>);
	static_assert(!eof_detector<returns_int>);
	static_assert(!eof_detector<returns_pointer>);
	static_assert(!eof_detector<returns_explicit_flag>);
	static_assert(!eof_detector<returns_bool_ref>);
	static_assert(!eof_detector<no_eof>);
	static_assert(!eof_detector<int>);
	static_assert(!eof_detector<void>);

	// References are stripped before the const is applied, so a reference argument means the same
	// as the value type: T&, const T& and T&& cannot smuggle a non-const eof() in.
	static_assert(eof_detector<const_bool&>);
	static_assert(eof_detector<const const_bool&>);
	static_assert(eof_detector<const_bool&&>);
	static_assert(eof_detector<::std::istringstream&>);
	static_assert(!eof_detector<non_const&>);
	static_assert(!eof_detector<non_const&&>);

	// The library picks between "eof() driven source" and "begin/end pair" with this concept,
	// so both branches have to stay reachable for their respective shapes.
	template <typename source_t>
	constexpr bool is_eof_driven()
	{
		return eof_detector<source_t>;
	}

	static_assert(is_eof_driven<const_bool>());
	static_assert(!is_eof_driven<::std::string::const_iterator>());

	// A minimal eof() driven source: this is the shape the iterator adapters expect.
	struct counting_source
	{
		int remaining = 3;

		bool eof() const noexcept { return remaining == 0; }
		char operator*() const noexcept { return 'x'; }
		counting_source& operator++() noexcept
		{
			--remaining;
			return *this;
		}
	};

	static_assert(eof_detector<counting_source>);

	template <eof_detector source_t>
	int drain(source_t& source)
	{
		int elements = 0;
		while (!source.eof())
		{
			(void)*source;
			++source;
			++elements;
		}
		return elements;
	}
}

int main()
{
	counting_source source;
	if (drain(source) != 3) return 1;
	if (!source.eof()) return 2;

	::std::istringstream stream{ "abc" };
	if (stream.eof()) return 3; // nothing has been read yet

	return 0;
}
