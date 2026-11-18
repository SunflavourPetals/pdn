# 应用程序接口

本文只介绍库对使用者的公开接口，共三组：**解析**（`pdn::parse` 与 `pdn::parse_result`）、**实体**（`pdn::entity`）、**序列化**（`pdn::serializer` 与 `pdn::make_*serializer`），也就是"把文本读成实体、访问实体、再把实体写回文本"这条路径。

自定义函数包、`parser`、`lexer`、DFA 与各编解码器属于实现层，用来替换默认行为或嵌入自己的错误处理，不在本文范围内；需要时见《实现说明》。

文中示例都是 C++20、UTF-8、`#include <pdn/spdn.h>`。

## 快速开始

```cpp
#include <pdn/spdn.h>
#include <cassert>
#include <string_view>

int main()
{
    using namespace std::string_view_literals;
    // 1) 解析：文本 -> 实体
    auto result = pdn::parse(u8R"(name: "Petals"; count: 3)"sv, pdn::utf8_tag);
    assert(result.has_value());

    // 2) 访问：按名字取成员
    auto& dom = result.value(); // pdn::entity<pdn::unicode::u8char_t>&
    assert(dom.type_of() == pdn::type_code::object);
    assert(dom[u8"name"].as_string() == u8"Petals");
    assert(dom[u8"count"].as_int() == 3);

    // 3) 序列化：实体 -> 文本
    auto text = pdn::make_u8serializer().serialize(dom.as_object());
    assert(!text.empty());

    // 4) 再解析一次，确认往返
    auto again = pdn::parse(text, pdn::utf8_tag);
    assert(again.has_value());
    assert(*result == *again);
}
```

## 通用约定

### 头文件、命名空间与标准

一个头文件即可带齐三组接口：

```cpp
#include <pdn/spdn.h>
```

如果只用到其中一组，也可以直接 include `pdn_entity.h`、`pdn_parse.h` 或 `pdn_serializer.h`（实现头都在 `pdn/include/pdn/`），编译更快。

所有名字都在 `pdn` 命名空间；类型别名在 `pdn::type`；编码与码点工具在 `pdn::unicode`。库是 header-only 的，需要 C++20。

### 字符宽度与 tag

解析和序列化都按"码元"工作，模板参数 `char_t` 决定宽度：

| 宽度 | 码元类型 | 字面量 |
| :---: | :---: | :---: |
| UTF-8 | `pdn::unicode::u8char_t`（即 `char8_t`） | `u8"..."` |
| UTF-16 | `pdn::unicode::u16char_t` | `u"..."` |
| UTF-32 | `pdn::unicode::u32char_t` | `U"..."` |

使用 `parse` 时必须给出 tag（或显式写模板实参）：

```cpp
auto r1 = pdn::parse(::std::u8string_view{ u8"x: 1" }, pdn::utf8_tag);   // parse_result<char8_t>
auto r2 = pdn::parse(::std::u16string_view{ u"x: 1" }, pdn::utf16_tag);  // parse_result<char16_t>
auto r3 = pdn::parse<char32_t>(::std::u32string_view{ U"x: 1" });        // 显式模板实参亦可
```

三个 tag 是 `pdn::utf8_tag`、`pdn::utf16_tag`、`pdn::utf32_tag`：空对象，只用来传类型，不携带数据。

解析结果的字符宽度与 `char_t` 一致：`pdn::parse_result<char8_t>::value()` 的类型是 `pdn::entity<char8_t>`。

### 错误模型

解析默认不抛异常：`parse` 返回 `pdn::parse_result<char_t>`，失败时用 `error_code()` / `failed()` 查原因，`what()` 给出粗粒度名字。

具体的诊断文本（行列位置、期望与实际）由错误消息生成器负责，默认包把它输出到流；要收集这些文本需要换自定义函数包（见《实现说明》）。

`value()`、`operator*`、`operator->` 在失败结果上是**前置条件违反**：debug 构建会 `assert`，`NDEBUG` 下返回默认实体（i32 0），不要依赖它。

## 解析

解析函数的常用重载有 Unicode 码元字符串 / 字符串视图版本和文件名字符串 / filesystem::path 版本。  

### 重载一览

默认函数包的 `parse` 重载都返回 `parse_result<char_t>`：

