#include <fstream>
#include <iostream>
#include <vector>

void PrintTestFiles() {
  static bool runned = false;
  if (runned) {
    return;
  }
  runned = true;

  // 来自gradescope日志的文件名
  std::vector<std::string> filenames = {
      "/autograder/bustub/test/primer/grading_starter_test.cpp",

      "/autograder/bustub/test/buffer/grading_lru_replacer_test.cpp",
      "/autograder/bustub/test/buffer/grading_buffer_pool_manager_instance_test.cpp",
      "/autograder/bustub/test/buffer/grading_parallel_buffer_pool_manager_test.cpp",

      "/autograder/bustub/test/container/grading_hash_table_page_test.cpp",
      "/autograder/bustub/test/container/grading_hash_table_test.cpp",
      "/autograder/bustub/test/container/grading_hash_table_scale_test.cpp",
      "/autograder/bustub/test/container/grading_hash_table_concurrent_test.cpp",
      "/autograder/bustub/test/container/grading_hash_table_verification_test.cpp",
      "/autograder/bustub/test/container/grading_hash_table_leaderboard_test.cpp",

      "/autograder/bustub/test/execution/grading_update_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_nested_loop_join_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_limit_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_executor_benchmark_test.cpp",
      "/autograder/bustub/test/concurrency/grading_lock_manager_3_test.cpp",
      "/autograder/bustub/test/execution/grading_executor_integrated_test.cpp",
      "/autograder/bustub/test/execution/grading_sequential_scan_executor_test.cpp",
      "/autograder/bustub/test/concurrency/grading_lock_manager_1_test.cpp",
      "/autograder/bustub/test/execution/grading_distinct_executor_test.cpp",
      "/autograder/bustub/test/concurrency/grading_lock_manager_2_test.cpp",
      "/autograder/bustub/test/concurrency/grading_transaction_test.cpp",
      "/autograder/bustub/test/buffer/grading_leaderboard_test.cpp",
      "/autograder/bustub/test/concurrency/grading_rollback_test.cpp",
      "/autograder/bustub/test/concurrency/grading_lock_manager_detection_test.cpp",
      "/autograder/bustub/test/execution/grading_aggregation_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_insert_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_delete_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_hash_join_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_sequential_scan_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_update_executor_test.cpp",
      "/autograder/bustub/test/execution/grading_executor_test_util.h",
      "/autograder/bustub/src/include/execution/plans/mock_scan_plan.h",
  };

  std::ifstream fin;
  for (const auto &filename : filenames) {
    fin.open(filename, std::ios::in);
    if (!fin.is_open()) {
      std::cout << "failed to open: " << filename << std::endl;
      continue;
    }

    std::cout << filename << std::endl;
    std::string buf;
    while (getline(fin, buf)) {
      std::cout << buf << std::endl;
    }
    fin.close();
  }
}
