cd /home/ouyang/advanced_compiling_techniques_assignments/assign2

clang -emit-llvm -c ./test/test00.c -o ./test/bitcode/test00.bc

./build/llvmassignment ./test/bitcode/test00.bc