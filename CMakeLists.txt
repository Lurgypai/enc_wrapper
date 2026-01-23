cmake_minimum_required(VERSION 3.20)

project(encryption_wrapper)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(ENC_WRAPPER_ENABLE_GCRYPT "Enable gcrypt library support." ON)
option(ENC_WRAPPER_ENABLE_NETTLE "Enable nettle library support." ON)

set(ENC_WRAPPER_SRC "enc_wrapper.c")
set(ENC_WRAPPER_LIBS)
set(ENC_WRAPPER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/enc_wrapper.h")

# ENABLE GCRYPT
if(ENC_WRAPPER_ENABLE_GCRYPT)
    find_package(GCrypt REQUIRED)
    find_package(GPGError REQUIRED)

    list(APPEND ENC_WRAPPER_SRC "gcrypt_impl/enc_gcrypt.c")
    list(APPEND ENC_WRAPPER_LIBS GCrypt::GCrypt GPGError::GPGError)
    list(APPEND ENC_WRAPPER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/gcrypt_impl/enc_gcrypt.h")
    message(STATUS "gcrypt enabled")
else()
    message(STATUS "gcrypt disabled")
endif()

# ENABLE NETTLE
if(ENC_WRAPPER_ENABLE_NETTLE)
    find_package(Nettle REQUIRED)

    list(APPEND ENC_WRAPPER_SRC "nettle_impl/enc_nettle.c")
    list(APPEND ENC_WRAPPER_LIBS Nettle::Nettle)
    list(APPEND ENC_WRAPPER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/nettle_impl/enc_nettle.h")
    message(STATUS "nettle enabled")
else()
    message(STATUS "nettle disabled")
endif()

#ADD LIBRARY
add_library(enc_wrapper STATIC ${ENC_WRAPPER_SRC})
if(ENC_WRAPPER_LIBS)
    target_link_libraries(enc_wrapper PRIVATE ${ENC_WRAPPER_LIBS})
endif()

# GENERATE PUBLIC HEADERS
set(ENC_WRAPPER_PUBLIC_INCLUDES "${CMAKE_CURRENT_BINARY_DIR}/include")
# remove old include
file(REMOVE_RECURSE ${ENC_WRAPPER_PUBLIC_INCLUDES})
# generate new include directory
add_custom_target(make_public_include_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ENC_WRAPPER_PUBLIC_INCLUDES}
    COMMAND ${CMAKE_COMMAND} -E copy ${ENC_WRAPPER_HEADERS} ${ENC_WRAPPER_PUBLIC_INCLUDES}
    COMMENT "Generate public include directory"
)

# need to provide location in installation dir and build dir separately here
target_include_directories(enc_wrapper PUBLIC
    $<INSTALL_INTERFACE:include>)

install(TARGETS enc_wrapper EXPORT enc_wrapperTargets
    ARCHIVE DESTINATION lib # static and import libs installed to lib
    LIBRARY DESTINATION lib # .so installed to lib
    RUNTIME DESTINATION bin # .dlls installed to bin
    )

install(DIRECTORY "${ENC_WRAPPER_PUBLIC_INCLUDES}/" DESTINATION include
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE GROUP_WRITE WORLD_WRITE
    DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE GROUP_WRITE WORLD_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
)

include(CMakePackageConfigHelpers)

# generate config
configure_package_config_file(
    cmake/enc_wrapperConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/enc_wrapperConfig.cmake"
    INSTALL_DESTINATION cmake
)

# install config
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/enc_wrapperConfig.cmake"
    cmake/FindGCrypt.cmake
    cmake/FindGPGError.cmake
    cmake/FindNettle.cmake
    DESTINATION cmake
)

# install remaining (binary)
install(
    EXPORT enc_wrapperTargets
    NAMESPACE enc_wrapper::
    DESTINATION cmake
)
