cmake_minimum_required(VERSION 3.20)

add_executable(test-enc_grain "test_enc_grain.c")
target_link_libraries(test-enc_grain enc_io)

add_executable(test-enc_object "test_enc_object.c")
target_link_libraries(test-enc_object enc_io)
