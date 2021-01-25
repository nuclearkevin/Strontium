CXXFLAGS        = g++ -Wall
COMPILE_FLAGS   = g++ -Wall -I ./include
OUTPUT_DIR_FLAG = -o ./bin/
OUTPUT_DIR      = ./bin/
OGL_FLAGS   		= -lglfw -lGLEW -lGLU -lGL

makebuild: make_dir app

app: Application.o Shaders.o Meshs.o Buffers.o Camera.o
	$(CXXFLAGS) -o ./Application $(OUTPUT_DIR)Application.o $(OUTPUT_DIR)Shaders.o $(OUTPUT_DIR)Meshs.o $(OUTPUT_DIR)Buffers.o $(OUTPUT_DIR)Camera.o $(OGL_FLAGS)

Application.o:
	$(COMPILE_FLAGS) -c ./src/Application.cpp $(OUTPUT_DIR_FLAG)Application.o

Shaders.o:
	$(COMPILE_FLAGS) -c ./src/Shaders.cpp $(OUTPUT_DIR_FLAG)Shaders.o

Meshs.o:
	$(COMPILE_FLAGS) -c ./src/Meshes.cpp $(OUTPUT_DIR_FLAG)Meshs.o

Buffers.o:
	$(COMPILE_FLAGS) -c ./src/Buffers.cpp $(OUTPUT_DIR_FLAG)Buffers.o

Camera.o:
	$(COMPILE_FLAGS) -c ./src/Camera.cpp $(OUTPUT_DIR_FLAG)Camera.o

make_dir:
	mkdir -p bin

clean:
	rm ./bin/*.o
