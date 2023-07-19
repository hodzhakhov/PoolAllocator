add_executable(lab9 main.cpp)

target_link_libraries(lab9 PoolsAllocator)
target_include_directories(lab9 PRIVATE ${PROJECT_SOURCE_DIR})
