# My data notation

## 本项目编译方法

1. 打开 pdn.sln，在 VS 中编译；  
2. 移动至 `./pdn` 目录内，使用命令行编译。

### 命令行编译方式

1. cl: 打开 VS 开发人员命令提示，移动至 `./pdn` 目录内，编译命令 `cl /EHsc /std:c++20 main.cpp`；
2. g++: 移动至 `./pdn` 目录内，编译命令 `g++ -std=c++20 -o main main.cpp`。  

[参考 MSDN](https://learn.microsoft.com/zh-cn/cpp/build/walkthrough-compiling-a-native-cpp-program-on-the-command-line)  

## 使用本项目的 pdn::parse 方法解析 .spdn(Petals Data Notation) 文件

将 `./pdn` 内的所有头文件 `.h` 引入项目包含目录，在使用 `pdn::parse` 的程序中使用 `#include` 预处理指令引入 `pdn_parse.h`，解析结果的类型为模板 `pdn::data_entity` 的实例，并且这个实例一定是 `object`。  
如果只使用 `pdn::data_entity`，仅引入 `pdn_data_entity.h` 即可。  
