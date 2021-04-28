COMPILE_FLAGS   := g++ -Wall -I ./include
OGL_FLAGS   		:= -lglfw -lGLEW -lGLU -lGL

SRC_DIR 			  := ./src/
IMGUI_DIR				:= ./src/imgui/
OUTPUT_DIR      := ./bin/

SOURCES 				:= $(wildcard $(SRC_DIR)*.cpp)
IMGUI_SOURCES 	:= $(wildcard $(IMGUI_DIR)*.cpp)
OBJECTS 				:= $(patsubst $(SRC_DIR)%.cpp, $(OUTPUT_DIR)%.o, $(SOURCES))
IMGUI_OBJECTS   := $(patsubst $(IMGUI_DIR)%.cpp, $(OUTPUT_DIR)%.o, $(IMGUI_SOURCES))

makebuild: make_dir Application

Application: $(OBJECTS) $(IMGUI_OBJECTS)
	$(COMPILE_FLAGS) $^ -o $@ $(OGL_FLAGS)

$(OUTPUT_DIR)%.o: $(SRC_DIR)%.cpp
	$(COMPILE_FLAGS) -I$(SRC_DIR) -c $< -o $@

$(OUTPUT_DIR)%.o: $(IMGUI_DIR)%.cpp
	$(COMPILE_FLAGS) -I$(IMGUI_DIR) -c $< -o $@

make_dir:
	mkdir -p bin

clean:
	rm ./bin/*.o
