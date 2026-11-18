#include <iostream>
#include <string>

#include <pdn/spdn.h>

#include "outu8sv.h"

int main()
{
    using namespace pdn;
    using namespace std::string_view_literals;

    auto result = parse(u8R"(int 1 string "hello")"sv, utf8_tag);
    if (!result)
    {
        std::cerr << "parse failed: " << result.what() << "\n";
        return 1;
    }
    auto& e = *result;

    using et = u8entity; // entity<char8_t>

    auto& int_of_e = e[u8"int"sv].get<et::auto_int>();
    std::cout << int_of_e << "\n"; // 1
    int_of_e = -1; // e[u8"int"] <- -1
    std::cout << e[u8"int"sv].get<et::auto_int>() << "\n"; // -1
    std::cout << get<et::string>(e[u8"string"sv]) << "\n"; // hello
}
