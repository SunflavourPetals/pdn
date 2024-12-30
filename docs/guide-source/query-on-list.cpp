#include <iostream>
#include <string>
#include <cassert>

#include "pdn_parse.h"
#include "pdn_data_entity.h"

#include "outu8sv.h"

int main()
{
    using namespace std::string_view_literals;
    using namespace pdn;
    auto entity = parse(u8R"(list [1, 2, 3, "hello", 5.0])"sv, utf_8_tag);

    const auto& list = entity[u8"list"];

    std::cout << as_int(list[0]) << "\n";
    std::cout << as_int(list[1]) << "\n";
    std::cout << as_int(list[2]) << "\n";
    std::cout << as_string(list[3]) << "\n";
    std::cout << as_fp(list[4]) << "\n";

    // std::cout << as_int(list[5]) << "\n";

    try
    {
        auto& e5 = as_list(list).at(5);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    auto list_ref = list.ref();

    std::cout << as_string(list_ref[3]) << "\n";

    auto elem_1 = list_ref[1];
    auto elem_1_0 = elem_1[0];
    assert(!elem_1_0.has_value());
    auto elem_5 = list_ref[5];
    assert(!elem_5.has_value());
    auto nullref = elem_5[0];
    assert(!nullref.has_value());
}