| 输入源 | 调用形式 | 说明 |
| :---: | :---: | :---: |
| 字符串或视图 | `parse(sv, tag)` | `sv` 是 `u8/u16/u32` 的 `string` 或 `string_view`；C 数组会被拒绝（无法从类型上区分字符数组与零结尾字符串，无法区分结尾空码元是否算正文） |
| 码元迭代器 | `parse(begin, end, tag)` | 迭代器解引用得到对应宽度的码元 |
| token 迭代器 | `parse(begin, end, tag)` | 高级用法，需要自行构造 token 迭代器（例如先建 `lexer`） |
| `std::istream&` | `parse(is, encode_type, tag, buffer_size)` | 按给定编码读取，不剥 BOM |
| `std::ifstream&` | `parse(ifs, tag, buffer_size)` | 自动识别 BOM；流未打开 → `open_file_failed` |
| 文件名 | `parse(path, tag)` | `std::string` / `const char*` / `std::wstring` / `const wchar_t*` / `std::filesystem::path` 五种形式(宽字符串重载在不支持使用宽字符串打开文件的环境不可用) |

补充说明：

- `buffer_size` 是读文件的缓冲大小，默认 `pdn::detail::default_swap_chain_buffer_size`，一般不用传。
- 文件名重载会把文件名记进诊断前缀，例如 `config.spdn: syntax error(1:5) ...`。
- `encode_type` 的取值是 `pdn::unicode::encode_type::utf8`、`utf16_le`、`utf16_be`、`utf32_le`、`utf32_be`。
- 每组重载都还有一个接受自定义函数包的重载，本文不展开。

### 输入源与编码

- 字符串视图与迭代器：编码由码元类型决定（`u8string` 即 UTF-8），调用者保证码元序列合法、完整。
- `std::istream`：编码由 `encode_type` 指定。该重载**不剥 BOM**——带 BOM 的流必须先自行处理，因为 BOM 会被当成正文的一个码点，之后所有行列都会整体偏移。
- `std::ifstream` 与文件名：读开头几个字节识别 BOM（UTF-8 / UTF-16 小端大端 / UTF-32 小端大端），没有 BOM 时按 UTF-8 处理。

### 解析上限

- **嵌套深度**默认 256 层，超过后解析停止并返回 `nesting_depth_exceed`；该情况只通过返回码报告，不产生诊断文本。
- **错误数量**默认上限 100 条，达到后停止并返回 `parse_error_limit`。
- 空输入或只有空白的输入是合法的，返回成功的空对象（`object` 大小 0）。

### parse_result

`pdn::parse_result<char_t>` 是"解析结果 + 停止原因"：

| 成员 | 说明 |
| :---: | :---: |
| `error_code()` | `pdn::parse_error_code`；成功时为零值（0 不是枚举项） |
| `success()` / `failed()` | 是否成功 / 失败 |
| `has_value()` / `operator bool` | 与 `success()` 相同 |
| `value()` | 解析出的 `entity<char_t>` 的引用（`&` 与 `const&`），失败时是前置条件违反 |
| `operator*` / `operator->` | 同 `value()`，按值类别提供重载 |
| `what()` | 停止原因的粗粒度 ASCII C 字符串，见下表 |

### 错误码

| 错误码 | `what()` | 含义 |
| :---: | :---: | :---: |
| `parse_error_code{}`（零值） | `"success"` | 解析成功 |
| `parse_error` | `"parse_error"` | 词法或语法错误，或 Unicode 解码错误；具体文本在诊断里 |
| `parse_error_limit` | `"parse_error_limit"` | 错误数量达到上限（默认 100） |
| `nesting_depth_exceed` | `"nesting_depth_exceed"` | 嵌套超过上限（默认 256） |
| `open_file_failed` | `"open_file_failed"` | 文件打不开 |
| `bad_stream` | `"bad_stream"` | 流处于 `bad()` 状态 |
| `stream_operation_failed` | `"stream_operation_failed"` | 流操作失败 |
| `incomplete_code_unit` | `"incomplete_code_unit"` | 输入在码元中间结束（如半个 UTF-8 序列） |
| `user_cancelled` | `"user_cancelled"` | 因其他原因的用户请求取消(非定制不会使用此代码) |

### 解析示例

