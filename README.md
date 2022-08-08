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
