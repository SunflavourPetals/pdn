#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <iostream>
#include <string>
#include <cstddef>
#include <stdexcept>

#include "pdn_error_message.h"
#include "pdn_error_handler_concept.h"

#ifdef PDN_NO_EXCEPTIONS
#error "pdn_error_hander.h not support PDN_NO_EXCEPTIONS"
#endif

namespace pdn::detail
{
	inline auto error_msg_for_filename(error_msg_string filename, const pdn::error_message& msg) -> pdn::error_message
	{
		filename.append(u8": "_em);
		filename.append(msg.error_message);
		return { .error_code = msg.error_code, .position = msg.position, .error_message = ::std::move(filename) };
	}
}

namespace pdn
{
	class default_error_handler
	{
	public:
		static void handle_error(const error_message& e)
		{
			handle_error(e, ::std::cerr);
		}
		static void handle_error(const error_message& e, ::std::ostream& out)
		{
			out << ::std::string_view{ reinterpret_cast<const char*>(e.error_message.data()), e.error_message.size() } << "\n";
		}
	};
	static_assert(concepts::error_handler<default_error_handler>);

	class default_threshold_error_handler
	{
	public:
		using error_count_t = unsigned;
		inline static constexpr auto default_limit = error_count_t{ 100 };
		void handle_error(const error_message& e)
		{
			handle_error(e, ::std::cerr);
		}
		void handle_error(const error_message& e, ::std::ostream& out)
		{
			++error_count;
			default_error_handler::handle_error(e, out);
			if (error_count >= limit) throw ::std::runtime_error{ "too many parsing errors" };
		}
		void clear() noexcept
		{
			error_count = 0;
		}
		auto count() const noexcept -> error_count_t
		{
			return error_count;
		}
		default_threshold_error_handler() = default;
		explicit default_threshold_error_handler(error_count_t max_error_count) : limit{ max_error_count } {}
	private:
		error_count_t error_count{};
	public:
		// Throws an exception once accumulated errors reach limit. If limit is 0, handle_error(...) behaves as if limit were 1.
		error_count_t limit{ default_limit };
	};
	static_assert(concepts::error_handler<default_threshold_error_handler>);
}

#endif
