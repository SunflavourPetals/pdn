#include <cstdio>
#include <type_traits>

#include <pdn/pdn_source_position_and_newline_recorder.h>

namespace
{
	using pdn::newline_modes;
	using pdn::source_position_and_newline_recorder;
	using recorder = source_position_and_newline_recorder;

	static_assert(pdn::concepts::source_position_recorder<recorder>);

	// a fresh recorder: position { 1, 1 }, no counters, no newline mode
	static_assert([] { return recorder{}.position().line; }() == 1);
	static_assert([] { return recorder{}.position().column; }() == 1);
	static_assert([] { return recorder{}.newline_mode(); }() == newline_modes::NONE);
	static_assert([] { return recorder{}.is_no_newline(); }());
	static_assert(![] { return recorder{}.is_mixed_newline(); }());

	// one kind of line break keeps the mode "coincident"
	static_assert([] {
		recorder r;
		r.update(U'\n');
		return r.newline_mode();
	}() == newline_modes::LF);
	static_assert([] {
		recorder r;
		r.update(U'\n');
		return r.is_coincident_newline();
	}());
	// two kinds make it mixed
	static_assert([] {
		recorder r;
		r.update(U'\n');
		r.update(U'\r');
		return r.is_mixed_newline();
	}());
	// and coincident-ness is exactly the negation of mixed-ness
	static_assert([] {
		recorder r;
		r.update(U'\n');
		r.update(U'\r');
		return r.is_coincident_newline() == !r.is_mixed_newline();
	}());

	// counters are per kind
	static_assert([] {
		recorder r;
		r.update(U'\n');
		r.update(U'\n');
		r.update(U'\u0085');
		r.update(U'\u2028');
		r.update(U'\u2029');
		return r.LF_count() == 2 && r.NEL_count() == 1 && r.LS_count() == 1 && r.PS_count() == 1;
	}());
	// CR followed by LF is folded: one CRLF, zero CR
	static_assert([] {
		recorder r;
		r.update(U'\r');
		r.update(U'\n');
		return r.CRLF_count() == 1 && r.CR_count() == 0 && r.LF_count() == 0;
	}());
	// two CRs then LF: one CR, one CRLF
	static_assert([] {
		recorder r;
		r.update(U'\r');
		r.update(U'\r');
		r.update(U'\n');
		return r.CR_count() == 1 && r.CRLF_count() == 1;
	}());
	// CRLF advances the line once, not twice
	static_assert([] {
		recorder r;
		r.update(U'\r');
		r.update(U'\n');
		return r.position().line;
	}() == 2);

	// NEL, LS and PS are line breaks (matching the lexer's is_new_line)
	static_assert([] {
		recorder r;
		r.update(U'\u0085');
		r.update(U'\u2028');
		r.update(U'\u2029');
		return r.position().line;
	}() == 4);

	// newline_mode() is the bit set of the counters that are non-zero
	static_assert([] {
		recorder r;
		r.update(U'\r');
		r.update(U'\n');
		r.update(U'x');
		r.update(U'\n');
		return r.newline_mode();
	}() == (newline_modes::CRLF | newline_modes::LF));

	// clear() restores the state of a freshly constructed recorder: the counters,
	// the mode, and the "previous code point was not CR" flag
	static_assert([] {
		recorder r;
		r.update(U'\n');
		r.update(U'\r');
		r.clear();
		return r.newline_mode() == newline_modes::NONE && r.is_no_newline();
	}());
	static_assert([] {
		recorder r;
		r.update(U'\r');
		r.clear();
		r.update(U'\n');
		return r.LF_count() == 1 && r.CRLF_count() == 0 && r.CR_count() == 0 && r.position().line == 2;
	}());
	static_assert([] {
		recorder cleared = [] {
			recorder r;
			r.update(U'\r');
			r.clear();
			return r;
		}();
		recorder fresh{};
		return cleared.position() == fresh.position() && cleared.newline_mode() == fresh.newline_mode();
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

	// a small mixed document: CRLF, then LF, then a lone CR at the end
	recorder r;
	r.update(U'a');
	r.update(U'\r');
	r.update(U'\n');
	r.update(U'b');
	r.update(U'\n');
	r.update(U'c');
	r.update(U'\r');
	const auto p = r.position();
	check(p.line == 4 && p.column == 1, "mixed document ends at 4:1");
	check(r.CRLF_count() == 1 && r.LF_count() == 1 && r.CR_count() == 1, "one CRLF, one LF, one lone CR");
	check(r.newline_mode() == (newline_modes::CRLF | newline_modes::LF | newline_modes::CR), "mode has three bits");
	check(r.is_mixed_newline() && !r.is_coincident_newline(), "three styles are mixed");

	// clear() then the same text equals a fresh recorder fed the same text
	recorder a;
	a.update(U'x');
	a.update(U'\n');
	a.clear();
	a.update(U'y');
	recorder b;
	b.update(U'y');
	check(a.position() == b.position(), "clear() then feeding equals a fresh recorder");
	check(a.newline_mode() == b.newline_mode(), "clear() then feeding equals a fresh recorder's mode");

	return failures;
}
