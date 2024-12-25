# 本仓库实现的 PDN 解析器

## parse

使用方法：  

`pdn::parse` 方法有对 `token` 迭代器、UTF 码元迭代器、UTF 编码的字符串视图、文件流、文件名的重载版本，根据其需求，用户最多需要提供 `function_package_for_parser`、`function_package_for_lexer`、`function_package_for_code_point_iterator` 给 `pdn::parse` 函数。  
`pdn::parse` 方法对用户提供 `function_package` 和使用默认配置有重载，一般使用默认配置已足够。  
`pdn::parse` 方法需要用户指定编码方式，在有确定的方案时推荐使用 `pdn::utf_8_tag`、`pdn::utf_16_tag`、`pdn::utf_32_tag` 作为 `pdn::parse` 的 `char_t` 参数来提供。  

``` C++
#include "pdn_parse.h"

template <typename char_t>
void parse_something()
{
    auto dom_1_using_utf_8  = pdn::parse("./example_1.spdn", pdn::utf_8_tag);
    auto dom_2_using_utf_16 = pdn::parse("./example_2.spdn", pdn::utf_16_tag);
    auto dom_3_using_utf_32 = pdn::parse("./example_3.spdn", pdn::utf_32_tag);

    auto dom_1 = pdn::parse("./example_1.spdn", char_t{});
    // 等价于
    auto dom_2 = pdn::parse<char_t>("./example_1.spdn");
}
```

用户提供 `function_package`，可参考 `pdn::concepts::function_package_for...` 等概念实现(分别定义于 `pdn_parser.h`、`pdn_lexer.h`、`pdn_code_point_iterator.h`)。  
记录行列位置信息的功能由 `function_package_for_code_point_iterator` 提供；`function_package_for_lexer` 需要获得解析点行列位置；`pdn` 的 `@val` 常量和类型名由 `function_package_for_parser` 提供。  
此外，以上三者都需要错误处理和错误信息生成功能，默认的错误处理为打印错误信息到标准错误流，默认的错误信息生成方法生成英文错误信息。  
最佳实践是组合 `pdn_function_package.h` 内的 `pdn::default_function_package` 和用户需要自定义的内容，使它满足以上三者的需求，然后为它们三个提供同一个`function_package` 对象(将按引用传递)。

## data_entity

解析后将得到一个持有 `object` 的 `data_entity` (数据实体)，是 `std::variant<pdn_data_types...>` 的派生类，除了可以用 `std::get` `std::get_if` `std::visit` 等方法访问，还提供如下访问函数(头文件 `pdn_data_entity.h`，名称空间 `pdn` 内)：  

1. `get` 系列函数：对代理进行处理的 `std::get` 的封装，不支持 `refer` 和 `const_refer` 版本；  
2. `get_ptr` 系列函数：获得相应类型的值或得到 nullptr；  
3. `get_optional` 系列函数：仅对基本类型提供(整数、浮点数、布尔和字符)，获得相应类型的 `optional` 或得到 `nullopt`；  
4. `as_xxx` 系列函数： 将实体转换为相应类型的值，如果无法转换，将获得一个零初始化或默认初始化的目标类型的值。  

`dom` 还有如下成员方法：  

1. `ref` `cref`：获得 `refer` 或 `const_refer` 类，它们有 `operator[]` 和 `at` 方法，但查询失败时都返回空 `refer|const_refer`。  
2. `operator[string_view<char_t>]`：获得 Object 的成员数据，如果当前实体不是 Object 或没有相应成员数据，将抛出异常；  
3. `operator[index]`：获得 List 的元素数据，如果当前实体不是 List 将抛出异常，且无范围检查；  
4. `at(string_view<char_t>)`：获得 Object 的成员数据的 `refer|const_refer`，如果当前实体不是 Object 或没有相应成员数据，返回空 `refer|const_refer`；  
5. `at(index)`：获得 List 的元素数据的 `refer|const_refer`，如果当前实体不是 List 或 out of range，返回空 `refer|const_refer`；  

`refer` 和 `const_refer` 类提供不抛出的 `operator[]`，当查询失败时，返回一个空 `refer|const_refer`，对空 `refer|const_refer` 的任何成员查询操作都只能得到空 `refer|const_refer`，获取值时只能获得 `nullptr`、`nullopt` 或是无效的值。  
`refer` 和 `const_refer` 类不支持 `pdn::get`，也不支持 `std::get` `std::get_if` `std::visit` 等方法。  
