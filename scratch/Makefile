BUILD_SCRIPT = ./obf.sh
BUILD_DIR = build
OBF_BUILD = build/projects/LLVM-Obfuscator/Release+Asserts

CPP = $(BUILD_DIR)/Release+Asserts/bin/clang++
CPP_FLAGS = -O3 -Wall -std=c++11
CPP_OBF = -Xclang -load -Xclang ${OBF_BUILD}/lib/LLVMObfuscatorTransforms.so

test/get_input.o: get_input.cpp get_input.h
	$(CPP) $(CPP_FLAGS) -c -o test/get_input.o get_input.cpp
