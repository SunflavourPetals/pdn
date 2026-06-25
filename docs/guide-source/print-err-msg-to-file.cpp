#include <iostream>
#include <fstream>
#include <string>

#include "spdn.h"

class my_function_package
{
public:
    auto position() const -> pdn::source_position
    {
        return dfp.position();
    }
    void update(const char32_t c)
    {
        dfp.update(c);
    }
    void handle_error(const pdn::error_message& msg)
    {
        dfp.handle_error(msg, out);
    }
    auto generate_error_message(pdn::raw_error_message raw) -> pdn::error_msg_string
    {
        return dfp.generate_error_message(std::move(raw));
    }
    auto generate_constant(const pdn::unicode::u8string& iden) -> ::std::optional<pdn::u8entity>
    {
        return dfp.generate_constant(iden);
    }
    auto generate_type(const pdn::type::u8string& iden) -> pdn::type_code
    {
        return dfp.generate_type(iden);
    }
    my_function_package()
    {
        out.open("print-err-msg-to-file.txt");
    }
private:
    pdn::default_function_package<char8_t> dfp{};
    std::ofstream out;
};

int main()
{
    using namespace std::string_view_literals;

    auto mfp = my_function_package{};
    auto e = pdn::parse(u8R"(100)"sv, mfp, mfp, mfp, pdn::utf8_tag);
}
