# assign2

## Build
环境：```llvm-10.0.1```
```sh
mkdir build
cd build
cmake -DLLVM_DIR=$LLVM -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-std=c++14" ..
make
```

## Useful Commands
- ```clang -emit-llvm -o0 -g -c ./test/testxx.c -o ./test/bitcode/testxx.bc```

    用于生成某个测试程序的bitcode。

- ```./run_test.sh```

    用于自动运行所有测试程序。

## Commit Logs

### 20221106
1. 完成了基本的指令遍历；通过test00。

### 20221102
1. 编译成功。