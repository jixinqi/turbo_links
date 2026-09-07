cmake_minimum_required(VERSION 3.16)

project(turbo_links VERSION 0.1.0 LANGUAGES CXX)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(turbo_links
    "src/main.cpp"
    "src/scan/scanner_packet_udp_ping.cpp"
    "src/scan/scanner_sender_udp.cpp"
)

set(turbo_links_targets turbo_links)


foreach(turbo_links_target IN LISTS turbo_links_targets)
    target_compile_features(${turbo_links_target} PRIVATE cxx_std_17)
    set_target_properties(${turbo_links_target} PROPERTIES CXX_EXTENSIONS OFF)

    target_include_directories(${turbo_links_target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_link_libraries(${turbo_links_target} PRIVATE
        Boost::system
        Threads::Threads
    )

    if(WIN32)
        target_compile_definitions(${turbo_links_target} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _WIN32_WINNT=0x0A00
        )
        target_link_libraries(${turbo_links_target} PRIVATE ws2_32)
    endif()

    if(MSVC)
        target_compile_options(${turbo_links_target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${turbo_links_target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