```cpp
auto ok = pdn::parse(::std::u8string_view{ u8"a: 1; b: [2, 3]" }, pdn::utf8_tag);
assert(ok.success());
assert(ok.value().as_object().size() == 2);

auto bad = pdn::parse(::std::u8string_view{ u8"a: [1, 2" }, pdn::utf8_tag);
assert(bad.failed());
assert(bad.error_code() == pdn::parse_error_code::parse_error);

// 文件名（相对/绝对路径都可以；扩展名随意）
auto from_file = pdn::parse(::std::string{ "config.spdn" }, pdn::utf8_tag);

// 已经打开的 ifstream：自动识别 BOM
::std::ifstream ifs{ "config.spdn", ::std::ios::binary };
auto from_ifs = pdn::parse(ifs, pdn::utf8_tag);

// istream：必须显式指定源编码
::std::istringstream iss{ "a: 1" };
auto from_istream = pdn::parse(iss, pdn::unicode::encode_type::utf8, pdn::utf8_tag);

// 码元迭代器
::std::u8string src = u8"a: 1";
auto from_iter = pdn::parse(src.cbegin(), src.cend(), pdn::utf8_tag);
```

## 实体

`pdn::entity<char_t>` 是 PDN 值的内存表示：一个能装下整数、浮点、布尔、字符、字符串、列表、对象的变体。它同时支持按名字/下标访问成员，可以直接当容器用。

### 值的种类

| PDN 类型 | C++ 类型（`type::` 下） | 说明 |
| :---: | :---: | :---: |
| `i8`…`i64` | `i8` `i16` `i32` `i64` | 定宽有符号整数 |
| `u8`…`u64` | `u8` `u16` `u32` `u64` | 定宽无符号整数 |
| `f32` / `f64` | `f32` `f64` | 浮点 |
| `boolean` | `boolean` | `bool` |
| `character` | `character<char_t>` | 单个字符（可存多个码元，见下） |
| `string` | `string<char_t>` | `std::basic_string` |
| `list` | `list<char_t>` | `std::vector<entity<char_t>>` |
| `object` | `object<char_t>` | `std::unordered_map`；定义 `PDN_ENABLE_ORDERED_MAP` 时换成保序容器 |

“实体类型的名字”是 `pdn::type_code` 的枚举值：`i8`、`i16`、`i32`、`i64`、`u8`、`u16`、`u32`、`u64`、`f32`、`f64`、`boolean`、`character`、`string`、`list`、`object`，外加不属于 PDN 类型的 `unknown`（空引用）。

值初始化的实体持有 `i32` 的 0。

### 构造

标量直接构造，容器值要先过 `make_proxy`：

```cpp
using char_t = pdn::unicode::u8char_t;

pdn::entity<char_t> n{ pdn::type::i32{ 5 } };
pdn::entity<char_t> f{ pdn::type::f64{ 1.5 } };
pdn::entity<char_t> b{ pdn::type::boolean{ true } };
pdn::entity<char_t> c{ pdn::type::character<char_t>{ u8"a", 1 } };  // (首码元, 码元数)

// 字符串、列表、对象在变体里存的是 proxy：托管所有权、拷贝即深拷贝
pdn::entity<char_t> s{ pdn::make_proxy<pdn::type::string<char_t>>(pdn::type::u8string{ u8"hi" }) };
pdn::entity<char_t> l{ pdn::make_proxy<pdn::type::list<char_t>>(pdn::type::list<char_t>{
    pdn::entity<char_t>{ pdn::type::i32{ 1 } },
    pdn::entity<char_t>{ pdn::type::i32{ 2 } } }) };

pdn::type::object<char_t> o;
o[u8"x"] = pdn::entity<char_t>{ pdn::type::i32{ 1 } };
pdn::entity<char_t> obj{ pdn::make_proxy<pdn::type::object<char_t>>(::std::move(o)) };
```

`type::character<char_t>` 的构造参数是“第一个码元 + 码元个数”，`u8"a"` 加 1 表示一个 UTF-8 码元；它另有 `data()`、`size()`、`to_string_view()`。

### 类型判别

```cpp
pdn::entity<pdn::unicode::u8char_t> e{ pdn::type::i32{ 42 } };

auto code   = e.type_of();                                // pdn::type_code::i32
auto is_i32 = e.type_test<pdn::type::i32>();              // 是否恰好持有 i32
auto is_int = e.type_test_any<pdn::type::i32, pdn::type::i64>();
assert(code == pdn::type_code::i32 && is_i32 && is_int);
```

