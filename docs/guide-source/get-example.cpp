#include <iostream>
#include <string>
#include <exception>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
    using namespace pdn;
    using namespace std::string_view_literals;

    auto e = parse(u8R"(int 1 string "hello")"sv, utf8_tag);

    using et = decltype(e); // entity<char8_t>

    try
    {
        auto& int_of_e = e[u8"int"sv].get<et::auto_int>();
        std::cout << int_of_e << "\n"; // 1
        int_of_e = -1; // e[u8"int"] <- -1
        std::cout << e[u8"int"sv].get<et::auto_int>() << "\n"; // -1
        std::cout << get<et::string>(e[u8"string"sv]) << "\n"; // hello
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
    }
}
