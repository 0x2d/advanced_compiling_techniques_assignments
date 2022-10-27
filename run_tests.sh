#! /bin/bash

cd /home/ouyang/ast-interpreter

answer=(100 10 20 200 10 10 20 10 20 20 5 100 4 20 12 -8 30 10 1020 1020 5 33312826232118161311863491419242934 2442 2442 2442)
i=0

for file in `ls /home/ouyang/ast-interpreter/test`
do
    output=$(./build/ast-interpreter "`cat ./test/$file`")

    if [ $output == ${answer[i]} ]
    then
        echo "$file passed!"
    else
        echo "$file failed!"
    fi

    let i++
    #这里控制运行哪些测试用例
    if [ $i -eq 25 ]
    then
        break
    fi
done
