protoc --cpp_out=./ ./server.proto
protoc --grpc_out=./ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./server.proto
protoc --cpp_out=./ ./client.proto
protoc --grpc_out=./ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./client.proto