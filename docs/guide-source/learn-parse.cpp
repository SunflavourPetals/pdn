#include <cassert>
#include <iostream>
#include <string>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
    using namespace std::literals;

    auto spdn_text = u8R"(test: "test")"sv;
    auto spdn_filename = "hello.spdn"s;

    auto e1 = pdn::parse(spdn_text, pdn::utf8_tag);
    assert(e1);

    auto e2 = pdn::parse(spdn_filename, pdn::utf8_tag);
    if (!e2)
    {
        std::cerr << "failed to parse " << spdn_filename << ": " << e2.what() << "\n";
    }

    class my_fn_pkg : public pdn::default_function_package<char8_t> {};

    auto fp = my_fn_pkg{};

    auto e3 = pdn::parse(spdn_text, fp, fp, fp, pdn::utf8_tag);
    assert(e3);

    fp.clear_position();
    fp.clear_error();

    auto e4 = pdn::parse(spdn_filename, fp, fp, fp, pdn::utf8_tag);
    if (!e4)
    {
        std::cerr << "failed to parse " << spdn_filename << ": " << e4.what() << "\n";
    }
}
