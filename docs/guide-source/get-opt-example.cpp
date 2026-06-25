#include <iostream>
#include <string>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
    using namespace pdn;
    using namespace std::string_view_literals;

    auto e = parse(u8R"(int 1 big 0xffff'ffff'ffff'ffff)"sv, utf8_tag);

    using et = decltype(e); // entity<char8_t>

    if (auto opt = e[u8"int"sv].get_opt<et::auto_int>())
    {
        std::cout << *opt << "\n"; // 1
    }
    if (auto opt = get_opt<et::u64>(e[u8"big"sv])) // not auto_int
    {
        std::cout << *opt << "\n"; // 18446744073709551615
    }
}
