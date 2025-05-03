cmake_minimum_required(VERSION 3.20)
project(myserver)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_library(protolib proto/order.proto)
target_link_libraries(protolib gRPC::grpc++)
target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET protolib LANGUAGE cpp)
protobuf_generate(TARGET protolib LANGUAGE grpc
GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")


add_executable(server src/server.cpp)
target_link_libraries(server protolib)

# Add the client executable
add_executable(client src/client.cpp src/lb_client.cpp)
target_include_directories(client PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # Include generated Protobuf files
target_link_libraries(client protolib) # Link the Protobuf library

# Add the gateway server executable
add_executable(gateway_server src/gateway_server.cpp)
target_include_directories(gateway_server PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # Include generated Protobuf files
target_link_libraries(gateway_server protolib) # Link the Protobuf library