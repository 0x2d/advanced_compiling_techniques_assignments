# llvm-pass

中国科学院大学2022秋季学期编译程序高级教程 assignment2 llvm-pass 的实现。编写llvm pass打印IR中的函数调用。

## Build
环境：```llvm-10.0.1```
```sh
mkdir build
cd build
cmake -DLLVM_DIR=$LLVM -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-std=c++14" ..
make
```

## Useful Commands
- ```clang -emit-llvm -o0 -g -c ./test/testxx.c -o ./bitcode/testxx.bc```

    用于生成某个测试程序的bitcode。

- ```./run_test.sh```

    用于自动运行测试程序。

## Commit Logs

### 20221109
1. 解决嵌套CallInst的问题，此时真正的函数调用是内层调用的返回值；通过test11、12。

### 20221107
1. 初步支持PHINode；通过test01。
2. 初步支持Argument；通过test02至10。

### 20221106
1. 完成了基本的指令遍历；通过test00。

### 20221102
1. 编译成功。