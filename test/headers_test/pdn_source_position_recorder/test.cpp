#include <cstdio>
#include <type_traits>

#include <pdn/pdn_source_position_recorder.h>

namespace
{
	using pdn::source_position_recorder;

	// the recorder models the concept it is written for
	static_assert(pdn::concepts::source_position_recorder<source_position_recorder>);
	static_assert(::std::is_copy_constructible_v<source_position_recorder>);
	static_assert(::std::is_copy_assignable_v<source_position_recorder>);

	// line and column start at 1:1
	static_assert([] { return source_position_recorder{}.position(); }().line == 1);
	static_assert([] { return source_position_recorder{}.position(); }().column == 1);

	// ordinary code points only advance the column
	static_assert([] {
		source_position_recorder r;
		r.update(U'a');
		r.update(U'b');
		r.update(U'\u4e2d');
		return r.position();
	}().column == 4);

	// CR, NEL, LS and PS each start a new line
	static_assert([] {
		source_position_recorder r;
		r.update(U'a');
		r.update(U'\r');
		return r.position();
	}().line == 2);
	static_assert([] {
		source_position_recorder r;
		r.update(U'\u0085');
		return r.position();
	}().line == 2);
	static_assert([] {
		source_position_recorder r;
		r.update(U'\u2028');
		return r.position();
	}().line == 2);
	static_assert([] {
		source_position_recorder r;
		r.update(U'\u2029');
		return r.position();
	}().line == 2);

	// CRLF is one line break: the LF after CR does not advance the line again
	static_assert([] {
		source_position_recorder r;
		r.update(U'\r');
		r.update(U'\n');
		return r.position();
	}().line == 2);
	static_assert([] {
		source_position_recorder r;
		r.update(U'\r');
		r.update(U'\n');
		r.update(U'x');
		return r.position();
	}().column == 2);
	// ... but LF alone does advance it
	static_assert([] {
		source_position_recorder r;
		r.update(U'\n');
		return r.position();
	}().line == 2);
	// ... and CR followed by something else is a line break of its own
	static_assert([] {
		source_position_recorder r;
		r.update(U'\r');
		r.update(U'\n');
		r.update(U'\r');
		r.update(U'x');
		return r.position();
	}().line == 3);

	// the column goes back to 1 after every kind of line break
	static_assert([] {
		source_position_recorder r;
		r.update(U'a');
		r.update(U'b');
		r.update(U'\n');
		return r.position();
	}().column == 1);

	// clear() restores the state of a freshly constructed recorder
	static_assert([] {
		source_position_recorder r;
		r.update(U'a');
		r.update(U'\n');
		r.update(U'b');
		r.clear();
		return r.position();
	}().line == 1);
	static_assert([] {
		source_position_recorder r;
		r.update(U'a');
		r.update(U'\n');
		r.clear();
		return r.position();
	}().column == 1);
	// in particular it forgets that the previous code point was CR,
	// so the first LF after clear() is a line break and not a CRLF tail
	static_assert([] {
		source_position_recorder r;
		r.update(U'\r');
		r.clear();
		r.update(U'\n');
		return r.position();
	}().line == 2);
	static_assert([] {
		source_position_recorder clear_then_lf = [] {
			source_position_recorder r;
			r.update(U'\r');
			r.clear();
			r.update(U'\n');
			return r;
		}();
		source_position_recorder fresh_then_lf;
		fresh_then_lf.update(U'\n');
		return clear_then_lf.position() == fresh_then_lf.position();
	}());
}

int main()
{
	int failures = 0;
	auto check = [&failures](bool ok, const char* what) {
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	};

	// a full second line, measured at run time
	source_position_recorder r;
	r.update(U'a');
	r.update(U'b');
	r.update(U'\r');
	r.update(U'\n');
	r.update(U'c');
	const auto p = r.position();
	check(p.line == 2 && p.column == 2, "CRLF then 'c' is 2:2");

	// repeated CRs are repeated line breaks
	source_position_recorder crs;
	crs.update(U'\r');
	crs.update(U'\r');
	crs.update(U'\n');
	check(crs.position().line == 3, "CR CR LF is three line breaks");

	// clearing then feeding the same text equals feeding it to a fresh recorder
	source_position_recorder a;
	a.update(U'x');
	a.update(U'\r');
	a.clear();
	a.update(U'y');
	source_position_recorder b;
	b.update(U'y');
	check(a.position() == b.position(), "clear() then feeding equals a fresh recorder");

	return failures;
}
