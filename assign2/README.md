# assign2

## Build
环境：```llvm-10.0.1```
```sh
mkdir build
cd build
cmake -DLLVM_DIR=$LLVM -DCMAKE_CXX_FLAGS="-std=c++14" ..
make
```

## Useful Commands
- ```clang -emit-llvm -c ./test/testxx.c -o ./test/bitcode/testxx.bc```

    用于生成某个测试程序的bitcode。

- ```./run_test.sh```

    用于自动评测所有测试程序（必须关闭DEBUG开关）。

## Commit Logs

### 20221102
1. 编译成功。