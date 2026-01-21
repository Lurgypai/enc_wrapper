cmake_minimum_required(VERSION 3.20)

project(encryption_wrapper)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(ENC_WRAPPER_ENABLE_GCRYPT "Enable gcrypt library support." ON)
option(ENC_WRAPPER_ENABLE_NETTLE "Enable nettle library support." ON)

set(ENC_WRAPPER_SRC "enc_wrapper.c")
set(ENC_WRAPPER_LIBS)
set(ENC_WRAPPER_HEADERS "enc_wrapper.h")

# ENABLE GCRYPT
if(ENC_WRAPPER_ENABLE_GCRYPT)
    find_package(GCrypt REQUIRED)
    find_package(GPGError REQUIRED)

    list(APPEND ENC_WRAPPER_SRC "gcrypt_impl/enc_gcrypt.c")
    list(APPEND ENC_WRAPPER_LIBS GCrypt::GCrypt GPGError::GPGError)
    list(APPEND ENC_WRAPPER_HEADERS "gcrypt_impl/enc_gcrypt.h")
    message(STATUS "gcrypt enabled")
else()
    message(STATUS "gcrypt disabled")
endif()

# ENABLE NETTLE
if(ENC_WRAPPER_ENABLE_NETTLE)
    find_package(Nettle REQUIRED)

    list(APPEND ENC_WRAPPER_SRC "nettle_impl/enc_nettle.c")
    list(APPEND ENC_WRAPPER_LIBS Nettle::Nettle)
    list(APPEND ENC_WRAPPER_HEADERS "nettle_impl/enc_nettle.h")
    message(STATUS "nettle enabled")
else()
    message(STATUS "nettle disabled")
endif()

#ADD LIBRARY
add_library(enc_wrapper STATIC ${ENC_WRAPPER_SRC})
if(ENC_WRAPPER_LIBS)
    target_link_libraries(enc_wrapper PUBLIC ${ENC_WRAPPER_LIBS})
endif()

# GENERATE PUBLIC HEADERS
set(ENC_WRAPPER_PUBLIC_INCLUDES "${CMAKE_SOURCE_DIR}/include")
file(REMOVE_RECURSE ${ENC_WRAPPER_PUBLIC_INCLUDES})
add_custom_target(make_include_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ENC_WRAPPER_PUBLIC_INCLUDES})
add_custom_target(copy_to_include_dir ALL
    COMMAND ${CMAKE_COMMAND} -E copy
    ${ENC_WRAPPER_PUBLIC_INCLUDES}
    ${ENC_WRAPPER_INCLUDE_INSTALL_DESTINATION}
)

set(ENC_WRAPPER_INCLUDE_INSTALL_DESTINATION include/enc_wrapper)

# need to provide location in installation dir and build dir separately here
target_include_directories(enc_wrapper PUBLIC
    $<BUILD_INTERFACE:include>
    $<INSTALL_INTERFACE:${ENC_WRAPPER_INCLUDE_INSTALL_DESTINATION}>)

install(TARGETS enc_wrapper EXPORT enc_wrapper
    ARCHIVE DESTINATION lib # static and import libs installed to lib
    LIBRARY DESTINATION lib # .so installed to lib
    RUNTIME DESTINATION bin # .dlls installed to bin
    )

install(DIRECTORY include DESTINATION ${MYLB_INCLUDE_INSTALL_DESTINATION}
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DIRECTORY_PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


# create cmake config to lib/cmake/enc_wrapperConfig.cmake
install(EXPORT enc_wrapper
    DESTINATION lib/cmake
    FILE enc_wrapperConfig.cmake)
