include_directories(
    ${PROJECT_SOURCE_DIR}/Thread
    ${PROJECT_SOURCE_DIR}/Log
    )
#编译以下文件夹
add_subdirectory(Log)
add_subdirectory(Thread)
# 生成libserver_base库(log,thread的一些函数)
set(LIB_SRC 
    ${PROJECT_SOURCE_DIR}/MyWebServer/Thread/Thread.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Thread/CountDownLatch.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Log/AsyncLogging.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Log/FileUtil.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Log/LogFile.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Log/Logging.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Log/LogStream.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/EventLoop.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/Poller.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/Channel.cpp
    ##${PROJECT_SOURCE_DIR}/MyWebServer/Net/PollPoller.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/Util.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/EventLoopThread.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/EventLoopThreadPool.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/HttpData.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/Server.cpp
    
)
add_library(libserver_base ${LIB_SRC})
target_link_libraries(libserver_base pthread rt)
set_target_properties(libserver_base PROPERTIES OUTPUT_NAME "server_base")

#生成webserver
set (NET_SRC 
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/EventLoop.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/Poller.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/Channel.cpp
    #${PROJECT_SOURCE_DIR}/MyWebServer/Net/PollPoller.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/Util.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/EventLoopThread.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/EventLoopThreadPool.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/HttpData.cpp
    ${PROJECT_SOURCE_DIR}/MyWebServer/Net/Server.cpp
)
include_directories(${PROJECT_SOURCE_DIR}/Net)

add_executable(WebServer ${NET_SRC})
target_link_libraries(WebServer libserver_base)
add_subdirectory(Net)