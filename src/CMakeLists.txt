add_executable(redis-server
    main.cpp
    server/Server.cpp
    resp/Decoder.cpp
    resp/Encoder.cpp
    store/Store.cpp
    commands/Dispatcher.cpp
)

target_include_directories(redis-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})