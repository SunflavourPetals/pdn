#include <iostream>
#include <string>
#include <cassert>
#include <exception>

#include "spdn.h"

template <typename char_t>
void parse_something()
{
    auto example1_using_utf_8_opt  = pdn::parse("./example.spdn", pdn::utf8_tag);
    // auto example2_using_utf_16_opt = pdn::parse("./example.spdn", pdn::utf16_tag);
    // auto example3_using_utf_32_opt = pdn::parse("./example.spdn", pdn::utf32_tag);
    // too many sections

    auto example1 = pdn::parse("./example.spdn", char_t{});
    // equals
    auto example2 = pdn::parse<char_t>("./example.spdn");
}

void access_test()
{
    using namespace std::string_view_literals;
    const auto src = u8R"(//source
auto_int : int 0;
auto_uint:uint 1;
i8 :i8  - 8;
i16:i16 -16;
i32:i32 -32;
i64:i64 -64;
u8 :u8    8;
u16:u16  16;
u32:u32  32;
u64:u64  64;
f32:f32 1.0;
f64:f64 2.0;
bool @true;
char 'C';
str "string";
object {
    m1 1;
    m2 "member";
    m3 [1, 2, 3];
}
list [
    { num 0 },
    [ 0 ],
    0,
    0.0,
    '0',
    @false,
]
)"sv;

    using namespace pdn;
    const auto& e = parse(src, utf8_tag);
    const auto r = e.ref();

    using namespace pdn::type;
    // get
    auto unused1 = get<auto_int>(e[u8"auto_int"]);
    auto unused2 = get<object<char8_t>>(e[u8"object"]);
    auto test_i64 = get<i64>(e[u8"i64"]);
    assert(test_i64 == -64);
    // get_if
    auto test_list_p = get_if<list<char8_t>>(e[u8"list"]);
    assert(test_list_p != nullptr);
    auto test_nonexistence_p = get_if<list<char8_t>>(r[u8"nonexistence"]);
    assert(test_nonexistence_p == nullptr);
    // get_opt
    auto test_auto_uint_opt = get_opt<auto_uint>(e[u8"auto_uint"]);
    assert(test_auto_uint_opt && (*test_auto_uint_opt == 1));
    auto test_nonexistence_opt = get_opt<i32>(r[u8"nonexistence"]);
    assert(static_cast<bool>(test_nonexistence_opt) == false);
    // as
    auto test_as_u32 = as_uint(e[u8"u32"], u32_tag);
    static_assert(::std::same_as<decltype(test_as_u32), u32>);
    assert(test_as_u32 == 32);
    auto test_as_i8 = as_int(r[u8"u64"], i8_tag);
    static_assert(::std::same_as<decltype(test_as_i8), i8>);
    assert(test_as_i8 == 64);
    auto test_as_string = as_string(r[u8"char"]);
    assert(test_as_string == string<char8_t>{});
}

int main() try
{
    parse_something<char8_t>();
}
catch (std::exception& e)
{
    std::cerr << e.what() << "\n";
}
catch (...)
{
    std::cerr << "unknown exception\n";
}
