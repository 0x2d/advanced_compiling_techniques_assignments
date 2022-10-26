# ast-interpreter

## Build
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

    用于自动评测所有测试程序（必须关闭ast-interpreter的DEBUG开关才能使用）。

## Logs
### 20221021
1. 通过test00；bop可以识别整型常量；将输出从标准错误流更改至标准输出流。

### 20221022
1. 为解决Environment类和InterpreterVisitor类交叉引用，重构代码；初步支持Heap、全局变量、IntegerLiteral。

### 20221023
1. Environment类和InterpreterVisitor类解耦合；增加调试代码；初步支持函数调用、if语句、更多二元运算符；通过test01、02、03。

### 20221025
1. 优化自动评测脚本；初步支持一元运算符、while语句；通过test04、05、06。

### 20221026
1. 初步支持for语句；通过test07至11。
2. 初步支持数组变量，对于ArraySubscriptExpr同时提供值和地址，默认使用值，根据需要使用地址；修改了一些由于指针为空导致的bug；细化了调试信息，增强了可视性；通过test12至16。