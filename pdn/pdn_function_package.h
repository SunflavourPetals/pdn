#ifndef PDN_Header_pdn_function_package
#define PDN_Header_pdn_function_package

#include <iosfwd>

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
		void update(char32_t c)
		{
			pos_recorder.update(c);
		}
		void handle_error(const pdn::error_message& msg)
		{
			err_handler.handle_error(msg);
		}
		void handle_error(const pdn::error_message& msg, ::std::ostream& out)
		{
			err_handler.handle_error(msg, out);
		}
		auto generate_error_message(pdn::raw_error_message raw) const -> pdn::error_msg_string
		{
			return err_msg_gen.generate_error_message(std::move(raw));
		}
		auto generate_constant(const pdn::unicode::utf_8_code_unit_string& iden) const -> ::std::optional<pdn::entity<char_t>>
		{
			return const_gen.generate_constant(iden);
		}
		auto generate_type(const types::string<char_t>& iden) const -> type_code
		{
			return type_gen.generate_type(iden);
		}
		default_function_package() = default;
		default_function_package(error_count_t max_error_count) : err_handler{ max_error_count } {}
	private:
		source_position_recorder           pos_recorder{};
		default_threshold_error_handler    err_handler{};
		default_error_message_generator    err_msg_gen{};
		default_constant_generator<char_t> const_gen{};
		default_type_generator<char_t>     type_gen{};
	};
}

#endif
