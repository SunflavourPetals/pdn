# PDN 及 本库教程

开始这个项目的契机是我希望有一个可用的数据交换格式，用于给我的以学习为目的编写的程序提供各种配置参数。我不满足于使用现有的 `ini` `xml` `json` 等格式，于是计划设计一个属于自己的数据交换格式，但主要还是用于学习。最开始设计出来的东西非常丑陋(主要是由于相关经验的欠缺)，后来我主要参照了 `C++` 的字面量词法和 `json` 的语法进行设计，于是有了现在的 `spdn` 格式。  

综上，`spdn` 文件的用途就是用来存储各式各样的信息。它的语法和 `json` 有点像，不过它们并不互相兼容。  
此外，`spdn` 提供了一些不太常用的功能，比如原始字符串、类型指定和 at 常量。当然，它们的使用场景有限，不知道这些也完全不影响用户使用。  

这个文档则致力于教学如何使用和编写 `spdn` 格式的文件，以及本库实现的解析器的使用方法。  

注意：这个解析器是使用 C++20 标准实现的，开始前请确认你的编译器是否支持 C++20 标准。  

## Hello world

现在我们来把 `Hello world!` 写进 `spdn` 文件里，并尝试在终端输出它！  

### 第一个 .spdn 文件

首先编写一个 `hello.spdn` 文件，文件内容如下：  

```spdn
say "Hello world!"
```

这个文件很简短，它包含一个键值对 `key="say"` `val="Hello world!"`，其中 `say` 只是我随意起的名字，在 `json` 中，与之等价的表达是：  

```json
{
    "say": "Hello world!"
}
```

`spdn` 省略了最外层的大括号，而且键可以以符合 C++ 标识符词法的方式提供。至于键和值之间的冒号，则完全取决于你的想法——你觉得加冒号好就加上冒号，觉得不好也可以不写，或是根据情况选择是否使用冒号，这完完全全是你的自由。  

```spdn
say: "Hello world!"
```

注意：`spdn` 文件的编码格式只可以是 `UTF-8` `UTF-16` `UTF-32` 其中之一，因为实现的解析器暂不支持其他编码。  

### 在 C++ 程序中访问 spdn 数据文件

现在我们尝试在 C++ 程序中获取 `hello.spdn` 中的内容：  

```C++
#include <iostream>

#include "pdn_parse.h" // 包含解析 spdn 的函数
#include "pdn_data_entity.h" // 包含 entity 的访问函数如 as_string

// ostream 对象如 std::cout 并不能输出 u8string 的值，
// 但是每次都进行转换会使程序变得冗长，不利于教学。
// 尽管下面这种方式过于简短粗暴，例如在 Windows 平台输出包含汉字等非 ASCII 字符的文本会在使用 936 代码页的 Win 系统上显示乱码。
// 尽管有进行转码的方法，但那不再本教程的讨论范围内，
// 如果你遇到了麻烦，可以先尝试使用 chcp 65001 命令，再运行程序，
// 等到合适的时机再学习更好的解决方案。
// 之后我会将这个函数以 inline 的方式放在头文件 outu8sv.h 中，以后就不再展示。
std::ostream& operator<<(std::ostream& o, std::u8string_view sv)
{
    o << std::string_view{ (const char*)sv.data(), sv.size() };
    return o;
}

int main()
{
    using namespace pdn;

    // 使用 pdn::parse 解析 hello.spdn 中的数据
    // pdn::utf_8_tag 指示以 utf-8 作为解析后的键和字符、字符串的编码方式，表现为用 C++ 的 u8string 类型进行描述
    // 该指示与被解析的文件采用什么编码无关，它只作用于解析结果
    auto entity_opt = parse("hello.spdn", utf_8_tag);
    
    // 得到一个 entity_opt，它是一个 std::optional<pdn::u8entity> 的对象
    if (!entity_opt) // 判断是否解析成功，若文件不存在或无法打开，则是 nullopt
    {
        std::cerr << "failed in parse hello.spdn\n";
        return 0;
    }
    
    // 从 optional 获取数据实体的引用，
    // 这个数据实体包含了 hello.spdn 中的所有数据作为它的成员(注释等不算做数据)，
    // 因此它是一个 spdn 中的 object，在 C++ 里的类型则是一个关联容器。
    const auto& entity = *entity_opt;

    // 从中查询 "say" 成员，并引用它
    const auto& say = entity[u8"say"];

    // 将我们查询到的数据以字符串的形式使用，需要用到 pdn::as_string
    std::cout << as_string(say) << "\n";
}

```

