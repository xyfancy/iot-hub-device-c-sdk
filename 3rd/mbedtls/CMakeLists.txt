option(USE_STATIC_MBEDTLS_LIBRARY "Build mbed TLS static library." ON)
option(USE_SHARED_MBEDTLS_LIBRARY "Build mbed TLS shared library." OFF)
option(LINK_WITH_PTHREAD "Explicitly link mbed TLS library to pthread." OFF)

file(GLOB src_mbedtls  
    ${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/library/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port/src/*.c
)
add_library(mbedtls STATIC ${src_mbedtls})
