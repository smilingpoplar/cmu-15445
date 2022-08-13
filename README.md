## CMU 15-445 数据库系统 [(2021)](https://15445.courses.cs.cmu.edu/fall2021/schedule.html)

课程视频：[2019](https://www.youtube.com/playlist?list=PLSE8ODhjZXjbohkNBWQs_otTrBTrjyohi)，[17-19课看2018](https://www.youtube.com/watch?v=E0zvyYkdXbU&list=PLSE8ODhjZXja3hgmuwhf89qboV1kOxMx7&index=17)

### [proj0](https://15445.courses.cs.cmu.edu/fall2021/project0/) C++ Primer

#### 环境搭建
- cmake找不到clang-format和clang-tidy：如果homebrew装在/opt/homebrew，修改CMakeLists.txt的BUSTUB_CLANG_SEARCH_PATH：`set(BUSTUB_CLANG_SEARCH_PATH ... "/opt/homebrew/opt/llvm@12/bin")`

- 编译运行
```
mkdir build
cd build
cmake ..
make -j starter_test
./test/starter_test
```
- 若开启Debug：`cmake -DCMAKE_BUILD_TYPE=Debug ..`，AddressSanitizer可能误报heap-buffer-overflow

```
export ASAN_OPTIONS=detect_container_overflow=0
```

- 格式检查
```
make format
make check-lint
make check-clang-tidy
```
#### vscode
- [vscode debugging with cmake-tools](https://vector-of-bool.github.io/docs/vscode-cmake-tools/debugging.html#debugging-with-cmake-tools-and-launch-json)

  <details>
    <summary>.vscode/launch.json 调试启动文件</summary>
    
    ```
    {
        "version": "0.2.0",
        "configurations": [
            {
                "name": "(lldb) Debug",
                "type": "cppdbg",
                "request": "launch",
                // Resolved by CMake Tools:
                "program": "${command:cmake.launchTargetPath}",
                "args": [],
                "stopAtEntry": false,
                "cwd": "${workspaceFolder}",
                "environment": [],
                "externalConsole": false,
                "MIMode": "lldb"
            }
        ]
    }
    ```
    </details>
- 启动debug：Cmd+Shift+P，Cmake:设置调试目标，Cmake：调试

  [若debug开始时卡住](https://github.com/microsoft/vscode-cpptools/issues/5805#issuecomment-1102836008)：暂时关闭"变量"面板
#### gradescope
- 注册2021Fall课程：邀请码4PR8G5，学校选Carnegie Mellon University
- 提交前打包：`zip proj0.zip src/include/primer/p0_starter.h`

### [proj1](https://15445.courses.cs.cmu.edu/fall2021/project1/) Buffer Pool
#### LRUReplacer
- frame是个空槽，存放从磁盘读到内存的page
- replacer只保存`unpinned frame`的frame_id。`pinned frame`对应的page正被引用，不能被换出内存，要从replacer中删除。
#### BufferPoolManagerInstance
- page_tables记录虚拟页page_id到物理页frame_id的映射
- 空闲页优先从free_list中获取
- 从replacer中换出物理页时，要回写脏页、删掉page_table中映射
- 本类是Page的友元类，可访问Page的私有成员
#### 若gradescope报错
> The autograder failed to execute correctly. Please ensure that your submission is valid. Contact your course staff for help in debugging this issue. Make sure to include a link to this page so that they can help you most effectively.

- 是个编译问题。最好写完一个task就提交gradescope测试。
- 我遇到的问题：FlushAllPgsImp()中遍历写`for (auto [page_id, frame_id] : page_table_)`会报错，写`for (const auto &e : page_table_)`没问题
