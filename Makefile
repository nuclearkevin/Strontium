INCLUDE_FLAGS      := -I ./include -I ./vendor -I ./vendor/glm -I ./vendor/glad/include
COMPILE_FLAGS      := g++ -Wall $(INCLUDE_FLAGS)

SRC_DIR 			     := ./src/
GLAD_DIR           := ./vendor/glad/src/
IMGUI_DIR				   := ./vendor/imgui/
IMGUI_FB_DIR			 := ./vendor/imguibrowser/FileBrowser/
OUTPUT_DIR         := ./bin/

# Praise be to the wildcards! Preps to shotgun compiles the engine source +
# submodules.
SOURCES 				   := $(wildcard $(SRC_DIR)*.cpp)
GLAD_SOURCES       := $(wildcard $(GLAD_DIR)*.c)
IMGUI_SOURCES 	   := $(wildcard $(IMGUI_DIR)*.cpp)
IMGUI_FB_SOURCES   := $(wildcard $(IMGUI_FB_DIR)*.cpp)
OBJECTS 				   := $(patsubst $(SRC_DIR)%.cpp, $(OUTPUT_DIR)%.o, $(SOURCES))
GLAD_OBJECTS       := $(patsubst $(GLAD_DIR)%.c, $(OUTPUT_DIR)%.o, $(GLAD_SOURCES))
IMGUI_OBJECTS      := $(patsubst $(IMGUI_DIR)%.cpp, $(OUTPUT_DIR)%.o, $(IMGUI_SOURCES))
IMGUI_FB_OBJECTS   := $(patsubst $(IMGUI_FB_DIR)%.cpp, $(OUTPUT_DIR)%.o, $(IMGUI_FB_SOURCES))

makebuild: make_dir Application

# Link everything together.
Application: $(OBJECTS) $(GLAD_OBJECTS) $(IMGUI_OBJECTS) $(IMGUI_FB_OBJECTS)
	$(COMPILE_FLAGS) $^ -o $@ -ldl -lglfw

# Compile the graphics engine.
$(OUTPUT_DIR)%.o: $(SRC_DIR)%.cpp
	$(COMPILE_FLAGS) -I$(SRC_DIR) -c $< -o $@

# Compile GLAD.
$(OUTPUT_DIR)%.o: $(GLAD_DIR)%.c
	$(COMPILE_FLAGS) -I$(GLAD_DIR) -c $< -o $@

# Compile Dear Imgui. Had to manually define GLAD and make a few modifications.
$(OUTPUT_DIR)%.o: $(IMGUI_DIR)%.cpp
	$(COMPILE_FLAGS) -I$(IMGUI_DIR) -c $< -o $@

# Compile the Imgui file browser.
$(OUTPUT_DIR)%.o: $(IMGUI_FB_DIR)%.cpp
	$(COMPILE_FLAGS) -I$(IMGUI_FB_DIR) -c $< -o $@

# Make the binary directory.
make_dir:
	mkdir -p bin

# Delete the contents of bin.
clean:
	rm ./bin/*.o
