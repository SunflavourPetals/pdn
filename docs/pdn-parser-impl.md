# 本仓库实现的 PDN 解析器

## parse

使用方法：  

`pdn::parse` 方法有对 `token` 迭代器、Unicode 码元迭代器、文件流、文件名的重载版本，根据其需求，用户最多需要提供 `function_package_for_parser`、`function_package_for_lexer`、`function_package_for_code_point_iterator` 给 `pdn::parse` 函数。  
`pdn::parse` 方法对用户提供 `function_package` 和使用默认配置有重载，一般使用默认配置已足够。  
`pdn::parse` 方法需要用户指定编码方式，推荐使用 `pdn::to_utf_8`、`pdn::to_utf_16`、`pdn::to_utf_32` 作为 `pdn::parse` 的 `char_t` 参数来提供。  

``` C++
#include "pdn_parse"

void parse_something()
{
    auto dom_1_using_utf_8  = pdn::parse("./example_1.pdn", pdn::to_utf_8);
    auto dom_2_using_utf_16 = pdn::parse("./example_2.pdn", pdn::to_utf_16);
    auto dom_3_using_utf_32 = pdn::parse("./example_3.pdn", pdn::to_utf_32);
}
```

用户提供 `function_package`，可参考 `pdn::concepts::function_package_for...` 等概念实现(分别定义于 `pdn_parser`、`pdn_lexer`、`pdn_code_point_iterator`)。  
记录行列位置信息的功能由 `function_package_for_code_point_iterator` 提供，`pdn` 的 `@val` 常量由 `function_package_for_lexer` 提供，此外它还需要获得解析点行列位置。`pdn` 中的类型名由 `function_package_for_parser` 提供。  
此外，以上三者都需要错误处理和错误信息生成功能，默认的错误处理为打印错误信息到标准输出，默认的错误信息生成方法生成英文错误信息。  
最佳实践是组合 `pdn_function_package.h` 内的 `pdn::default_function_package` 和用户需要自定义的内容，使它满足以上三者的需求，然后为它们三个提供同一个`function_package` 对象(将按引用传递)。

## dom

解析后将得到一个 dom，dom 是 `std::variant<pdn_data_types...>` 的派生类，除了可以用 `std::get` `std::get_if` `std::visit` 等方法查询，dom 还以成员函数的方式提供：  

1. `get_xxx` 系列函数：对代理进行处理的 `std::get` 的封装；  
2. `xxx_opt` `xxx_ptr` 系列函数：获得相应类型的值或得到 `nullopt`/`nullptr`；  
3. `xxx_val` 系列函数： 将实体转换为相应类型的值，如果无法转换，将获得一个零初始化或默认初始化的目标类型的值。  
4. `operator[const string<char_t>&]`：获得 Object 的成员数据，如果当前实体不是 Object 或没有相应成员数据，将抛出异常；  
5. `operator[size_t]`：获得 List 的元素数据，如果当前实体不是 List 或 out of range，将抛出异常；  
6. `ref` `cref`：获得 `refer` 或 `const_refer` 类。  

`refer` 和 `const_refer` 类提供不抛出的 `operator[]`，当查询失败时，返回一个空 `refer|const_refer`，对空 `refer|const_refer` 的任何成员查询操作都只能得到空 `refer|const_refer`，获取值时只能获得 `nullptr`、`nullopt` 或是无效的值。  
`refer` 和 `const_refer` 类不提供 `get_xxx` 系列的函数。  
