#ifndef PDN_Header_pdn_function_package
#define PDN_Header_pdn_function_package

#include <iosfwd>
#include <utility>
#include <type_traits>

#include "pdn_source_position_recorder.h"
#include "pdn_error_handler.h"
#include "pdn_error_message_generator.h"
#include "pdn_constant_generator.h"
#include "pdn_type_generator.h"

namespace pdn
{
	template <typename char_t>
	class default_function_package
	{
	public:
		using error_count_t = default_threshold_error_handler::error_count_t;
		inline static constexpr auto default_limit = default_threshold_error_handler::default_limit;
		auto fill_filename(const pdn::error_message& msg) -> pdn::error_message
		{
			return detail::error_msg_for_filename(filename(), msg);
		}
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
		void clear_error_count() noexcept
		{
			err_handler.clear();
		}
		auto error_count() const noexcept
		{
			return err_handler.count();
		}
		void reset_error_threshold(const error_count_t max_error_count) noexcept
		{
			err_handler.limit = max_error_count;
		}
		auto error_threshold() const noexcept
		{
			return err_handler.limit;
		}
		auto position() const -> pdn::source_position
		{
			return pos_recorder.position();
		}
		void update(const char32_t c)
		{
			pos_recorder.update(c);
		}
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
		void handle_error()
		{
			err_handler.handle_error();
		}
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
		default_function_package() = default;
		explicit default_function_package(error_count_t max_error_count) :
			err_handler{ max_error_count } {}
		explicit default_function_package(error_msg_string filename, error_count_t max_error_count = default_limit) :
			err_handler{ max_error_count },
			parsing_filename{ ::std::move(filename) } {}
	private:
		source_position_recorder        pos_recorder{};
		default_threshold_error_handler err_handler{};
		error_msg_string                parsing_filename{};
	};
}

#endif
