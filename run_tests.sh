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

output=$(./build/ast-interpreter "`cat ./test/test02.c`")

if [ $output -eq 20 ]
then
    echo "Test02 passed!"
else
    echo "Test02 failed!"
fi

output=$(./build/ast-interpreter "`cat ./test/test03.c`")

if [ $output -eq 200 ]
then
    echo "Test03 passed!"
else
    echo "Test03 failed!"
fi

output=$(./build/ast-interpreter "`cat ./test/test04.c`")

if [ $output -eq -100 ]
then
    echo "Test04 passed!"
else
    echo "Test04 failed!"
fi
