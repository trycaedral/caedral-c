cmake_minimum_required(VERSION 3.16)
project(caedral-c VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_library(caedral STATIC
    src/caedral.c
    third_party/cJSON.c
)

target_include_directories(caedral PUBLIC include PRIVATE src third_party)
target_link_libraries(caedral PUBLIC CURL::libcurl)

add_executable(caedral_quickstart examples/quickstart.c)
target_link_libraries(caedral_quickstart PRIVATE caedral)

add_executable(caedral_test_integration tests/test_integration.c)
target_link_libraries(caedral_test_integration PRIVATE caedral)

enable_testing()
add_test(NAME integration COMMAND caedral_test_integration)
