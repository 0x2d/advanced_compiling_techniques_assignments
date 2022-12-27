# points-to analysis

中国科学院大学2022秋季学期编译程序高级教程 assignment3 points-to analysis 的实现。

## Build
环境：```llvm-10.0.1```
```sh
mkdir build
cd build
cmake -DLLVM_DIR=$LLVM ..
make
```

## Useful Commands
- ```clang -emit-llvm -o0 -g -c ./test/testxx.c -o ./bitcode/testxx.bc```

    用于生成某个测试程序的bitcode。

- ```./run_test.sh```

    用于自动评测所有测试程序。

## Commit Logs

### 20221227
1. 继续完善数据流分析，通过测试02至05。

### 20221222
1. 完成DataFlow框架；初步建立指针分析框架，通过测试00、01。

### 20221215
1. 编译成功。
