# 本仓库实现的 PDN 解析器

## parser

使用方法：  

创建 `pdn::parser<char_t>` 对象(其 `char_t` 只能为 `char8_t`、`char16_t`、`char32_t` 之一)，构造方法有四个参数，分别是：  

1. `err_handler` 发生错误时将通过此参数通知用户，用户可以选择处理它们，如打印错误信息到日志文件，或是抛出异常以停止解析。  
2. `err_msg_gen` 根据错误码和其他信息生成错误信息，默认参数为错误信息生成器en，用户可以参照默认参数的内容为解析器提供其他语言的错误信息生成器。  
3. `constants_gen` 常量表，对应 PDN At 标识符的相关内容，用户可以提供其他常量表。  
4. `type_gen` 类型生成器，根据标识符生成类型枚举码，对应 PDN 数据类型，用户可以提供其他类型生成器。  

创建好 parser 后使用成员函数 `parse` 解析 pdn 文件：  

1. `parser(auto&& begin, auto end)` 解析 begin 至 end 范围内的内容，begin 必须是 `unicode_code_unit` 类型(`char8_t`、`char16_t`、`char32_t`)的迭代器，且重复对该迭代器解引用得到的代码单元值必须相同。  
2. `parser(const string& | const char* | const wstring& | const wchar_t* filename)` 解析文件中的内容，如果打开失败，将抛出异常。  

## dom

解析后将得到一个 dom，dom 是 `std::variant<pdn_data_types...>` 的派生类，除了可以用 `std::get` `std::get_if` `std::visit` 等方法查询，dom 还以成员函数的方式提供：  

1. `get_xxx` 系列函数：对代理进行处理的 `std::get` 的封装；  
2. `xxx_opt` `xxx_ptr` 系列函数：获得相应类型的值或得到 `nullopt`/`nullptr`；  
3. `xxx_val` 系列函数： 将实体转换为相应类型的值，如果无法转换，将获得一个零初始化或默认初始化的目标类型的值。  
4. `operator[const string<char_t>&]`：获得 Object 的成员数据；  
5. `operator[size_t]`：获得 List 的元素数据；  
6. `ref` `cref`：获得 `refer` 或 `const_refer` 类。  

`refer` 和 `const_refer` 类提供不抛出的 `operator[]`。  
