protoc.exe --cpp_out=./ ./server.proto
protoc.exe --grpc_out=./ --plugin=protoc-gen-grpc=grpc_cpp_plugin.exe ./server.proto
protoc.exe --cpp_out=./ ./client.proto
protoc.exe --grpc_out=./ --plugin=protoc-gen-grpc=grpc_cpp_plugin.exe ./client.proto