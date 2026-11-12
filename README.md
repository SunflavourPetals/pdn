# My data notation

## 本项目编译方法

1. 打开 pdn.sln，在 VS 中编译；  
2. 移动至 `./pdn` 目录内，使用命令行编译。

### 命令行编译方式

1. cl: 打开 VS 开发人员命令提示，移动至 `./pdn` 目录内，编译命令 `cl /EHsc /std:c++20 /Iinclude main.cpp`；
2. g++: 移动至 `./pdn` 目录内，编译命令 `g++ -std=c++20 -Iinclude -o main main.cpp`。  

[参考 MSDN](https://learn.microsoft.com/zh-cn/cpp/build/walkthrough-compiling-a-native-cpp-program-on-the-command-line)  

## 使用本项目的 pdn::parse 方法解析 .spdn(Petals Data Notation) 文件

将 `./pdn/include` 加入项目包含目录，在使用 `pdn::parse` 的程序中使用 `#include` 预处理指令引入 `<pdn/pdn_parse.h>`，解析成功时 `pdn::parse_result<char_t>` 中用 `operator*` 取出的数据实体一定是 `object`。  
如果只使用 `pdn::entity`，仅引入 `<pdn/pdn_entity.h>` 即可。  
如果不想纠结，可以直接引入 `<pdn/spdn.h>`，它包含 `pdn_entity.h` `pdn_parse.h` `pdn_serializer.h`。  
