#! /bin/bash

cd /home/ouyang/advanced_compiling_techniques_assignments/llvm-pass

answer=("10 : plus" "22 : plus" "24 : plus, minus" "27 : plus, minus" "10 : plus, minus\n26 : foo\n33 : foo" \
            "33 : plus, minus" "10 : plus, minus\n26 : clever" "10 : plus, minus\n28 : clever\n30 : clever" "10 : plus, minus\n26 : clever" \
            "10 : plus, minus\n14 : foo \n30 : clever" "15 : plus, minus\n19 : foo\n35 : clever" "15 : foo\n16 : plus\n32 : clever" \
            "15 : foo\n16 : plus, minus\n32 : clever" "30 : foo, clever\n31 : plus, minus" "24 : foo\n31 : clever,foo\n32 : plus, minus" \
            "14 : plus, minus\n24 : foo\n27 : foo" "14 : foo\n17 : clever\n24 : clever1\n25 : plus" "14 : foo\n17 : clever\n24 : clever1\n25 : plus" \
            "14 : foo\n18 : clever, foo\n30 : clever1\n31 : plus" "14 : foo\n18 : clever, foo\n24 : clever, foo\n36 : clever1\n37 : plus")
i=0
#测试开始和结束的test编号
start=0
end=19

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
        echo -e "\033[32m-----$file-----\033[0m"
        ./build/llvmassignment ./bitcode/`basename $file .c`.bc
        echo -e ">>> \e[1;33m answer \e[0m"
        echo -e ${answer[i]}
    fi
    let i++
done
