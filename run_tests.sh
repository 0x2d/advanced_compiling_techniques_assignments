#! /bin/bash
#方括号前后必须加空格，赋值语句不可以加空格

cd /home/ouyang/ast-interpreter

answer=(100 10 20 200 10 10 20 10 20 20 5 100 4 20 12 -8 30)
i=0

for file in `ls /home/ouyang/ast-interpreter/test`
do
    output=$(./build/ast-interpreter "`cat ./test/$file`")

    if [ $output -eq ${answer[i]} ]
    then
        echo "$file passed!"
    else
        echo "$file failed!"
    fi

    let i++
    if [ $i -eq 17 ]
    then
        break
    fi
done
