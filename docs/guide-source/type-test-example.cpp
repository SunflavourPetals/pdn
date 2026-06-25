#include <iostream>
#include <string>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
    using namespace pdn;
    using namespace std::string_view_literals;

    auto e = parse(u8R"(hello "hello")"sv, utf8_tag);

    using et = decltype(e); // entity<char8_t>

    std::cout << type_test<et::auto_int>(e[u8"hello"sv]) << "\n"; // 0
    std::cout << type_test<et::string>(e[u8"hello"sv]) << "\n"; // 1
}
