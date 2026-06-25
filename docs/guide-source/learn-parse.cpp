#include <exception>
#include <string>

#include "spdn.h"

int main() try
{
    using namespace std::literals;
    
    auto spdn_text = u8R"(test: "test")"sv;
    auto spdn_filename = "hello.spdn"s;

    // parse UTF8 string
    [[maybe_unused]]
    auto e1 = pdn::parse(spdn_text, pdn::utf8_tag); // utf16_tag utf32_tag

    // parse spdn file
    [[maybe_unused]]
    auto e2 = pdn::parse(spdn_filename, pdn::utf8_tag); // utf16_tag utf32_tag

    // my function package
    class my_fn_pkg : public pdn::default_function_package<char8_t> {};

    auto fp = my_fn_pkg{};

    // parse UTF8 string
    [[maybe_unused]]
    auto e3 = pdn::parse(spdn_text, fp, fp, fp, pdn::utf8_tag); // utf8 only

    // parse spdn file
    [[maybe_unused]]
    auto e4 = pdn::parse(spdn_filename, fp, fp, fp, pdn::utf8_tag); // utf8 only
}
catch (std::exception& e)
{
    std::cerr << e.what() << "\n";
}
catch (...)
{
    std::cerr << "unknown exception\n";
}
