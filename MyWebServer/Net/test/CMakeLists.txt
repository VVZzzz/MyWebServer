add_executable(Looptest looptest.cpp)
target_link_libraries(Looptest libserver_base)

add_executable(Polltest polltest.cpp)
target_link_libraries(Polltest libserver_base)

add_executable(HttpClient HttpClient.cpp)