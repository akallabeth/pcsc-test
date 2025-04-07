cmake_minimum_required(VERSION 3.16)

project(pcsc-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCSC REQUIRED libpcsclite)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} ${PCSC_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PCSC_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PUBLIC ${PCSC_CFLAGS_OTHER})

include(GNUInstallDirs)
install(TARGETS pcsc-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
