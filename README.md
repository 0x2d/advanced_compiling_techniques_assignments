# ast-interpreter

中国科学院大学2022秋季学期编译程序高级教程 assignment1 ast-interpreter 的实现。使用Clang前端，构建一个简单的C语言解释器。

## Build
环境：```llvm-10.0.1```
```sh
mkdir build
cd build
cmake -DLLVM_DIR=your_llvm_intalled_location ..
make -j6
```

## Useful Commands
- ```clang -fsyntax-only -Xclang -ast-dump testxx.c```

    用于打印某个测试程序的AST树。

- ```./run_tests.sh```

    用于自动评测所有测试程序（必须关闭ast-interpreter的DEBUG开关）。

## Commit History

### 20221028
1. 将Char类型视为Integer类型，~~ 想不明白为啥之前无视Char类型也能过测试 ~~ 我懂了，因为clang::Type::isInteger()对于所有Char相关的类型也返回true，所以我这次的commit完全没意义。

### 20221027
1. 函数调用时令FunctionDecl指针指向函数定义而非函数声明；支持二元运算符>=、<=；虽然不支持char类型，但是通过了test21-24。完结撒花！！！

### 20221026
1. 初步支持for语句；通过test07至11。
2. 初步支持数组变量，对于ArraySubscriptExpr同时提供值和地址，默认使用值，根据需要使用地址；修改了一些由于指针为空导致的bug；细化了调试信息，增强了可视性；通过test12至16。
3. 初步支持malloc和指针变量，支持4字节和8字节变量；新增支持ParenExpr、UnaryExprOrTypeTraitExpr、指针运算；新增测试用例；通过test17-19。
4. 修复了一些BUG；使用try catch语句支持函数的返回；通过test20。

### 20221025
1. 优化自动评测脚本；初步支持一元运算符、while语句；通过test04、05、06。

### 20221023
1. Environment类和InterpreterVisitor类解耦合；增加调试代码；初步支持函数调用、if语句、更多二元运算符；通过test01、02、03。

### 20221022
1. 为解决Environment类和InterpreterVisitor类交叉引用，重构代码；初步支持Heap、全局变量、IntegerLiteral。

### 20221021
1. 通过test00；bop可以识别整型常量；将输出从标准错误流更改至标准输出流。
