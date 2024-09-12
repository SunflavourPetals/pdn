My data notation.

## 本项目编译方法

1. 打开 pdn.sln，在 VS 中编译；  
2. 移动至 `./pdn` 目录内，使用命令行编译。

### 命令行编译方式：

1. cl: 打开 VS 开发人员命令提示，移动至 `./pdn` 目录内，编译命令 `cl /EHsc /std:c++20 /utf-8 pdn.cpp`；
2. g++: 移动至 `./pdn` 目录内，编译命令 `g++ -std=c++20 -o pdn pdn.cpp`。  

[参考 MSDN](https://learn.microsoft.com/zh-cn/cpp/build/walkthrough-compiling-a-native-cpp-program-on-the-command-line)  

## 使用本项目的 parser

将 `./pdn` 内的所有头文件 `.h` 引入项目文件夹，在使用 `pdn::parser` 的程序中使用 `#include` 预处理指令引入 `pdn_parser.h`，解析结果的类型为模板 `pdn::dom` 的实例。  
如果只使用 `pdn::dom`，仅引入 `pdn_types.h` 即可。  
