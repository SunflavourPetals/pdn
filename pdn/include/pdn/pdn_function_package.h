#ifndef PDN_Header_pdn_function_package
#define PDN_Header_pdn_function_package

#include <iosfwd>
#include <cstddef>
#include <optional>
#include <utility>

#include "pdn_source_position_recorder.h"
#include "pdn_source_position_recorder_concept.h"
#include "pdn_error_handler.h"
#include "pdn_error_handler_concept.h"
#include "pdn_error_message_generator.h"
#include "pdn_error_message_generator_concept.h"
#include "pdn_constant_generator.h"
#include "pdn_constant_generator_concept.h"
#include "pdn_type_generator.h"
#include "pdn_type_generator_concept.h"
#include "pdn_stop_reason.h"
#include "pdn_stoppable_concept.h"
#include "pdn_nesting_depth_limiter.h"
#include "pdn_nesting_depth_limiter_concept.h"
#include "pdn_unicode_base.h"

// The default implementation of the function package contract. It bundles a source position
// recorder, a threshold error handler that can prefix every diagnostic with a file name, and the
// three default generators, so pdn_parse.h can parse without any help from the user. Everything
// is per-parse state: call clear_error() and clear_position() before reusing the object, and
// derive from it (or write the members by hand) when a hook has to behave differently.

namespace pdn
{
	template <typename char_t>
	class default_function_package
	{
	public:
		// The counter type of the threshold handler, and the threshold it starts with.
		using error_count_t = default_threshold_error_handler::error_count_t;
		static constexpr auto default_error_limit = default_threshold_error_handler::default_limit;
		// The nesting limit reported below; pdn_parser.h falls back to the same number when a
		// package does not implement the limit at all.
		static constexpr auto nesting_depth_limit() noexcept -> ::std::size_t
		{
			return default_nesting_depth_limiter::nesting_depth_limit();
		}
		// Forwarded from the threshold handler: it asks to stop once the error count reaches the
		// limit, with stop_reason_code::parse_error_limit.
		bool stop_requested() const noexcept
		{
			return err_handler.stop_requested();
		}
		auto stop_reason() const noexcept -> stop_reason_code
		{
			return err_handler.stop_reason();
		}
		// Prepends "<filename>: " to a diagnostic. handle_error() calls this before forwarding,
		// so the handler only ever sees the finished text.
		auto fill_filename(const pdn::error_message& msg) -> pdn::error_message
		{
			return detail::error_msg_for_filename(filename(), msg);
		}
		// The file name used only to prefix diagnostics; an empty name means "no prefix".
		void set_filename(pdn::error_msg_string name)
		{
			parsing_filename = ::std::move(name);
		}
		void clear_filename() noexcept
		{
			parsing_filename.clear();
		}
		auto filename() const noexcept -> const pdn::error_msg_string&
		{
			return parsing_filename;
		}
		// Resets the error count and the stop state, so the same package can parse again.
		void clear_error() noexcept
		{
			err_handler.clear();
		}
		auto error_count() const noexcept
		{
			return err_handler.error_count();
		}
		// Changes the threshold without touching the count; a limit of 0 stops at the first error
		// (see default_threshold_error_handler).
		void reset_error_threshold(const error_count_t max_error_count) noexcept
		{
			err_handler.limit = max_error_count;
		}
		auto error_threshold() const noexcept
		{
			return err_handler.limit;
		}
		// The position the code point iterator feeds: clear it before reusing the package for
		// another parse, otherwise the first diagnostic of the new run reports a stale position.
		void clear_position() noexcept
		{
			pos_recorder.clear();
		}
		auto position() const noexcept -> pdn::source_position
		{
			return pos_recorder.position();
		}
		void update(const char32_t c) noexcept
		{
			pos_recorder.update(c);
		}
		// Prints and counts one diagnostic, with the file name prefix when one is set.
		void handle_error(const pdn::error_message& msg)
		{
			if (filename().empty())
			{
				err_handler.handle_error(msg);
			}
			else
			{
				err_handler.handle_error(fill_filename(msg));
			}
		}
		// Same as above, but writes to `out` instead of std::cerr.
		void handle_error(const pdn::error_message& msg, ::std::ostream& out)
		{
			if (filename().empty())
			{
				err_handler.handle_error(msg, out);
			}
			else
			{
				err_handler.handle_error(fill_filename(msg), out);
			}
		}
		// Counts one error without printing anything (for a caller that only needs the count).
		// Sets parse_error_limit once the count reaches the threshold.
		// only increase error count by 1, and set parse_error_limit if error count exceeds limit
		void handle_error()
		{
			err_handler.handle_error();
		}
		// The package hooks used by the parser, forwarded to the default implementations.
		static auto generate_error_message(pdn::raw_error_message raw) -> pdn::error_msg_string
		{
			return default_error_message_generator::generate_error_message(std::move(raw));
		}
		static auto generate_constant(const pdn::unicode::u8string& iden) -> ::std::optional<pdn::entity<char_t>>
		{
			return default_constant_generator<char_t>::generate_constant(iden);
		}
		static auto generate_type(const type::string<char_t>& iden) -> pdn::type_code
		{
			return default_type_generator<char_t>::generate_type(iden);
		}
		// Default state: no file name, the default threshold, the position at the start.
		default_function_package() = default;
		explicit default_function_package(error_count_t max_error_count) :
			err_handler{ max_error_count } {}
		explicit default_function_package(error_msg_string filename, error_count_t max_error_count = default_error_limit) :
			err_handler{ max_error_count },
			parsing_filename{ ::std::move(filename) } {}
	private:
		source_position_recorder        pos_recorder{};
		default_threshold_error_handler err_handler{};
		error_msg_string                parsing_filename{};
	};
}

namespace pdn::detail
{
	// Every concept the library accepts a package through: the code point iterator needs the
	// position recorder plus the handler and the message generator, the lexer needs the getter
	// half of the recorder with the same two, and the parser needs the handler, the message
	// generator and both generators. stoppable and nesting_depth_limiter are checked as well
	// and the guide promises them.
	template <typename char_t>
	concept complete_function_package =
		concepts::source_position_recorder<default_function_package<char_t>> &&
		concepts::error_handler<default_function_package<char_t>> &&
		concepts::error_message_generator<default_function_package<char_t>> &&
		concepts::type_generator<default_function_package<char_t>, char_t> &&
		concepts::constant_generator<default_function_package<char_t>, char_t> &&
		concepts::stoppable<default_function_package<char_t>> &&
		concepts::nesting_depth_limiter<default_function_package<char_t>>;
	static_assert(complete_function_package<unicode::u8char_t>);
	static_assert(complete_function_package<unicode::u16char_t>);
	static_assert(complete_function_package<unicode::u32char_t>);
}

#endif
