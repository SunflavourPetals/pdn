# PDN benchmark

只使用 C++20 标准库和 `std::chrono::steady_clock`。

## 构建

```sh
cmake -S benchmark -B build/benchmark -G "MinGW Makefiles"
cmake --build build/benchmark -j
```

MSVC：

```bat
cmake -S benchmark -B build\benchmark
cmake --build build\benchmark --config Release -j
```

单配置生成器默认使用 `Release`；如果是多配置生成器，请显式加 `--config Release`。

会生成两个可执行文件：

- `pdn_benchmark`：默认的 `unordered_map` DOM。
- `pdn_benchmark_ordered`：定义 `PDN_ENABLE_ORDERED_MAP` 的 DOM。

## 运行

```sh
./build/benchmark/pdn_benchmark
./build/benchmark/pdn_benchmark_ordered
```

参数：

- `--no-file`：跳过文件输入用例。
- `--help`：显示帮助。

## 测什么

每个用例先预热 3 次，再用一次调用估算迭代次数，在约 0.30 秒内循环执行，最后报告：

- `bytes`：输入字节数（序列化用例是输出字节数）。
- `iters`：实际迭代次数。
- `total_ms`：计时总时长。
- `ns/op`：单次操作平均耗时。
- `MiB/s`：吞吐量。
- `checksum`：用例期间结果累加值，防止优化器把被测代码删掉；同时可用于发现解析结果意外为空。

覆盖的路径：

- `parse/empty`：固定开销基线。
- `parse/scalars-1k`、`parse/scalars-10k`：大量标量成员。
- `parse/scalars-10k-utf16`、`parse/scalars-10k-utf32`：UTF-16/UTF-32 输入。
- `parse/list-100k`：大列表。
- `parse/nested`：多层嵌套对象/列表。
- `parse/strings-5k`：带转义的字符串。
- `parse/comments-5k`：注释密集的输入。
- `parse/typed-5k`：显式写类型的成员。
- `parse/file-scalars-10k`：通过文件名走文件输入链路（包含打开/读取，结果受 OS 文件缓存影响）。
- `serialize/scalars-10k`、`serialize/nested`、`serialize/strings-5k`：DOM 序列化。
- `roundtrip/scalars-10k`：解析 + 序列化 + 再解析。

## 结果解读

- 这是微基准，不是完整应用 benchmark；重点看同一台机器、同一编译器、同一构建类型下的相对变化。
- 文件用例包含 I/O，不能直接和内存解析的 `MiB/s` 比较。
- `unordered` 与 `ordered` 的序列化输出顺序不同，不要拿序列化文本做跨配置对比。
- 优化级别对结果影响很大；请使用 Release，并在报告结果时注明编译器版本和 CPU。

## 关于本 benchmark

本 benchmark 和 readme 由 Deepseek v4.1flash 1017 按要求生成，库作者审查与提交。  

