# PDN 及 本库教程

开始这个项目的契机是我希望有一个可用的数据交换格式，用于给我的以学习为目的编写的程序提供各种配置参数。我不满足于使用现有的 `ini` `xml` `json` 等格式，于是计划设计一个属于自己的数据交换格式，但主要还是用于学习。最开始设计出来的东西非常丑陋(主要是由于相关经验的欠缺)，后来我主要参照了 `C++` 的字面量词法和 `json` 的语法进行设计，于是有了现在的 `spdn` 格式。  

综上，`spdn` 文件的用途就是用来存储各式各样的信息。它的语法和 `json` 有点像，不过它们并不互相兼容。  
此外，`spdn` 提供了一些不太常用的功能，比如原始字符串、类型指定和 at 常量。当然，它们的使用场景有限，不知道这些也完全不影响用户使用。  

这个文档则致力于教学如何使用和编写 `spdn` 格式的文件，以及本库实现的解析器的使用方法。  

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
    // pdn::utf_8_tag 指示其中的文本以 utf-8 进行编码并用 C++ 的 u8string 类型进行描述
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

例如，移动到 `PS .../pdn/docs/guide-source/chapter-1>` 使用以下命令：  

```shell
g++ -std=c++20 -I../../../pdn -o hello-spdn ./hello-spdn.cpp
```

编译之后运行它(`hello-spdn.exe`)，程序会输出 `Hello world!`。

### 一些疑问

为什么不用 `std::string` 而是 `std::u8string`？  

`spdn` 基于 Unicode，我想在类型上明确表达使用 `utf_8_tag` 的解析结果是用 UTF-8 编码的，但是也就不能直接用 `std::cout` 进行输出了。不知道这样是不是设计失误。  

如有必要，可以使用 [`c8rtomb`](https://zh.cppreference.com/w/cpp/string/multibyte/c8rtomb "cppreference") 转换编码。  
此后的教学中就省略对这个问题的讨论了。  

## 一个贴近实际的例子

一个软件所需要的资源可能有很多种，比如一组数据、一些图片等。我们只需要表示数字和文本，其他资源如图片、音频等使用路径或者链接来引用。  

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

这个例子出现的所有冒号和分号都是可选的，对于这种每个键值对后都换行的文件，使用分号反而使它不美观，我写出来只是为了展示每个键值对后都可以使用分号隔开。  

虽然我们使用的列表的元素全部都是字符串，但是它实际上可以存放一组不同类型的数据，包括列表和对象: `list [[ 123, 456 ], { m: 789 }, 123, "string" ]`。  

学会这个例子，你就能熟练地编写 `spdn` 了。  

待续
