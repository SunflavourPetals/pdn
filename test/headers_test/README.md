# 头文件测试

`test/headers_test/` 是逐头测试的 CMake 工程：每个被审的头文件对应一个同名子目录，目录里放一个 `test.cpp`，直接 include `pdn/include` 下的真实头文件（不拷贝副本），所以测试失败指向的就是库里那一行代码。

当前注册 93 个测试，全量运行约 1–2 秒。

## 依赖

- CMake 3.20 或更高
- 一个 C++20 编译器：GCC 11+、Clang 14+、MSVC 19.30+ 都可以；本仓库已用 GCC 与 Clang 各验证过一遍（Windows + MinGW 目标）
- 无第三方库；测试只用标准库与 `pdn/include` 下的头

## 构建与运行

在仓库根目录执行：

```bash
cmake -S test/headers_test -B build/headers_test
cmake --build build/headers_test --parallel
ctest --test-dir build/headers_test --output-on-failure
```

最后一行预期输出 `100% tests passed, 0 tests failed out of 93`。

这三行用的是 CMake 的默认生成器：Linux / macOS 上通常是 `Unix Makefiles`，Windows 上通常是某个 Visual Studio 或 `NMake Makefiles`。默认生成器不可用时（例如命令行里没有 `nmake`，或想固定编译器）显式指定 `-G`：

Linux / macOS，编译器在 `PATH` 里：

```bash
cmake -S test/headers_test -B build/headers_test -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

Windows + MinGW：

```bash
cmake -S test/headers_test -B build/headers_test -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="<path-to-mingw>/bin/g++.exe"
```

Windows + Visual Studio（多配置生成器，构建与 ctest 都要带配置）：

```bash
cmake -S test/headers_test -B build/headers_test -G "Visual Studio 17 2022"
cmake --build build/headers_test --config Debug --parallel
ctest --test-dir build/headers_test -C Debug --output-on-failure
```

装了 Ninja 也可以 `-G Ninja`，速度更快，同样属于单配置生成器。

构建目录不要提交，仓库的 `.gitignore` 已忽略 `build/`。

## 换编译器与指定头文件目录

换编译器时建议换一个构建目录：CMake 把编译器缓存在构建目录里，旧目录里改不干净。

```bash
cmake -S test/headers_test -B build/headers_test -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=clang++
```

Windows 上把 clang 指向 MinGW 运行库时，还需要目标三元组：

```bash
cmake -S test/headers_test -B build/headers_test-clang -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="--target=x86_64-w64-windows-gnu"
```

库头目录默认取 `test/headers_test/../../pdn/include`，也可以覆盖：

```bash
cmake -S test/headers_test -B build/headers_test -DPDN_INCLUDE_DIR=/path/to/pdn/include
```

该目录下必须有 `pdn/spdn.h`，否则配置阶段直接报错 `PDN headers not found`。

## 常用操作

```bash
ctest --test-dir build/headers_test -N                                       # 列出测试
ctest --test-dir build/headers_test -R pdn_lexer --output-on-failure         # 只跑一个
ctest --test-dir build/headers_test -R "pdn_(lexer|parser)" --output-on-failure   # 按正则跑一组
ctest --test-dir build/headers_test --rerun-failed                           # 只重跑上次失败的
ctest --test-dir build/headers_test -j 4 --output-on-failure                 # 并行
```

每个测试本身是普通可执行文件，也可以直接运行（失败时退出码非 0）：

```bash
./build/headers_test/pdn_lexer/test_pdn_lexer
```

## 编译期开关

库的行为由若干宏控制（见 `docs/pdn-api.md` 的“编译期开关”一节）。这些宏必须对整个程序一致，所以整份测试一起定义：

```bash
cmake -S test/headers_test -B build/headers_test-noext -DCMAKE_CXX_FLAGS=-DPDN_NO_EXTENSIONS
```

同理可用 `-DPDN_USER_OBJECT=::pdn::ordered_map`、`-DPDN_NO_EXCEPTIONS -DPDN_USER_TERMINATE=std::abort` 等组合。建议每种组合用一个独立的构建目录。

## 大 TU 与 `-Wa,-mbig-obj`

少数测试包含伞形头 `pdn/spdn.h`，在 Windows + MinGW 下单个目标文件会超过 COFF 默认的节大小，汇编器报 `file too big`。这些目录的 `CMakeLists.txt` 里因此写了：

```cmake
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_spdn PRIVATE -Wa,-mbig-obj)
endif()
```

`-Wa,-mbig-obj` 只对 GNU 汇编器、且只在 Windows 目标上有意义，所以条件里同时判 `WIN32`：Linux 上的 GNU as 不认这个选项。

## 新增一个测试

1. 新建 `test/headers_test/pdn_<名字>/` 目录，放 `test.cpp`：只 include 被测头（需要时再补标准头），写 `int main()`，用 `assert` 或自己的 `CHECK` 宏断言，失败返回非 0。
2. 同目录放 `CMakeLists.txt`，四行：

    ```cmake
    add_executable(test_pdn_<name> test.cpp)
    target_include_directories(test_pdn_<name> PRIVATE "${PDN_INCLUDE_DIR}")
    target_compile_features(test_pdn_<name> PRIVATE cxx_std_20)
    add_test(NAME pdn_<name> COMMAND test_pdn_<name>)
    ```

3. 在 `test/headers_test/CMakeLists.txt` 末尾追加 `add_subdirectory(pdn_<name>)`。

命名保持 ASCII 小写与下划线：可执行文件叫 `test_pdn_<name>`，测试名（`ctest -R` 用的）叫 `pdn_<name>`。新目标如果也会撞上大 TU 问题，照上一节补 `-Wa,-mbig-obj`。改完直接重新构建即可，CMake 会在构建时自动重新配置。

## 常见问题

- **默认生成器配置失败**（例如报 `'nmake' '-?' failed`、`CMAKE_CXX_COMPILER not set`）：显式加 `-G`，见上面各平台示例。
- **配置时报 `PDN headers not found`**：`PDN_INCLUDE_DIR` 指错了，它必须是包含 `pdn/spdn.h` 的目录（也就是 `pdn/include`，不是 `pdn/`）。
- **Windows + MinGW 汇编或链接报 `file too big`、`relocation truncated`**：见上一节，给该目标加 `-Wa,-mbig-obj`。
- **多配置生成器下 ctest 报 `Unable to find executable`**：构建用了 `--config Debug` 而 ctest 没加 `-C Debug`，两边保持一致即可。
- **改了头文件但测试结果没变**：确认构建目录对应的是这份源码；`cmake --build` 会增量重编，不需要每次重新配置。
- **换编译器后配置失败**：CMake 缓存了旧编译器，删掉该构建目录重新配置。

## 关于本测试

本测试和 readme 由 Deepseek v4.1flash 1017 按要求生成，库作者审查与提交。  
