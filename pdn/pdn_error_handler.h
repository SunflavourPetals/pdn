#ifndef PDN_Header_pdn_error_handler
#define PDN_Header_pdn_error_handler

#include <iostream>
#include <string>
#include <cstddef>
#include <stdexcept>

#include "pdn_error_message.h"
#include "pdn_error_handler_concept.h"

namespace pdn
{
	class default_error_handler
	{
	public:
		void handle_error(const error_message& e) const
		{
			handle_error(e, ::std::cerr);
		}
		void handle_error(const error_message& e, ::std::ostream& out) const
		{
			out << ::std::string_view{ reinterpret_cast<const char*>(e.error_message.data()), e.error_message.size() } << "\n";
		}
	};
	static_assert(concepts::error_handler<default_error_handler>);

	class default_threshold_error_handler : public default_error_handler
	{
	public:
		using error_count_t = unsigned;
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
		error_count_t limit{ 255 };
	};
	static_assert(concepts::error_handler<default_threshold_error_handler>);
}

#endif