`type_test<T>()` 对容器类型会解开 proxy，所以持有 `proxy<string>` 的实体满足 `type_test<type::string<char_t>>()`。

### 取值与转换

| 访问器 | 返回 |
| :---: | :---: |
| `as_int()` / `as_int(tag)` | `type::i64` / tag 指定的整数类型 |
| `as_uint()` / `as_uint(tag)` | `type::u64` / tag 指定的无符号类型 |
| `as_fp()` / `as_fp(tag)` | `type::f64` / tag 指定的浮点类型 |
| `as_bool()` | `type::boolean` |
| `as_char()` | `type::character<char_t>` |
| `as_u8char()` / `as_u16char()` / `as_u32char()` | 对应编码的 `character` |
| `as_string()` | `const type::string<char_t>&` |
| `as_u8string()` / `as_u16string()` / `as_u32string()` | 对应编码的 `string` |
| `as_list()` | `const type::list<char_t>&` |
| `as_object()` | `const type::object<char_t>&` |
| `as<T>()` | 统一入口，`T` 写目标类型名，按 T 转发到上面的转换 |
| `as(tag)` | 统一入口，按 tag 转发；容器与字符串按引用返回 |

`as` 家族有两种指定目标类型的方式，约定不同：

- **模板实参形式**（`as<T>(e)`、`e.as<T>()`）：`T` 写**目标类型名**，例如 `e.as<pdn::type::i8>()`、`pdn::as<pdn::type::u8string>(e)`、`e.as<pdn::type::list<char_t>>()`。tag 的**类型**本身也满足约束（写无 cv 的 `pdn::detail::*_tag_t` 才行；`decltype(pdn::i8_tag)` 带 `const`，会编译失败），所以模板实参一律写类型名。
- **实参形式**（`as(e, tag)`、`e.as(tag)`，以及 `as_int`/`as_uint`/`as_fp` 的第二个参数）：写 **tag**，例如 `e.as(pdn::i8_tag)`、`pdn::as(e, pdn::list_tag)`。不要写 `as(e, pdn::type::i8{})` 这种“传目标类型对象”的形式——小类型虽然能编译，但它会为参数构造一个临时对象，对 string/list/object 这类目标尤其不合适；统一用 tag 也更好读。

tag 都在 `pdn::type_tag` 命名空间里（它是 inline 命名空间，直接写 `pdn::i8_tag` 即可）：`i8_tag`/`i16_tag`/`i32_tag`/`i64_tag`、`u8_tag`/`u16_tag`/`u32_tag`/`u64_tag`、`f32_tag`/`f64_tag`、`bool_tag`、`char_tag`/`u8char_tag`/`u16char_tag`/`u32char_tag`、`string_tag`/`u8string_tag`/`u16string_tag`/`u32string_tag`、`list_tag`、`object_tag`；另有两个别名 `int_tag`（同 `auto_int_tag`）与 `uint_tag`（同 `auto_uint_tag`）。tag 到具体类型的映射由 `pdn::type_tag::tag_to_type_t<tag_t, char_t>` 给出：`auto_int_tag` → `type::auto_int`，其余 `xxx_tag` → `type::xxx`（字符与容器 tag 的映射还带 `char_t` 参数）。

```cpp
pdn::entity<pdn::unicode::u8char_t> e{ pdn::type::i32{ 42 } };

// 模板实参：写类型名
assert(e.as<pdn::type::i8>() == 42);
assert(pdn::as<pdn::type::u16>(e) == 42u);

// 实参：写 tag
assert(e.as_int(pdn::i8_tag) == 42);     // 有符号 8 位
assert(e.as_uint(pdn::u16_tag) == 42u);  // 无符号 16 位
assert(e.as_fp(pdn::f32_tag) == 42.0f);  // float

pdn::entity<pdn::unicode::u8char_t> s{ pdn::make_proxy<pdn::type::string<pdn::unicode::u8char_t>>(
    pdn::type::u8string{ u8"hi" }) };
assert(s.as(pdn::u8string_tag) == u8"hi");         // as(tag)
assert(pdn::as<pdn::type::u8string>(s) == u8"hi"); // as<T>
```

转换规则（`as_int` / `as_uint` / `as_fp` / `as_bool` 之间）：

