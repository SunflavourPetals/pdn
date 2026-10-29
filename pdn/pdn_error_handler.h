#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <iostream>
#include <string>
#include <cstddef>
#include <stdexcept>

#include "pdn_error_message.h"
#include "pdn_error_handler_concept.h"

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
		using error_count_t = ::std::size_t;
		void handle_error(const error_message& e)
		{
			handle_error(e, ::std::cerr);
		}
		void handle_error(const error_message& e, ::std::ostream& out)
		{
			++error_cnt;
			out << ::std::string_view{ reinterpret_cast<const char*>(e.error_message.data()), e.error_message.size() } << "\n";
		}
		// only increase error count by 1
		void handle_error()
		{
			++error_cnt;
		}
		void clear() noexcept
		{
			error_cnt = 0;
		}
		auto error_count() const noexcept -> error_count_t
		{
			return error_cnt;
		}
	private:
		error_count_t error_cnt{};
	};
	static_assert(concepts::error_handler<default_error_handler>);

	class default_threshold_error_handler
	{
	public:
		using error_count_t = default_error_handler::error_count_t;
		inline static constexpr auto default_limit = error_count_t{ 100 };
		void handle_error(const error_message& e)
		{
			handle_error(e, ::std::cerr);
		}
		void handle_error(const error_message& e, ::std::ostream& out)
		{
			err_handler.handle_error(e, out);
			if (err_handler.error_count() >= limit)
			{
#ifdef PDN_NO_EXCEPTIONS
#ifdef PDN_USER_TERMINATE
				PDN_USER_TERMINATE("pdn::default_threshold_error_handler::handle_error(error_message const&, ::std::ostream&)", "too many parsing errors");
#endif
				::std::terminate();
#else
				throw ::std::runtime_error{ "too many parsing errors" };
#endif
			}
		}
		// only increase error count by 1, and throw an exception if error count exceeds limit
		void handle_error()
		{
			err_handler.handle_error();
			if (err_handler.error_count() >= limit)
			{
#ifdef PDN_NO_EXCEPTIONS
#ifdef PDN_USER_TERMINATE
				PDN_USER_TERMINATE("pdn::default_threshold_error_handler::handle_error()", "too many parsing errors");
#endif
				::std::terminate();
#else
				throw ::std::runtime_error{ "too many parsing errors" };
#endif
			}
		}
		void clear() noexcept
		{
			err_handler.clear();
		}
		auto error_count() const noexcept -> error_count_t
		{
			return err_handler.error_count();
		}
		default_threshold_error_handler() = default;
		explicit default_threshold_error_handler(error_count_t max_error_count) : limit{ max_error_count } {}
	private:
		default_error_handler err_handler{};
	public:
		// Throws an exception once accumulated errors reach limit. If limit is 0, handle_error(...) behaves as if limit were 1.
		error_count_t limit{ default_limit };
	};
	static_assert(concepts::error_handler<default_threshold_error_handler>);
}

#endif
