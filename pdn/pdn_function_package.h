#ifndef PDN_Header_pdn_function_package
#define PDN_Header_pdn_function_package

#include <iosfwd>

#include "pdn_source_position_recorder.h"
#include "pdn_error_handler.h"
#include "pdn_error_message_generator.h"
#include "pdn_constants_generator.h"
#include "pdn_type_generator.h"

namespace pdn
{
	template <typename char_t>
	class default_function_package
	{
	private:
		pdn::source_position_recorder            pos_recorder{};
		pdn::default_error_handler               err_handler{};
		pdn::default_error_message_generator     err_msg_gen{};
		pdn::default_constants_generator<char_t> const_gen{};
		pdn::default_type_generator<char_t>      type_gen{};
	public:
		auto position() const -> pdn::source_position
		{
			return pos_recorder.position();
		}
		void update(char32_t c)
		{
			pos_recorder.update(c);
		}
		void handle_error(const pdn::error_message& msg) const
		{
			err_handler.handle_error(msg);
		}
		void handle_error(const pdn::error_message& msg, ::std::ostream& out) const
		{
			err_handler.handle_error(msg, out);
		}
		auto generate_error_message(pdn::error_code_variant errc_variant, pdn::error_msg_string err_msg_str) const -> pdn::error_msg_string
		{
			return err_msg_gen.generate_error_message(std::move(errc_variant), std::move(err_msg_str));
		}
		auto generate_constant(const pdn::unicode::utf_8_code_unit_string& iden) const -> ::std::optional<pdn::constant_variant<char_t>>
		{
			return const_gen.generate_constant(iden);
		}
		auto generate_type(const types::string<char_t>& iden) const -> type_code
		{
			return type_gen.generate_type(iden);
		}
	};
}

#endif
