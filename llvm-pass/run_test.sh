#! /bin/bash

cd /home/ouyang/advanced_compiling_techniques_assignments/llvm-pass

answer=("10 : plus" "22 : plus" "24 : plus, minus" "27 : plus, minus" "10 : plus, minus\n26 : foo\n33 : foo" \
            "33 : plus, minus" "10 : plus, minus\n26 : clever" "10 : plus, minus\n28 : clever\n30 : clever" "10 : plus, minus\n26: clever" \
            "10 : plus, minus\n14 : foo \n30 : clever" "15 : plus, minus\n19 : foo\n35 : clever" "15 : foo\n16 : plus\n32 : clever" \
            "15 : foo\n16 : plus, minus\n32 : clever" "30 : foo, clever\n31 : plus, minus")
i=0
#测试开始和结束的test编号
start=13
end=13

for file in `ls ./test`
do
    if [ $i -ge $start ] && [ $i -le $end ]
    then
        #判断是否已经生成了bitcode文件
        if [ ! -e "./bitcode/`basename $file .c`.bc" ]
        then
            clang -emit-llvm -o0 -g -c ./test/$file -o ./bitcode/`basename $file .c`.bc
        fi

        #输出结果和正确答案
        echo "-----$file-----"
        ./build/llvmassignment ./bitcode/`basename $file .c`.bc
        echo "  >>> answer"
        echo -e ${answer[i]}
    fi
    let i++
done
