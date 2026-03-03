cmake_minimum_required(VERSION 3.20)

add_executable(test-enc_grain "test_enc_grain.c")

target_link_libraries(test-enc_grain enc_wrapper::enc_wrapper)
target_link_libraries(test-enc_grain enc_io)
