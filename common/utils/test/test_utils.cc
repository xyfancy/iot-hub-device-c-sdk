/**
 * @copyright
 *
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright(C) 2018 - 2021 THL A29 Limited, a Tencent company.All rights reserved.
 *
 * Licensed under the MIT License(the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file test_utils.cc
 * @brief unittest for utils
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-07-07
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-07-07 <td>1.0     <td>fancyxu   <td>first commit
 * <tr><td>2021-07-27 <td>1.1     <td>fancyxu   <td>support utils json
 * <tr><td>2021-07-29 <td>1.1     <td>fancyxu   <td>rename HAL_Timer and add utils json test
 * </table>
 */

#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "qcloud_iot_platform.h"
#include "utils_json.h"
#include "utils_list.h"
#include "utils_log.h"

namespace utils_unittest {

/**
 * @brief test fixture of utils list
 *
 */
class UtilsListTest : public testing::Test {
 protected:
  void SetUp() override {
    UtilsListFunc func = {
        .list_malloc = HAL_Malloc,
        .list_free = HAL_Free,
        .list_lock_init = HAL_MutexCreate,
        .list_lock = HAL_MutexLock,
        .list_unlock = HAL_MutexUnlock,
        .list_lock_deinit = HAL_MutexDestroy,
    };

    self_list = utils_list_create(func, 10);
    ASSERT_NE(self_list, nullptr);

    for (int i = 0; i < 10;) {
      int *val = reinterpret_cast<int *>(HAL_Malloc(sizeof(int)));
      *val = i++;
      ASSERT_NE(utils_list_push(self_list, reinterpret_cast<void *>(val)), nullptr);
      ASSERT_EQ(utils_list_len_get(self_list), i);
    }
  }

  void TearDown() override { utils_list_destroy(self_list); }

  void *self_list;
};

/**
 * @brief Test list.
 *
 */
TEST_F(UtilsListTest, list) {
  ASSERT_EQ(utils_list_push(self_list, reinterpret_cast<void *>(1)), nullptr);

  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(utils_list_len_get(self_list), 10 - i);
    ASSERT_EQ(*(int *)(utils_list_pop(self_list)), i);
  }
}

/**
 * @brief Test list iterator.
 *
 */
TEST_F(UtilsListTest, list_iterator) {
  void *iter = utils_list_iterator_create(self_list, LIST_HEAD);
  ASSERT_NE(iter, nullptr);

  void *node = NULL;
  int i = 0;

  while ((node = utils_list_iterator_next(iter))) {
    ASSERT_EQ(utils_list_len_get(self_list), 10 - i);
    ASSERT_EQ(*(int *)(utils_list_get_val(node)), i++);
    utils_list_remove(self_list, node);
  }
  ASSERT_EQ(utils_list_len_get(self_list), 0);
  utils_list_iterator_destroy(iter);
}

/**
 * @brief Test log.
 *
 */
TEST(UtilsLogTest, log) {
  LogHandleFunc func;
  func.log_malloc = HAL_Malloc;
  func.log_free = HAL_Free;
  func.log_get_current_time_str = HAL_Timer_Current;
  func.log_printf = HAL_Printf;
  func.log_handle = NULL;
  ASSERT_EQ(utils_log_init(func, LOG_LEVEL_DEBUG, 2048), 0);
  Log_d("Here is a debug level log test!");
  Log_i("Here is a info level log test!");
  Log_w("Here is a warning level log test!");
  Log_e("Here is a error level log test!");
  utils_log_deinit();
}

/**
 * @brief Test json.
 *
 */
TEST(UtilsJsonTest, json) {
  char test_json[] =
      "{\"str_test\":\"test\",\"int_test\":100,\"float_test\":1.210f,\"bool_test\":true,"
      "\"bool_false_test\":false,\"null_test\":null}";

  UtilsJsonValue value;

  int data_int = 0;
  ASSERT_EQ(utils_json_value_get("int_test", strlen("int_test"), test_json, strlen(test_json), &value), 0);
  ASSERT_EQ(utils_json_value_data_get(value, UTILS_JSON_VALUE_TYPE_INT32, &data_int), 0);
  ASSERT_EQ(data_int, 100);

  float data_float = 0.0;
  ASSERT_EQ(utils_json_value_get("float_test", strlen("float_test"), test_json, strlen(test_json), &value), 0);
  ASSERT_EQ(utils_json_value_data_get(value, UTILS_JSON_VALUE_TYPE_FLOAT, &data_float), 0);
  ASSERT_EQ(data_float, 1.210f);

  ASSERT_EQ(utils_json_value_get("bool_test", strlen("bool_test"), test_json, strlen(test_json), &value), 0);
  ASSERT_EQ(strncmp(value.value, "true", value.value_len), 0);

  ASSERT_EQ(utils_json_value_get("bool_false_test", strlen("bool_false_test"), test_json, strlen(test_json), &value),
            0);
  ASSERT_EQ(strncmp(value.value, "false", value.value_len), 0);

  ASSERT_EQ(utils_json_value_get("null_test", strlen("null_test"), test_json, strlen(test_json), &value), 0);
  ASSERT_EQ(strncmp(value.value, "null", value.value_len), 0);

  ASSERT_EQ(utils_json_value_get("str_test", strlen("str_test"), test_json, strlen(test_json), &value), 0);
  ASSERT_EQ(strncmp(value.value, "test", value.value_len), 0);

  char test_json_depth[] = "{\"depth_test\": {\"test\":\"test1\"}}";
  ASSERT_EQ(utils_json_value_get("depth_test.test", strlen("depth_test.test"), test_json_depth, strlen(test_json_depth),
                                 &value),
            0);
  ASSERT_EQ(strncmp(value.value, "test1", value.value_len), 0);
}

}  // namespace utils_unittest