- 布尔、整数、浮点可以互转：非零为 `true`，`true` 为 1，浮点取整时截断。
- 超出目标类型范围时**钳到目标类型的 min/max**，不抛异常、不产生未定义行为。
- `NaN` 转整数得 0，转布尔得 `true`。
- 字符、字符串、列表、对象**不参与数值转换**：对它们调 `as_int()` 得 0，`as_bool()` 得 `false`。

严格取值用 `get` 家族（不做转换，类型必须匹配，自动剥离代理）：

```cpp
pdn::entity<pdn::unicode::u8char_t> e{ pdn::type::i32{ 42 } };

auto& i = e.get<pdn::type::i32>();      // 引用；类型不符是前置条件违反
auto* p = e.get_if<pdn::type::i32>();   // 指针，不匹配得 nullptr
auto  o = e.get_opt<pdn::type::i32>();  // std::optional
assert(&i == p);
assert(o.has_value() && *o == 42);
```

### 容器访问

| 形式 | 返回 | 行为 |
| :---: | :---: | :---: |
| `e.at(i)` / `e.at(key)` | `refer` / `const_refer` | 未命中、越界或类型不符时返回**空**引用，不抛异常 |
| `e[i]` / `e[key]` | `entity&` / `const entity&` | 下标不做边界检查；键不存在时抛 `std::out_of_range` |
| `e.ref()` / `e.cref()` | `refer` / `const_refer` | 取得引用对象，可用于传递“可能为空”的成员 |

`refer` 与 `const_refer` 提供 `has_value()`、`operator bool`、`as_*`、`at` 与 `operator[]`，空引用上的 `as_*` 一律返回零值（0 / false / 空容器）。

```cpp
pdn::entity<pdn::unicode::u8char_t> e{ pdn::make_proxy<pdn::type::object<pdn::unicode::u8char_t>>(
    pdn::type::object<pdn::unicode::u8char_t>{}) };

auto maybe = e.at(u8"missing");   // key_type 是 std::basic_string_view<char_t>，可以直接给字面量
assert(!maybe.has_value());
assert(maybe.as_int() == 0);
```

### 比较

`operator==` 是值比较（C++20 下 `!=` 自动由它重写）：

| 形式 | 语义 |
| :---: | :---: |
| `entity == entity` | 变体的活跃类型相同且值相等才相等；`i32{ 1 }` 与 `i64{ 1 }` 不相等。字符串、列表、对象比较 proxy 指向的值，对象不关心插入顺序 |
| `const_refer == const_refer` | 两个空引用相等；空与非空不等；都非空则比较指向的实体 |
| `entity == const_refer` | 引用为空则不等，否则按值比较。`refer` 可隐式转成 `const_refer`，所以 `entity == refer` 同样可用；写成 `cref == entity` 时由 C++20 的重写规则匹配到这条 |

```cpp
pdn::entity<pdn::unicode::u8char_t> a{ pdn::type::i32{ 1 } };
pdn::entity<pdn::unicode::u8char_t> b{ pdn::type::i32{ 1 } };
pdn::entity<pdn::unicode::u8char_t> c{ pdn::type::i64{ 1 } };

assert(a == b);
assert(a != c);                                           // 活跃类型不同
assert(a == a.cref());
assert(a != pdn::const_refer<pdn::unicode::u8char_t>{});  // 与空引用不相等
```

### 自由函数

`pdn` 命名空间里为实体和引用各提供一套同名的自由函数，效果与成员函数相同：

```cpp
pdn::entity<pdn::unicode::u8char_t> e{ pdn::type::i32{ 42 } };

assert(pdn::type_of(e) == pdn::type_code::i32);
assert(pdn::type_test<pdn::type::i32>(e));
assert(pdn::as_int(e) == 42);
assert(pdn::as_int(e, pdn::i8_tag) == 42);   // 带目标类型（这里指定 i8）
assert(pdn::as_uint(e) == 42u);
assert(pdn::as_fp(e) == 42.0);
assert(pdn::as_bool(e));
assert(pdn::get<pdn::type::i32>(e) == 42);
assert(pdn::get_if<pdn::type::i32>(e) != nullptr);
assert(pdn::get_opt<pdn::type::i32>(e).has_value());
```

### 实体示例

