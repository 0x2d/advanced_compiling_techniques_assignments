#! /bin/bash
#方括号前后必须加空格，赋值语句不可以加空格

cd /home/ouyang/ast-interpreter

output=$(./build/ast-interpreter "`cat ./test/test00.c`")

if [ $output -eq 100 ]
then
    echo "Test00 passed!"
else
    echo "Test00 failed!"
fi

output=$(./build/ast-interpreter "`cat ./test/test01.c`")

if [ $output -eq 10 ]
then
    echo "Test01 passed!"
else
    echo "Test01 failed!"
fi