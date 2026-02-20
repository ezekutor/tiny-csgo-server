cmake_minimum_required(VERSION 3.23)
project(tiny-csgo-server)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

set(HL2SDK_DIR "" CACHE PATH "Path to hl2sdk (cs2 branch)")
set(ASIO_SRC "" CACHE PATH "Path to standalone Asio")

if(NOT HL2SDK_DIR)
    message(FATAL_ERROR "Please set HL2SDK_DIR to your hl2sdk cs2 checkout path")
endif()

if(NOT ASIO_SRC)
    message(FATAL_ERROR "Please set ASIO_SRC to your Asio checkout path")
endif()

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${HL2SDK_DIR}/common/protobuf-2.5.0/src
    ${HL2SDK_DIR}/public/steam
    ${ASIO_SRC}/include
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/bitbuf BITBUF_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/netmessage NETMSG_SRC)
set(SRC_LIST sv-main.cpp ${NETMSG_SRC} ${BITBUF_SRC})

if(MSVC)
    add_definitions(
        -D _WIN32_WINNT=0x0601
        -D _CRT_SECURE_NO_WARNINGS
        -D _GENERIC
        -D COMPILER_MSVC
        -D COMPILER_MSVC64
        -D _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
    )

    set(STEAM_API_LIB "${HL2SDK_DIR}/lib/x64/steam_api64.lib")
    if(NOT EXISTS ${STEAM_API_LIB})
        set(STEAM_API_LIB "${HL2SDK_DIR}/lib/public/steam_api64.lib")
    endif()

    set(PROTOBUF_LIB "${HL2SDK_DIR}/lib/x64/release/libprotobuf.lib")
    if(NOT EXISTS ${PROTOBUF_LIB})
        set(PROTOBUF_LIB "${HL2SDK_DIR}/lib/win64/release/vs2022/libprotobuf.lib")
    endif()

    set(STATIC_LIB_LIST ${STEAM_API_LIB} ${PROTOBUF_LIB})

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_definitions(
        -D GNUC
        -D POSIX
        -D COMPILER_GCC
    )

    set(STATIC_LIB_LIST
        ${HL2SDK_DIR}/lib/linux64/release/libprotobuf.a
        ${HL2SDK_DIR}/lib/linux64/libsteam_api.so
        pthread
    )

    add_compile_options(
        -Wno-volatile
        -Wno-format-security
        -Wno-register
        -fcoroutines
    )
endif()

add_executable(tiny-csgo-server ${SRC_LIST})
target_link_libraries(tiny-csgo-server ${STATIC_LIB_LIST})

if(MSVC)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT tiny-csgo-server)
endif()
