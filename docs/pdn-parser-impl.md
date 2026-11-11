# 本仓库实现的 PDN 解析器

## parse

使用方法：  

`pdn::parse` 方法有对 `token` 迭代器、UTF 码元迭代器、UTF 编码的字符串视图、文件流、文件名的重载版本，根据其需求，用户最多需要提供 `function_package_for_parser`、`function_package_for_lexer`、`function_package_for_code_point_iterator` 给 `pdn::parse` 函数。  
`pdn::parse` 方法对用户提供 `function_package` 和使用默认配置有重载，一般使用默认配置已足够。  
`pdn::parse` 方法需要用户指定编码方式，在有确定的方案时推荐使用 `pdn::utf8_tag`、`pdn::utf16_tag`、`pdn::utf32_tag` 作为 `pdn::parse` 的 `char_t` 参数来提供。  

所有版本的 `pdn::parse` 都返回 `pdn::parse_result<char_t>`。  

``` C++
#include "pdn_parse.h"

template <typename char_t>
void parse_something()
{
    auto example1_using_utf_8  = pdn::parse("./example1.spdn", pdn::utf8_tag);
    auto example2_using_utf_16 = pdn::parse("./example2.spdn", pdn::utf16_tag);
    auto example3_using_utf_32 = pdn::parse("./example3.spdn", pdn::utf32_tag);

    auto example1 = pdn::parse("./example1.spdn", char_t{});
    // 等价于
    auto example2 = pdn::parse<char_t>("./example1.spdn");
}
```

