#! /bin/bash

cd /home/ouyang/advanced_compiling_techniques_assignments/assign3

answer=("14 : plus, minus\n24 : foo\n27 : foo"  "22 : plus, minus"                      "28 : plus, minus" \
        "27 : plus, minus"                      "10 : plus, minus\n26 : foo\n33 : foo"  "33 : plus, minus" \
        "11 : malloc\n15 : plus\n21 : minus")
i=0
#测试开始和结束的test编号
start=0
end=6

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
        ./build/assignment3 ./bitcode/`basename $file .c`.bc
        echo -e ">>> \e[1;33m answer \e[0m"
        echo -e ${answer[i]}
    fi
    let i++
done
