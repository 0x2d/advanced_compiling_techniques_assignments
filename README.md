# 中国科学院大学2022秋季学期编译程序高级教程作业

- ```add_compile_options(-fno-rtti)```

    ```CMakeLists.txt```中缺少这个编译选项会导致自行编译的```LLVM```链接失败，且该命令必须置于add_executable之前。