```cpp
auto r = pdn::parse(::std::u8string_view{ u8"num: 42; text: \"hi\"; list: [1, 2]" }, pdn::utf8_tag);
assert(r.success());
auto& dom = r.value();

// 类型判别
assert(dom[u8"num"].type_of() == pdn::type_code::i32);
assert(dom[u8"num"].type_test<pdn::type::i32>());

// 转换
assert(dom[u8"num"].as_int() == 42);
assert(dom[u8"num"].as_uint() == 42u);
assert(dom[u8"num"].as_fp() == 42.0);
assert(dom[u8"num"].as_bool());

// 容器
auto& list = dom[u8"list"].as_list();
assert(list.size() == 2);
assert(list[1].as_int() == 2);

// 空引用
assert(!dom.at(u8"nope").has_value());
```

## 序列化

### serializer

`pdn::serializer<char_t>` 把 `type::object<char_t>` 写成 spdn 文本。排版完全由六个公有字符串成员控制，可以直接改，也可以由 `make_serializer` 从枚举生成：

| 成员 | 默认值 | 作用 |
| :---: | :---: | :---: |
| `tab` | `"\t"` | 每层缩进 |
| `last_sep` | `";"` | 成员/值的结尾符 |
| `separator` | `": "` | 名字与值之间 |
| `separator_for_ts` | `": "` | 名字与“类型 + 值”之间 |
| `list_separator` | `" "` | 名字与 `[` 之间 |
| `object_separator` | `" "` | 名字与 `{` 之间 |

### serialize

签名为 `serialize(const type::object<src_char_t>& dom, bool use_line_feed = true, bool use_last_sep_for_lo = true) const -> string_t`。用法：

```cpp
pdn::type::object<pdn::unicode::u8char_t> o;
o[u8"x"] = pdn::entity<pdn::unicode::u8char_t>{ pdn::type::i32{ 1 } };

pdn::serializer<pdn::unicode::u8char_t> ser{};
auto text    = ser.serialize(o);               // 默认：换行、缩进、分号
auto flat    = ser.serialize(o, false);        // 单行
auto no_last = ser.serialize(o, true, false);  // 命名容器值不带结尾分号
```

- `dom` 为空时返回空串（写出来的就是空文件）。
- 非空时每个成员都以 `last_sep` 结尾，所以结果总以 `last_sep` 结束。
- `use_line_feed = true`：成员一行一个，按层缩进；`false`：全部排成一行，成员之间用空格。
- `use_last_sep_for_lo`：带名字的列表/对象值（list/object）末尾是否也补 `last_sep`。
- 列表元素之间用 `,`，并且**总是写尾逗号**（PDN 允许）；对象成员之间用分号隔开。
- 名字不是合法 PDN 标识符时，会自动加反引号并转义。

成员顺序取决于底层容器：默认的 `unordered_map` 不保证顺序（写回文本后顺序可能变化）；定义 `PDN_ENABLE_ORDERED_MAP` 后保持插入顺序。

### 类型标注

PDN 的写法允许省略类型，序列化器按下面的规则决定写不写类型名：

| 值 | 写出示例 | 规则 |
| :---: | :---: | :---: |
| `i32` | `5` | 默认整型（`auto_int`），省略类型 |
| `i32` 最小值 | `i32 -2147483648` | 裸写会被解析成 `i64`，必须带类型 |
| `i64` | `i64 5` | 非默认宽度，写类型 |
| `i64` 最小值 | `@i64_min` | 没有对应的字面量写法 |
| `i8` / `i16` / `u8`…`u64` | `i8 -5` | 写类型 |
| `f32` | `f32 1.5` | 写类型，以区分 `f32` 与 `f64` |
| `f64` | `1.5` | 默认浮点类型，省略类型 |
| `boolean` | `@true` / `@false` | 无字面量，用 at 常量 |
| `character` | `'a'` | |
| `string` | `"hi"` | |
| `list` | `[` 换行 `\t1,` 换行 `\t2,` 换行 `];` | 元素需要类型时写成 `type: value` |
| `object` | `{` 换行 `\t...` 换行 `}` | |

浮点按最短往返形式输出（没有 `.`/`e` 时补 `.0`）；部分与at常量相等的值写成 `@pi` 这类常量名，`inf`/`NaN` 写成 `@inf` / `-@inf` / `@NaN`。字符串与字符按 PDN 规则转义；`character` 若解码失败则写成替换字符 U+FFFD。

### 枚举与工厂

