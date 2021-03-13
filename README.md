### 介绍

本项目是[2020编译系统设计大赛](https://compiler.educg.net/)的个人实现，使用`C`语言完成，其中编译器前端采用
`flex`和`bison`实现，后端未采用其他框架，直接根据语法树生成汇编代码。

### 进度

- [x] 语法树的生成
- [x] 部分的代码检查
- [x] x86平台的代码生成
- [ ] Arm平台的代码生成
- [ ] 代码优化

### 源代码

```

src/
├── analyze.c # 语义分析
├── analyze.h
├── codegen.c # x86代码生成
├── codegen.h
├── globals.h # 全局头文件
├── main.c    # 入口
├── Makefile
├── parse.c   # 语法分析，由bison生成
├── parse.h
├── parse.y   # bison源文件
├── scan.c    # 词法分析，由flex生成
├── scan.h
├── scan.l    # flex源文件
├── symtab.c  # 符号表
├── symtab.h
├── util.c    # 项目工具
├── util.h
```

### license

MIT