完成代码的编写，现在可以试着编译并运行它。  

本仓库提供的 PDN 解析器和其他工具都是使用 C++20 标准进行编写的，所以你的编译器必须支持其所用到的特性才行。另外，使用编译命令时需要正确指定包含目录，如果你熟悉 `CMAKE` `xmake` 或是其他构建工具，那这些对你应该不成问题。  
如果不是，这里有一个使用 `g++` 进行编译的模板：  
`g++ -std=c++20 -I<包含目录> -o <输出文件> <源文件>`  

例如，移动到 `.../pdn/docs/guide-source` 目录下使用以下命令：  

```shell
g++ -std=c++20 -I../../pdn -o hello-spdn ./hello-spdn.cpp
```

编译之后运行它(`hello-spdn.exe`)，程序会输出 `Hello world!`。

### 关于使用 u8string 的疑问

为什么不用 `std::string` 而是 `std::u8string`？  

`spdn` 基于 Unicode，我想在类型上明确表达使用 `utf_8_tag` 的解析结果是用 UTF-8 编码的，但是也就不能直接用 `std::cout` 进行输出了。不知道这样是不是设计失误。  

如有必要，可以使用 [`c8rtomb`](https://zh.cppreference.com/w/cpp/string/multibyte/c8rtomb "cppreference") 或 boost 库等途径转换编码。  
此后的教学中就不再讨论这个问题了。  

## 一个贴近实际的例子

一个软件所需要的资源可能有很多种，比如一组数据、一些图片等。我们只需要表示数字和文本，其他资源如图片、音频等使用文件路径或者链接来引用。  

假设现在我们有一个图形界面程序，想要为他提供有关窗口大小、一组用于轮播展示的图片和图片的展示位置，并且希望这些内容可以更改而不必重新编译程序，我们就不能将数据直接写死在代码中了。  
像下面这样为它编写一个 `spdn` 文件：  

```spdn
window: {
    width: 640;
    height: 480;
};
pictures: {
    path: [
        "sky.png",
        "mountain.png",
    ];
    position: {
        left: 0.25; // 距离窗口最右侧 0.25 个 window.width 的距离
        top: 0.15; // 距离窗口最上端 0.15 个 window.height 的距离
    };
};
```

我们把信息分成了 `window` 和 `pictures` 两个部分，这样用花括号包围的数据，在 `spdn` 叫做 `object`，即对象，花括号内是更深一层的键值对序列。  
在 `pictures` 中，我们定义了一个列表 `path` 来存放一组路径，它的每个元素之间必须有逗号隔开，非空列表末尾元素后的逗号是可选的，我们还定义了一个 `position` 对象记录位置，我使用了行注释来解释 `left` 和 `top` 对应数据的含义。  

这个例子出现的所有冒号和分号都是可选的，对于这种每个键值对后都换行的文件，使用分号可能反而使它不美观，我写出来只是为了展示每个键值对后都可以使用分号隔开。  
我提供了一个我认为最美观的形式：  

```spdn
// ./guide-source/example.spdn
window {
    width: 640
    height: 480
}
pictures {
    path [
        "sky.png",
        "mountain.png",
    ]
    position {
        left: 0.25 // 距离窗口最右侧 0.25 个 window.width 的距离
        top: 0.15 // 距离窗口最上端 0.15 个 window.height 的距离
    }
}
```

虽然我们使用的列表的元素全部都是字符串，但是它实际上可以存放一组不同类型的数据，包括列表和对象: `list [[ 123, 456 ], { m: 789 }, 123, "string" ]`。  

学会这个例子，你就能熟练地编写 `spdn` 格式的文件了。  

## 数字

// todo

## 字符串

// todo

## 布尔数据

`spdn` 的布尔类型数据：  

```spdn
bool_1: boolean 1       // 非零值被转换为布尔值 true
bool_2: bool    123.456 // 非零值被转换为布尔值 true，但不太好，bool 和 boolean 等价
bool_3: bool    0       // 零被转换为布尔值 false
bool_4 @true  // 布尔值 true
bool_5 @false // 布尔值 false
```

由于 `spdn` 无关键字的设计，`true` 和 `false` 本身是合法的标识符，如果想获得布尔值，要么像 `bool_1` `bool_2` `bool_3` 一样，通过类型指定来进行类型转换，要么就使用 at 常量，它的语法是 `@` 符号后紧跟一个普通的标识符，如 `@true` `@false`。  
有关 at 常量，后面会做详细教学。  

// todo at 常量链接

## 标识符的三种表现形式

我们提到 `spdn` 参照了 `json` 的语法，可能你已经注意到 `json` 的键是用字符串表示的，而我们之前一直都在用“标识符”的词法来表示键。  
我相信大部分的数据，不需要超过“标识符”表示范围的名字，我不喜欢在给它们命名时用一些特殊的字符，也不喜欢每次在表示一个键时，都要用字符串的形式。尽管如此，我也没有禁止字符串形式的键的表示方法，相反，我提供“字符串标识符”的同时还额外提供“原始字符串标识符”，作为键的表示方法。  
当我们想要在键中使用特殊字符比如空格时，只能通过字符串标识符或原始字符串标识符的方式达成：  

```spdn
a_name_without_spaces 123 // OK
a name with spaces 123 // "a" "name" "with" "spaces" "123" 不符合语法
```

字符串标识符的词法是反引号 `` ` `` 包围的内容，不能包含换行符(LF)，且支持转义字符：  

```spdn
`a name with spaces` 123 // OK

`a name with
linefeed` 123
// 通常情况下我们使用 LF 或是 CRLF 作为换行符，
// 标识符字符串内包含 LF 将导致错误
// 例如解析器很有可能认为 `a name with 是一个忘记写结尾反引号的标识符字符串
// 接着又认为 linefeed 是另一个普通的标识符
// 然后又把 ` 123 当作了又一个忘记写结尾反引号的标识符

`abcd\n\`\0` 123 // OK
// 使用转义字符 \n 表示换行(LF)
// spdn 的标识符字符串独有 \` 转义，表示一个反引号
// 使用转义字符 \0 表示 NUL 字符

`\J` 123 // 错误，没有 \J 这个转义序列
// 后果至少是反斜线被丢弃，键为 `J`
// 请不要反这种错误
```

除了 `` \` `` 是 `spdn` 字符串标识符独有的转义序列外，`spdn` 支持 C++ 除了条件转义序列(实现定义)的所有转义序列，与目前最新的 C++ 标准(C++26)完全一样，参考 [`cppreference`](https://zh.cppreference.com/w/cpp/language/escape "escape")。  

“原始字符串标识符”则是更没有用武之地的特性，我们之后再作讲解，先将精力放在更重要、更有用的地方。  

// todo 超链接 原始字符串标识符

## 类型和类型指定

### 类型指定的语法

在[一个贴近实际的例子](#一个贴近实际的例子)里，我们使用了数字、字符串、列表、和成组的键值对(即对象：`window` `pictures` `position`)。  
在 `spdn` 中，数据都有自己的类型，类型决定了数据的形式和取值范围。最常用的，莫过于数字、字符串、列表和对象。  
我们可以为实体指定类型：`实体名称 : 类型 数据的表示`，如 `num: int 123`，为列表的元素指定类型使用 `类型 : 数据的表示`，如 `list [int: 123, float: 1.0]`。  
警告，如果没有必要，不要为实体指定类型！

```spdn
an_i8_number: i8 127 // 还行，我想明确表示数据的类型是 i8

a_regular_number: int 123 // 不好，这个类型指定完全是多余的！

a_number_less_than_127: i8 9
// 不好，取值范围的约束不应该由 spdn 来做
// 这个实体还因为被指定为 i8 类型而不能表示小于 -128 的整数

object: object { num: 123 }
// 不好！{ num: 123 } 不是对象还能是什么？这个类型指定完全是多余的！

// 当我们要表达“约束这个数据的取值范围”时，应该使用注释向他人说明，并在解析完 spdn 文件后，检查这些数据：
score 100 // 分数的范围为 0 到 100 分
```

一定不要滥用类型指定，它会让文档变得不自然和冗长，我们更应该关注数据本身，而不是它们的类型。  

类型指定可能导致类型转换，转换规则在下一节讲解。  

### 类型转换规则

// todo

### 类型系统详解

`spdn` 有十五种类型：`i8` `i16` `i32` `i64` `u8` `u16` `u32` `u64` `f32` `f64` `boolean` `character` `string` `list` `object`。  

#### 整数和浮点数类型

可以看到像某些编程语言一样冗杂的有关数字的类型，它们很可能是一种设计失误。设计它们，是为了能更好的对应 C++ 的整数和浮点数类型，通常这些整数类型是 C++ 标准库 `cstdint` 中 `int8_t` 到 `int64_t`、`uint8_t` 到 `uint64_t`。并且，给用户选择的权力，如指定一个实体是 `f32` 类型的：`num:f32 1.0`，如果不使用类型指定，那么 `num` 的类型就是 `f64`，而不是 `f32`。  
  
`spdn` 中有类型别名 `int` 和 `uint`，在 PC 上它们基本是 `i32` 和 `u32` 的别名，如果对 `int` `uint` 别名的具体类型感兴趣，可以参阅 [`pdn_reference`](./pdn-reference.md#数据类型)。  
而 `f32` 和 `f64` 就是 C++ 的 `float` 和 `double`，在 `spdn` 中 `float` 是 `f32` 的别名，`double` 是 `f64` 的别名。  

我们在 `spdn` 中写下的整数，绝大部分是 `int` 类型的，如果我们写下的数字超过了 `int` 类型能表示的范围，解析器会为它选择一个合适的整数类型，如果没有能表示这个整数的类型，那么就会产生错误：`1` 的类型是 `int`，`0xffff'ffff'ffff'ffff` 的类型是 `u64`，`0x1'0000'0000'0000'0000` Oops! 现在连 `u64` 都无法表示这个整数，只能报告错误了！  
其实像这样的情况应该换用字符串(C++的内置整数类型无法满足要求，由程序想办法解析这么大的数)或者浮点数表示了，`0x1p64`，这里使用的是十六进制科学计数法表示。还记得我们说过 `spdn` 参考了 C++ 的字面量词法吗？`spdn` 支持[二进制、八进制、十进制和十六进制的整数](https://zh.cppreference.com/w/cpp/language/integer_literal "cppreference")，以及[十进制、十进制科学计数法、十六进制科学计数法的浮点数](https://zh.cppreference.com/w/cpp/language/floating_literal "cppreference")。  

在 `spdn` 里浮点数形式的的数据都是 `f64` 类型的，想要得到 `f32` 类型的数据只能使用类型指定来转换，将整数转换为 `f32` 浮点数：`num1:f32 123`，将 `f64` 浮点数转换为 `f32` 浮点数：`num2:f32 123.0`。  

#### 布尔类型

`spdn` 的布尔类型，对应 C++ 的 `bool` 类型。  

#### 字符类型

坦白来说，这个字符类型并没有什么实际用处，它的定义是“由一个 Unicode 码点就能表示的字符”，语法即是由单引号包含的的一个字符：`'A'` `'字'` `'あ'`。  
由于有的符号或形状可以由多个码点组合表示，所以有些看起来是一个字符的内容实际上不能由 `spdn` 的字符表示：`'😀'` 合法，`'🧑‍⚕️'` (可能没有被渲染为“一个医生”)不合法。  

通常我们建议使用字符串，除非你有更好的理由使用字符。  

#### 其他类型

现在还剩下字符串、列表和对象类型，这几个类型我们已经耳熟能详了，也没有什么特别要注意的事情，这里就不再赘述。  

## at 常量和原始字符串和原始字符串字面量

// todo 说明这些内容不重要，先粗略浏览，需要时再学习。
// todo

## 访问不同类型数据的方法

### 用键从对象中查询数据

在 [Hello world](#hello-world) 章节中，我们已经学会解析 `spdn` 文件和查询数据实体的成员，并用 `as_string` 方法将数据当作字符串使用。  

一份 `spdn` 数据被解析后会被装进一个类型为对象的数据实体里，如 `hello.spdn` 被解析后，得到了一个数据实体，里面含有一个键值对 `say: "Hello world!"`。在 C++ 中我们使用下标运算符 `operator[]` 去查询 `say` 所指代的数据，就像在使用一个关联容器，然而它的语义与关联容器如 `map` 的 `operator[]` 并非相同，它只进行查询，而不包含在查询的键不存在时构造新值的操作。  

当对不是对象类型的数据实体使用键的下标运算符时，会抛出 `std::bad_variant_access` 异常，因为本库就是使用 `variant` 达到动态数据实体类型的目标的；访问不存在的数据时，会抛出 `std::out_of_range` 异常。  

```C++
auto& say = entity[u8"say"]; // entity 来自 hello.spdn
say[u8"x"]; // 抛出 bad_variant_access 异常！say是一个字符串而不是对象
entity[u8"name"]; // 抛出 out_of_range 异常！没有一个叫 name 的成员数据
```

有些数据没有那么重要，并且它们有可能不存在，在这种情况我们可能不想使用异常。  

本库提供了实体引用和常实体引用类，它们就是用来应对上述情况的。  
这是一种可以为空的引用(并非 C++ 语言中的引用，请注意区分，下文简称实体的引用为 ref)。  

使用数据实体的成员函数 `ref` 和 `cref` 获取数据实体的引用，当我们不需要更改数据实体时，可以使用常实体引用：  

```C++
auto e_ref = entity.cref();
```

对 ref 进行的查询不会抛出异常，查询成功时返回相应数据实体的 ref，查询失败(该 ref 为空、该 ref 所指代的实体不是对象、该 ref 指代的对象没有相应成员)时返回空 ref：  

```C++
auto say = e_ref[u8"say"]; // say 是一个 ref，它不为空
auto x = say[u8"x"]; // 得到空 ref，say 是一个字符串而不是对象
if (!x) std::cout << "get null ref from say[u8\"x\"]\n";
auto y = x[u8"y"]; // 得到空 ref，因为 x 就是一个空 ref
if (!y) std::cout << "get null ref from x[u8\"y\"]\n";
auto name = e_ref[u8"name"]; // 得到空 ref，没有一个叫 name 的成员数据
if (name.has_value())
{
    std::cout << "has an entity named name\n";
}
else
{
    std::cout << "no entity named name\n";
}
```

ref 对象的 `operator bool` 和 `has_value` 具有相同的功能。  

代码见 `guide-source/query-by-key.cpp`。  

### 从列表中查询数据

列表和对象相似，只不过是改成用编号指代数据而不是键。  
而且对任意类型的实体都有 ref 和 cref 。直接看代码可能更加直观：  

```C++
// guide-source/query-on-list.cpp
#include <iostream>
#include <string>
#include <cassert>

#include "pdn_parse.h"
#include "pdn_data_entity.h"

#include "../outu8sv.h"

int main()
{
    using namespace std::string_view_literals;
    using namespace pdn;
    auto entity = parse(u8R"(list [1, 2, 3, "hello", 5.0])"sv, utf_8_tag);
    // 直接对 spdn 序列进行解析，返回一个 pdn::u8entity，而不再是一个 optional

    const auto& list = entity[u8"list"];

    std::cout << as_int(list[0]) << "\n";
    std::cout << as_int(list[1]) << "\n";
    std::cout << as_int(list[2]) << "\n";
    std::cout << as_string(list[3]) << "\n";
    std::cout << as_fp(list[4]) << "\n";

    // std::cout << as_int(list[5]) << "\n";
    // 没有越界检查！

    try
    {
        // 将实体 list 当作 list 使用，在 C++ 中对应的类型通常是 std::vector<entity>
        auto& e5 = as_list(list).at(5); // vector 的 at 函数带有越界检查
        for (auto& e : as_list(list))
        {
            std::cout << as_int(e) << "\n";
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    auto list_ref = list.ref();
    // 由于 list 是 const pdn::u8entity&，因此 ref 函数返回常实体引用

    std::cout << as_string(list_ref[3]) << "\n";

    auto elem_1 = list_ref[1];
    auto elem_1_0 = elem_1[0]; // elem_1 不是列表，得到空 ref
    assert(!elem_1_0.has_value());
    auto elem_5 = list_ref[5]; // 5 号元素不存在，得到空 ref
    assert(!elem_5.has_value());
    auto nullref = elem_5[0]; // elem_5 是空 ref，得到空 ref
    assert(!nullref.has_value());
}
```

### as 系列函数

上面我们已经用过了一些 as 系列的函数，如 `as_int` `as_string` `as_list` 等，本库的 as 系列函数分为 8 类：`as_int` `as_uint` `as_fp`(fp 是 floating-point 的缩写) `as_bool` `as_char` `as_string` `as_list` `as_object`，全部在头文件 `pdn_data_entity.h`(间接包含，实际定义在 `pdn_entity_as_accessor.h`)，名称空间 `pdn` 中。  

* `as_int` 的返回值类型是 `i64`；  
* `as_uint` 的返回值类型是 `u64`；  
* `as_fp` 的返回值类型是 `f64`；  
* `as_bool` 的返回值类型是 `boolean`，对应 C++ 的 `bool` 类型；  
* `as_char` 的返回值类型是 `character`；  
* `as_string` 的返回值类型通常是是 `std::basic_string<char_t>`；  
* `as_list` 的返回值类型通常是 `std::vector<entity<char_t>>`；  
* `as_object` 的返回值类型通常是 `std::unordered_map<std::basic_string<char_t>, entity<char_t>>`；  

在使用 as 系列函数时，在可能的情况下会尝试类型转换，如实体实际上是浮点数，但我们对它使用了 `as_int`，我们就获得了该实体转换到整数值的结果。  
在实体和 as 函数返回值类型不一样，但是都在整数、浮点数和布尔类型范围内时，会发生这种转换；此外，字符可以转变为布尔值(NUL字符表示 `false`，其余表示 `true`，但布尔值不能转变为字符)，其余类型之间无法进行类型转换，使用 as 函数的结果是一个默认值，如 `0`，空字符串、空列表和空对象。  

当发生转换时，如果要转换的值超过了目标类型的表示范围，那么会发生“截断”，“截断”到目标类型能表示的最大值或最小值(原因之一是有符号整数溢出是未定义行为)：如将 `@infinity` 使用 `as_int` 转换到 `i64`，会得到 `i64` 能表示的最大值。  

此外，将 `nan` 转换到整数时会得到默认值：0。  

// todo 添加示例

#### as 系列函数详解

这段的内容你可能暂时用不到，等之后再来了解也许会更好。  

`spdn` 的有符号整数类型有 `i8` `i16` `i32` `i64` 四种；  
无符号整数类型有 `u8` `u16` `u32` `u64` 四种；  
浮点数有 `f32` `f64` 两种。  

注：被称为 `f32` `f64` 的类型，实际上对应的 C++ 类型是 `float` 和 `double`，通常情况下，实现是 IEEE-754 的 binary32 和 binary64。  

`as_int` `as_uint` `as_fp` 默认的获取类型分别是 `i64` `u64` `f64`。如果你想指定要获取的类型，可以使用名称空间 `pdn` 内的 `i8_tag` - `i64_tag`、`u8_tag` - `u64_tag` 及 `f32_tag` 和 `f64_tag`。  
此外，`pdn::types` 名称空间内有类型别名 `auto_int` 和 `auto_uint`。`auto_int` 要么是 `int`，要么是 `pdn::types::i32`；`auto_uint` 要么是 `unsigned int`，要么是 `pdn::types::u32`(当然也很可能两者都是)。它们同样有对应的 `auto_int_tag` 和 `auto_uint_tag`。  

使用 as 系列函数时，整数、浮点数和布尔值会被转换到目标类型，如对表示 `100.0` 的数据实体使用 `as_bool`，会得到 `true`，再举一个例子，对表示 `200` 的实体 `e` 使用 `as_int(e, i8_tag)`，那么可能会得到 `127`。  

对表示 `character` 的实体使用 `as_bool`，如果实体表示 NUL 字符，得到 `false`，否则得到 `true`，字符不能转换成其他类型的值。  

对于无法转换的数据，会得到一个默认值：对一个表示字符串的实体 `s` 使用 `as_list(s)`，会得到一个空字符串。  

### get 系列函数

// todo

## 解析器的错误处理

### 错误信息生成和报告

// todo 国际化和输出错误信息到文件

### 利用解析器错误处理终止解析

// todo

## 配置自定义的解析器的常量

// todo

### 配置动态的常量

// todo 举例 random

## 使用序列化器及更换 oredered_map

// todo 说明 ordered_map 的不必要性