`parse_result` 的完整成员见 guide 的[解析结果 parse_result](pdn-guide.md#解析结果-parse_result)章节，常用的有：

1. `success()` `failed()` `has_value()` 和 `operator bool`：判断解析是否成功；  
2. `error_code()`：解析终止的原因，类型为 `pdn::parse_error_code`，成功时的值为 0；  
3. `what()`：`error_code()` 对应的英文名称，失败时可以直接打印；  
4. `operator*` 和 `value()`：取出解析得到的数据实体；  
5. `operator->`：访问解析结果的数据实体。  

``` C++
#include <iostream>

#include "pdn_parse.h"

void parse_file()
{
    auto result = pdn::parse("./example1.spdn", pdn::utf8_tag);
    if (!result)
    {
        std::cerr << "parse failed: " << result.what() << "\n";
        return;
    }
    const auto& entity = *result;
    // 使用 entity...
}
```

`pdn::parse_error_code` 除了 `parse_error`（普通的词法或语法错误）之外，其余取值与 `pdn::stop_reason_code` 对应，另有一个 `open_file_failed` 用于打开文件失败的情况，详见[终止解析](#终止解析)。

用户提供 `function_package`，可参考 `pdn::concepts::function_package_for...` 等概念实现(分别定义于 `pdn_parser.h`、`pdn_lexer.h`、`pdn_code_point_iterator.h`)。  
记录行列位置信息的功能由 `function_package_for_code_point_iterator` 提供；`function_package_for_lexer` 需要获得解析点行列位置；`pdn` 的 `@val` 常量和类型名由 `function_package_for_parser` 提供。  
此外，以上三者都需要错误处理、错误信息生成和错误数量查询功能；`error_handler` 概念要求提供 `handle_error(err_msg)` 和 `error_count()`。默认的错误处理为打印错误信息到标准错误流，默认的错误信息生成方法生成英文错误信息；自定义函数包需要实现或转发 `error_count()`。  
最佳实践是组合 `pdn_function_package.h` 内的 `pdn::default_function_package` 和用户需要自定义的内容，使它满足以上三者的需求，然后为它们三个提供同一个`function_package` 对象(将按引用传递)。  
`pdn::default_function_package` 还提供 `stop_requested()`、`stop_reason()` 和 `nesting_depth_limit()`，使函数包可以请求终止解析、限制嵌套层数，详见[终止解析](#终止解析)。

## 终止解析

解析可以被请求终止。`pdn::parse` 会返回一个带有对应 `error_code()` 的 `parse_result`。

从字节迭代器到解析器共有五层：字节迭代器、码元迭代器、码点迭代器、词法分析器、语法分析器，每层都可以把下层的终止状态转发到上层。按约定，只有两处操作可以触发终止请求：

1. 每层解析所用的 begin 迭代器的 `operator++()`；  
2. `function_package` 的 `handle_error(...)`。

其他操作不允许触发终止请求。终止请求一旦发出，原因不会被后续终止请求覆盖，只保留首次终止请求的原因。

需要自定义迭代器的用户应提供 `stop_requested()` 和 `stop_reason()`，即满足 `pdn::concepts::stoppable`；不满足该概念时，该层迭代器无法请求终止。

### stop_reason_code

`pdn::stop_reason_code` 的取值及含义：

| 值 | 含义 |
| :-- | :-- |
| `parse_error_limit` | 累计的错误数量达到了上限(本库的实现默认为 100) |
| `nesting_depth_exceed` | `{}` 或 `[]` 的嵌套层数超过了 `nesting_depth_limit()` |
| `bad_stream` | 流已损坏(`badbit`) |
| `stream_operation_failed` | 流操作失败 |
| `incomplete_code_unit` | 文件末尾的编码单元序列不完整，如 UTF-16 末尾只有半个码元时 |
| `user_cancelled` | 用户自行请求终止，本库的实现不使用此代码 |

### 错误数量上限

`pdn::default_threshold_error_handler` 默认在错误数达到 100 时请求终止，原因是 `parse_error_limit`，`pdn::default_function_package::default_limit` 即这个默认值。可以用 `reset_error_threshold(n)` 修改上限，用 `clear_error()` 清零错误数并清除终止状态，此后同一个函数包对象可以继续使用(需要按情况调用 `clear_position` 重置函数包对象记录的解析处的行列位置)。  

### 嵌套层数上限

`function_package` 可以提供成员 `nesting_depth_limit()`：

``` C++
#include "spdn.h"

struct my_function_package : pdn::default_function_package<char8_t>
{
    static constexpr auto nesting_depth_limit() noexcept -> ::std::size_t
    {
        return 16;
    }
};
```

未提供时使用 256。`{}` 与 `[]` 的嵌套层数超过上限时，解析以 `nesting_depth_exceed` 终止，且不会因此增加错误数。

### 自定义终止

在 `handle_error` 中请求终止，可以让解析在某次错误处理时停下：

``` C++
#include <string_view>

#include "spdn.h"

struct cancel_on_error : pdn::default_function_package<char8_t>
{
    pdn::stop_reason_code reason{};

    void handle_error(const pdn::error_message& msg)
    {
        default_function_package::handle_error(msg);
        // 检查是否已请求停止
        if (default_function_package::stop_requested())
        {
            reason = default_function_package::stop_reason();
            return;
        }
        // TODO 修改代码为当错误码为特定值时才请求停止，实现思路：先检查main中的解析结果的错误码，然后把对应错误码的判断放到这里，如果判断是对应的错误码，才设置 reason 来请求停止
        reason = pdn::stop_reason_code::user_cancelled;
    }

    bool stop_requested() const noexcept
    {
        return reason != pdn::stop_reason_code{};
    }

    auto stop_reason() const noexcept -> pdn::stop_reason_code
    {
        return reason;
    }
};

void parse_something()
{
    using namespace std::string_view_literals;
    auto fp = cancel_on_error{};
    auto result = pdn::parse(u8R"(100)"sv, fp, fp, fp, pdn::utf8_tag);
    // result.error_code() == pdn::parse_error_code::user_cancelled
}
```

派生类中的 `stop_requested()` 和 `stop_reason()` 会隐藏 `default_function_package` 的同名成员，解析器按传入的实际类型调用它们。  

## entity

`pdn::parse` 返回 `pdn::parse_result<char_t>`，成功后用 `operator*` 或 `value()` 取出数据实体，失败时用 `error_code()` 或 `what()` 查询终止解析的原因。数据实体持有 `object`，是 `std::variant<pdn_data_types...>` 的派生类，除了可以用 `std::get` `std::get_if` `std::visit` 等方法访问，还提供如下访问函数(头文件 `pdn_entity.h`，名称空间 `pdn` 内)：  

1. `get` 系列函数：对代理进行处理的 `std::get` 的封装，不支持 `refer` 和 `const_refer` 版本；  
2. `get_if` 系列函数：获得相应类型的值或得到 nullptr；  
3. `get_opt` 系列函数：仅对基本类型提供(整数、浮点数、布尔和字符)，获得相应类型的 `optional`，失败时得到 `nullopt`；  
4. `as` 系列函数： 将实体转换为相应类型的值，如果无法转换，将获得一个零初始化或默认初始化的目标类型的值；  
5. `type_test` 系列函数：测试实体持有的值是否为指定类型。  

以上函数同时提供 `entity` `entity_ref` `entity_cref` 成员函数版本(后两者不含 `get` 函数，其提供的同名 `get` 函数用于取引用的实体的地址)。  

`entity` 还有如下成员方法：  

1. `ref` `cref`：获得 `refer` 或 `const_refer` 类，它们有 `operator[]` 和 `at` 方法，但查询失败时都返回空 `refer|const_refer`。  
2. `operator[string_view<char_t>]`：获得 Object 的成员数据，如果当前实体不是 Object 或没有相应成员数据，将抛出异常；  
3. `operator[index]`：获得 List 的元素数据，如果当前实体不是 List 将抛出异常，且无范围检查；  
4. `at(string_view<char_t>)`：获得 Object 的成员数据的 `refer|const_refer`，如果当前实体不是 Object 或没有相应成员数据，返回空 `refer|const_refer`；  
5. `at(index)`：获得 List 的元素数据的 `refer|const_refer`，如果当前实体不是 List 或 out of range，返回空 `refer|const_refer`；  

`refer` 和 `const_refer` 类提供不抛出的 `operator[]`，当查询失败时，返回一个空 `refer|const_refer`，对空 `refer|const_refer` 的任何成员查询操作都只能得到空 `refer|const_refer`，获取值时只能获得 `nullptr`、`nullopt` 或是无效的值。  
`refer` 和 `const_refer` 类不支持 `pdn::get`，也不支持 `std::get` `std::get_if` `std::visit` 等方法。  

示例：  

```C++
#include <iostream>
#include <string>
#include <cassert>

#include "spdn.h"

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
    const auto result = parse(src, utf8_tag);
    assert(result);
    const auto& e = *result;
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
```
