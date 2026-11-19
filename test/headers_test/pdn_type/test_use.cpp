// Second half of the pdn_type.h test: once pdn_entity.h is included, the aliases can be used.
//
// Splitting this out matters: asking whether entity is complete before and after its definition
// in the same translation unit would make the answer depend on the point of instantiation.

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <pdn/pdn_type.h>
#include <pdn/pdn_entity.h>

namespace
{
	int failures = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	template <typename t>
	concept complete = requires { sizeof(t); };
}

static_assert(complete<pdn::entity<char8_t>>);
static_assert(std::same_as<pdn::u8entity, pdn::entity<char8_t>>);
static_assert(std::same_as<pdn::type::u8list, std::vector<pdn::u8entity>>);
static_assert(std::same_as<pdn::type::u8object, pdn::type::object<char8_t>>);

int main()
{
	pdn::type::u8list list;
	list.reserve(4);
	check(list.empty(), "fresh list must be empty");

	pdn::type::u8object object;
	object.reserve(4);
	check(object.empty(), "fresh object must be empty");
	check(object.find(std::u8string_view{ u8"absent" }) == object.end(), "missing keys must not be found");
	check(object.count(std::u8string_view{ u8"absent" }) == 0, "missing keys must not be counted");

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}

	std::printf("all checks passed\n");
	return 0;
}
