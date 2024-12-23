#ifndef PDN_Header_pdn_exception
#define PDN_Header_pdn_exception

#include <stdexcept>

namespace pdn
{
	class runtime_error : public ::std::runtime_error
	{
	public:
		using ::std::runtime_error::runtime_error;
	};

	class inner_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};
}

#endif
