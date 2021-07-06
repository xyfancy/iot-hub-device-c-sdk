file(GLOB src 
    ${CMAKE_CURRENT_SOURCE_DIR}/network/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/os/${PLATFORM}/*.c
)
set(inc ${CMAKE_CURRENT_SOURCE_DIR}/network)

set(src_platform ${src_platform} ${src} PARENT_SCOPE)
set(inc_platform ${inc_platform} ${inc} PARENT_SCOPE)
