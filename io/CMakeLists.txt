cmake_minimum_required(VERSION 3.20)

project(encryption_io)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(ENC_IO_SRC "./src/enc_grain.c" "./src/enc_object.c" "./src/enc_store_test.c" "./src/enc_store.c")
set(ENC_IO_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(enc_io STATIC ${ENC_IO_SRC})
target_sources(enc_io PRIVATE
    FILE_SET HEADERS FILES
    "${ENC_IO_INCLUDE}/enc_grain.h"
    "${ENC_IO_INCLUDE}/enc_object.h"
    "${ENC_IO_INCLUDE}/enc_store_test.h"
    "${ENC_IO_INCLUDE}/enc_store.h"
)

target_include_directories(enc_io PUBLIC
    $<BUILD_INTERFACE:${ENC_IO_INCLUDE}>
    $<INSTALL_INTERFACE:include>
)

find_package(enc_wrapper REQUIRED)
target_link_libraries(enc_io PUBLIC enc_wrapper::enc_wrapper)

install(TARGETS enc_io EXPORT enc_ioTargets
    ARCHIVE DESTINATION lib # static and import libs installed to lib
    LIBRARY DESTINATION lib # .so installed to lib
    RUNTIME DESTINATION bin # .dlls installed to bin
    )

install(DIRECTORY "${ENC_IO_INCLUDE}/" DESTINATION include
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE GROUP_WRITE WORLD_WRITE
    DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE GROUP_WRITE WORLD_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
)

include(CMakePackageConfigHelpers)

# generate config
configure_package_config_file(
    cmake/enc_ioConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/enc_ioConfig.cmake"
    INSTALL_DESTINATION cmake
)

# install config
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/enc_ioConfig.cmake"
    DESTINATION cmake
)

# install remaining (binary)
install(
    EXPORT enc_ioTargets
    NAMESPACE enc_wrapper::
    DESTINATION cmake
)

# tests
add_subdirectory(test)