| 枚举 | 取值 |
| :---: | :---: |
| `serialize_tab` | `tab`（`"\t"`）、`space_1`…`space_8`、`no_tab` |
| `serialize_last_semi` | `semi_yes`（`";"`）、`no_semi` |
| `serialize_separator` | `sep_semi_with_space`（`": "`）、`sep_semi`（`":"`）、`sep_space_around_semi`（`" : "`）、`sep_space`（`" "`） |
| `serialize_separator_ts` | `sts_semi_with_space`、`sts_semi`、`sts_space_around_semi` |

```cpp
// 三个工厂的签名相同：tab, last, sep, sep_ts, list_sep, obj_sep，全部有默认值
auto styled = pdn::make_serializer<char8_t>(
    pdn::serialize_tab::space_2,           // 缩进用两个空格
    pdn::serialize_last_semi::no_semi,     // 不要结尾分号
    pdn::serialize_separator::sep_semi,    // 名字与值之间用 ":"
    pdn::serialize_separator_ts::sts_semi  // 名字与 "类型 + 值" 之间用 ":"
);

auto u8ser = pdn::make_u8serializer(pdn::serialize_tab::space_4);  // 只改缩进，其余默认
```

工厂的默认实参全部是各枚举的 `{}`：缩进是制表符、结尾是分号、分隔符都是 `": "`（列表/对象名字与容器之间也是 `": "`）。直接默认构造 `serializer<char_t>{}` 时，`list_separator` 和 `object_separator` 是空格。

### 序列化示例

```cpp
using char_t = pdn::unicode::u8char_t;
using obj_t  = pdn::type::object<char_t>;

obj_t o;
o[u8"count"] = pdn::entity<char_t>{ pdn::type::i32{ 3 } };
o[u8"name"]  = pdn::entity<char_t>{ pdn::make_proxy<pdn::type::string<char_t>>(pdn::type::u8string{ u8"Petals" }) };
o[u8"list"]  = pdn::entity<char_t>{ pdn::make_proxy<pdn::type::list<char_t>>(pdn::type::list<char_t>{
    pdn::entity<char_t>{ pdn::type::i32{ 1 } },
    pdn::entity<char_t>{ pdn::type::i32{ 2 } } }) };

// 默认排版：一行一个成员、制表符缩进、分号结尾
auto text = pdn::make_u8serializer().serialize(o);

// 单行排版
auto one_line = pdn::make_u8serializer().serialize(o, false);

// 自定义风格：两个空格缩进、不要结尾分号、冒号两侧不留空格
auto styled = pdn::make_serializer<char8_t>(
    pdn::serialize_tab::space_2,
    pdn::serialize_last_semi::no_semi,
    pdn::serialize_separator::sep_semi,
    pdn::serialize_separator_ts::sts_semi)
    .serialize(o);
```

## 编译期开关

| 宏 | 作用 |
| :---: | :---: |
| `PDN_ENABLE_ORDERED_MAP` | `type::object` 换成保持插入顺序的 `ordered_map`（默认 `std::unordered_map`） |
| `PDN_NO_EXTENSIONS` | 关闭非标准扩展：裸 `∞` 拼写、`cppint`/`cppuint`、六个非标准 at 常量 |
| `PDN_NO_EXCEPTIONS` `PDN_USER_TERMINATE` | 无异常环境：涉及越界/失败的 `throw` 改为调用 `std::terminate` 或 `PDN_USER_TERMINATE` |
| `PDN_USER_FORMAT` | 替代 `<format>` 的浮点格式化 |
| `PDN_USER_FROM_CHARS_FLOAT` | 替代浮点 `std::from_chars` |

这些宏都要求不同的翻译单元之间保持一致，否则可能违反 ODR。逐项说明见《PDN 及本库教程》中的相应章节。

## 相关文档

- [`pdn-reference.md`](pdn-reference.md)：`spdn` 的格式规范。
- [`pdn-guide.md`](pdn-guide.md)：从写第一个 `.spdn` 文件开始的教程，含数字、字符串、列表、对象、at 常量与宏定义。
- [`pdn-parser-impl.md`](pdn-parser-impl.md)：实现说明，函数包、`parser`、`lexer` 与错误处理。

## 关于本文件

本文件由 Deepseek v4.1flash 1017 按要求生成，库作者审查、改进与提交。  
