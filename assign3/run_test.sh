#! /bin/bash

cd /home/ouyang/advanced_compiling_techniques_assignments/assign3

answer=("14 : plus, minus\n24 : foo\n27 : foo"  "22 : plus, minus"                      "28 : plus, minus" \
        "27 : plus, minus"                      "10 : plus, minus\n26 : foo\n33 : foo"  "33 : plus, minus" \
        "11 : malloc\n15 : plus\n21 : minus"    "19 : plus\n25 : minus"                 "25 : plus\n31 : minus" \
        "21 : malloc\n27 : plus\n33 : minus"    "25 : malloc\n31 : minus\n37 : plus"    "11 : plus, minus\n18 : malloc\n27 : clever" \
        "11 : plus, minus\n21 : malloc\n30 : clever"    "15 : plus, minus\n31 : clever" "10 : plus, minus\n14 : foo\n30 : clever" \
        "15 : plus, minus\n19 : foo\n35 : clever"   "16 : foo\n17 : plus\n24 : malloc\n32 : clever" "20 : foo\n21 : plus, minus\n37 : clever" \
        "30 : foo, clever\n31 : plus, minus"    "24 : foo\n28 : clever\n30 : plus"      "47 : foo, clever\n48 : plus, minus" \
        "15 : plus, minus\n31 : clever"         "17 : plus\n31 : make_simple_alias\n32 : foo"   "14 : plus, minus\n25 : malloc\n26 : malloc\n30 : foo\n31 : make_simple_alias\n33 : foo" \
        "17 : minus\n29 : make_no_alias\n30 : foo"  "21 : plus\n37 : make_alias\n38 : foo"  "31 : malloc\n39 : plus\n40 : make_alias\n45 : minus" \
        "22 : plus\n27 : foo\n44 : clever"      "22 : plus, minus\n29 : foo\n34 : malloc\n36 : malloc\n38 : malloc\n47 : clever"    "21 : minus, plus\n26 : clever\n27 : plus, minus\n41 : malloc\n46 : foo\n51 : foo" \
        "41 : foo, clever\n42 : plus, minus"    "39 : foo\n40 : plus\n44 : clever\n45 : plus, minus\n47 : plus, minus" \
        "38 : foo\n56 : clever\n57 : minus\n58 : minus" "27 : minus\n38 : foo\n70 : make_simple_alias\n75 : make_alias\n81 : clever\n82 : minus\n83 : plus, minus\n84 : swap_w\n85 : minus" \
        "39 : swap_w\n40 : plus, minus\n49 : make_simple_alias\n51 : make_simple_alias\n52 : foo\n68 : make_simple_alias\n70 : make_simple_alias\n75 : make_alias\n79 : clever\n80 : clever\n81 : plus, minus")
i=0
#测试开始和结束的test编号
start=0
end=34

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
