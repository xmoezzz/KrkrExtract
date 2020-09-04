CXX = afl-g++

RM = rm -rf


OBJECTS = TLGDecoder.o tlg_fuzzer.o

tlg_fuzzer : $(OBJECTS) 
		  $(CXX) -o tlg_fuzzer $(OBJECTS) -lasan $(LIBS)

TLGDecoder.o : TLGDecoder.cpp NtToLinux.h TLGDecoder.h
		  $(CXX) -c -DFUZZ_ME -std=c++11 -fsanitize=address -static-libasan -fno-omit-frame-pointer TLGDecoder.cpp

tlg_fuzzer.o : tlg_fuzzer.cpp NtToLinux.h TLGDecoder.h
		  $(CXX) -c -DFUZZ_ME -std=c++11 -fsanitize=address -static-libasan -fno-omit-frame-pointer tlg_fuzzer.cpp

clean:
	rm tlg_fuzzer $(OBJECTS)
	 
