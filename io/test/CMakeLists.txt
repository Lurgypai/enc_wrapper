cmake_minimum_required(VERSION 3.20)

add_executable(test-enc_grain "test_enc_grain.c")
target_link_libraries(test-enc_grain enc_io)

add_executable(test-enc_grain_index "test_enc_grain_index.c")
target_link_libraries(test-enc_grain_index enc_io)

add_executable(test-enc_object "test_enc_object.c")
target_link_libraries(test-enc_object enc_io)

add_executable(test-enc_store_test "test_enc_store_test.c")
target_link_libraries(test-enc_store_test enc_io)

add_executable(test-enc_store "test_enc_store.c")
target_link_libraries(test-enc_store enc_io)

