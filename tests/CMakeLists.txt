add_executable(flow-tests
    store.test.cpp
)

target_include_directories(flow-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)