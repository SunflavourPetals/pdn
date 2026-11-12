#include <iostream>
#include <string>

#include <pdn/spdn.h>

#include "outu8sv.h"

int main()
{
    using namespace pdn;
    using namespace std::string_view_literals;

    auto result = parse(u8R"(hello "hello")"sv, utf8_tag);
    if (!result)
    {
        std::cerr << "parse failed: " << result.what() << "\n";
        return 1;
    }
    const auto& e = *result;

    using et = u8entity; // entity<char8_t>

    std::cout << type_test<et::auto_int>(e[u8"hello"sv]) << "\n"; // 0
    std::cout << type_test<et::string>(e[u8"hello"sv]) << "\n"; // 1
}
