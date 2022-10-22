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

    用于自动评测所有测试程序。

## Logs
### 20221021
1. 通过test00；bop可以识别整型常量；将输出从标准错误流更改至标准输出流。

### 20221022
1. 为解决Environment类和InterpreterVisitor类交叉引用，重构代码；初步支持Heap、全局变量、IntegerLiteral。