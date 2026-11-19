# My data notation

本项目内容是一个自制数据交换格式及其解析器的实现，解析器使用 C++20，包含文档和解析器的 header only 库。文档参考 `docs`，头文件目录 `pdn/include`。  

这个数据交换格式比较像 `json`，但是更加灵活，支持注释……  
看起来像这样：  

```spdn
weekend_plan {
    date: { y: 2026; m: 11; d: 20; }
    weather: "晴"
    outdoor: @true
    destination: "公园"
    items [
        { name: "水"   count: 2 },
        { name: "面包" count: 1 },
    ]
    note: "" // 笔记一个字都没动
}
```

详见 [pdn-reference](./docs/pdn-reference.md)  

关于解析器的使用方式，详见 [pdn-guide](./docs/pdn-guide.md) 和 [pdn-api](./docs/pdn-api.md)  

## 使用 CMake 构建

`pdn` 是 header-only 库，仓库根目录的 CMake 负责构建 demo、测试和安装配置。

以 Windows + MinGW 为例：  

```shell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

Linux/macOS 去掉 `-G "MinGW Makefiles"` 即可：  

```shell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Windows 上如果省略 `-G`，CMake 可能回退到 NMake；当 `nmake` 不在 `PATH` 时会配置失败。也可以直接使用 `cmake --preset mingw-debug`。

只需要 demo 时：

以 Windows + MinGW 为例：  

```shell
cmake -S . -B build -G "MinGW Makefiles" -DPDN_BUILD_TESTS=OFF
cmake --build build
build/pdn_demo
```

Linux/macOS 同样去掉 `-G "MinGW Makefiles"` 即可：  

```shell
cmake -S . -B build -DPDN_BUILD_TESTS=OFF
cmake --build build
build/pdn_demo
```

Windows 下可执行文件是 `build/pdn_demo.exe`。

安装后，消费端可以通过 `find_package(pdn CONFIG REQUIRED)` 使用 `pdn::pdn`：

```shell
cmake --install build --prefix <安装目录>
```

## 本项目 demo 编译方法

1. 使用 CMake 构建；  
2. 打开 pdn.sln，在 VS 中构建；  
3. 移动至 `./pdn` 目录内，使用命令行编译。

### 命令行编译方式

1. cl: 打开 VS 开发人员命令提示，移动至 `./pdn` 目录内，编译命令 `cl /EHsc /std:c++20 /Iinclude main.cpp`；
2. g++: 移动至 `./pdn` 目录内，编译命令 `g++ -std=c++20 -Iinclude -o main main.cpp`。  

[参考 MSDN](https://learn.microsoft.com/zh-cn/cpp/build/walkthrough-compiling-a-native-cpp-program-on-the-command-line)  
