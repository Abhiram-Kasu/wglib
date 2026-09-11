add_executable(wglib_api_smoke api_smoke.cpp)
target_link_libraries(wglib_api_smoke PRIVATE wglib::wglib)

add_test(NAME wglib_api_smoke COMMAND wglib_api_smoke)
