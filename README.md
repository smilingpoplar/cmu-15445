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

### [proj2](https://15445.courses.cs.cmu.edu/fall2021/project2/) Extendible Hash Index
#### Page布局
实现基于磁盘的hash表，包括DirectoryPage和BucketPage
- 所有内容都存在磁盘页中，通过BufferPoolManager获取page，再`reinterpret_cast<XXXPage *>(page)`成DirectoryPage或BucketPage。可以reinterpret_cast<>是因为Page结构的首字段为`char data_[PAGE_SIZE]{}`。
- DirectoryPage和BucketPage不能添加成员变量，因为PAGE_SIZE=4K是固定的
- DirectoryPage保存hash表元数据，含bucket_idx=>page_id的映射
- BucketPage保存键值对，不允许键值都相同的重复值
##### BucketPage
页头位图occupied_和readable_
- slot一旦占用就不释放，删除时只要置零readable_位，相当于设置tombstone标记
- occupied_位图主要用来判断遍历终结，readable_位图表示slot是否可读

#### Extendible Hashing
[算法解释](https://www.geeksforgeeks.org/extendible-hashing-dynamic-approach-to-dbms/)
- 初始化时新建1个DirectoryPage和1个BucketPage
- 读写page后要`buffer_pool_manager_->UnpinPage()`，因为buffer_pool_manager_获取的都是pinned page
- 一共`2^global_depth`个bucket，用`bucket_idx = Hash(key) & global_depth_mask`寻址
- bucket的`local_depth <= global_depth`，有`2^(global_depth - local_depth)`个表项指向它
- `table_latch_`读写锁用来锁DirectoryPage，`auto page = reinterpret_cast<Page *>(bucket_page);`后page所含读写锁用来锁BucketPage
- 当插入键值时，若bucket已满，触发bucket分裂：若`local_depth == global_depth`，先把hash表项复制扩展一倍、`global_depth++`，这样就有两倍的表项指向原来的桶；bucket分裂就是新建一个桶，把指向原桶的一半表项指向新桶，在原桶和新桶两者间重新分配键值对。
- split_image指一个bucket的镜像桶，把`bucket_idx ^ local_high_bit`而得。记(bucket_idx)_ld，ld表示local_depth，则(101)_1的镜像桶为(111)_1，(101)_0的镜像桶为(100)_0。
- 当删除键值时，若`bucket->IsEmpty() && local_depth > 0 && local_depth == split_image_local_depth`，把空桶并入镜像桶：把所有指向原桶和镜像桶的表项都指向镜像桶，`local_depth--`；若所有`local_depth < global_depth`，`global_depth--`。并入后还应在镜像桶上继续尝试合并。

##### gradescope上的测试用例
- 本地测试用例太少，把[下面函数](https://github.com/smilingpoplar/cmu-15445/tree/main/test/gradescope/print_test_files.cpp)加入待提交文件（比如`extendible_hash_table.cpp`）中，提交gradescope，拷贝输出的测试用例代码。
##### 调试
- 在split或merge后用`VerifyIntegrity()`验证页表
- 遇到异常退出，用`lldb ./test/xxx_test`启动，查看堆栈
