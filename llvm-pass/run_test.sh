#! /bin/bash

cd /home/ouyang/advanced_compiling_techniques_assignments/llvm-pass

answer=("10 : plus" "22 : plus")
i=0

for file in `ls ./test`
do
    clang -emit-llvm -o0 -g -c ./test/$file -o ./bitcode/`basename $file .c`.bc

    echo "-----$file-----"
    ./build/llvmassignment ./bitcode/`basename $file .c`.bc
    echo "-----answer"
    echo ${answer[i]}

    let i++
    #这里控制运行哪些测试用例
    if [ $i -eq 2 ]
    then
        break
    fi
done
