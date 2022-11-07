#! /bin/bash

cd /home/ouyang/advanced_compiling_techniques_assignments/llvm-pass

answer=("10 : plus" "22 : plus" "24 : plus, minus" "27 : plus, minus" "10 : plus, minus\n26 : foo\n33 : foo" \\
            "33 : plus, minus" "10 : plus, minus\n26 : clever" "10 : plus, minus\n28 : clever\n30 : clever" "10 : plus, minus\n26: clever" \\
            "10 : plus, minus\n14 : foo \n30 : clever" "15 : plus, minus\n19 : foo\n35 : clever" "15 : foo\n16 : plus\n32 : clever" \\
)
i=0

for file in `ls ./test`
do
    clang -emit-llvm -o0 -g -c ./test/$file -o ./bitcode/`basename $file .c`.bc

    echo "-----$file-----"
    ./build/llvmassignment ./bitcode/`basename $file .c`.bc
    echo ">>> answer"
    echo ${answer[i]}

    let i++
    #这里控制运行哪些测试用例
    if [ $i -eq 12 ]
    then
        break
    fi
done
