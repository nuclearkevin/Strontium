INCLUDE_FLAGS      := -I ./include -I ./vendor -I ./vendor/glm -I ./vendor/glad/include -I ./vendor/entt/include -I ./vendor/glfw/include/ -I ./vendor/assimp/include -I ./vendor/yamlcpp/include
COMPILE_FLAGS      := g++ -g $(INCLUDE_FLAGS) -std=c++17
GLFW_FLAGS         := -pthread ./vendor/glfw/src/libglfw3.a
ASSIMP_FLAGS       := ./vendor/assimp/lib/libassimp.a -lz

SRC_DIR 			     := ./src/
GLAD_DIR           := ./vendor/glad/src/
IMGUI_DIR				   := ./vendor/imgui/
IMGUI_FB_DIR			 := ./vendor/imguibrowser/FileBrowser/
IMGUIZMO_DIR			 := ./vendor/imguizmo/
YAML_DIR_ONE       := ./vendor/yamlcpp/src/yaml-cpp/
YAML_DIR_TWO       := $(YAML_DIR_ONE)contrib/
OUTPUT_DIR         := ./bin/

# Praise be to the wildcards! Preps to shotgun compile the engine source +
# submodules.
CORE_SOURCES 			 := $(wildcard $(SRC_DIR)Core/*.cpp)
GRAPHICS_SOURCES   := $(wildcard $(SRC_DIR)Graphics/*.cpp)
UTILS_SOURCES      := $(wildcard $(SRC_DIR)Utils/*.cpp)
LAYERS_SOURCES     := $(wildcard $(SRC_DIR)Layers/*.cpp)
SCENE_SOURCES      := $(wildcard $(SRC_DIR)Scenes/*.cpp)
GUI_SOURSES        := $(wildcard $(SRC_DIR)GuiElements/*.cpp)
SERIAL_SOURCES     := $(wildcard $(SRC_DIR)Serialization/*.cpp)
GLAD_SOURCES       := $(wildcard $(GLAD_DIR)*.c)
IMGUI_SOURCES 	   := $(wildcard $(IMGUI_DIR)*.cpp)
IMGUI_FB_SOURCES   := $(wildcard $(IMGUI_FB_DIR)*.cpp)
IMGUIZMO_SOURCES   := $(wildcard $(IMGUIZMO_DIR)*.cpp)
YAML_SOURCES_ONE   := $(wildcard $(YAML_DIR_ONE)*.cpp)
YAML_SOURCES_TWO   := $(wildcard $(YAML_DIR_TWO)*.cpp)

CORE_OBJECTS 			 := $(patsubst $(SRC_DIR)Core/%.cpp, $(OUTPUT_DIR)Core/%.o, $(CORE_SOURCES))
GRAPHICS_OBJECTS   := $(patsubst $(SRC_DIR)Graphics/%.cpp, $(OUTPUT_DIR)Graphics/%.o, $(GRAPHICS_SOURCES))
UTILS_OBJECTS      := $(patsubst $(SRC_DIR)Utils/%.cpp, $(OUTPUT_DIR)Utils/%.o, $(UTILS_SOURCES))
LAYERS_OBJECTS     := $(patsubst $(SRC_DIR)Layers/%.cpp, $(OUTPUT_DIR)Layers/%.o, $(LAYERS_SOURCES))
SCENE_OBJECTS      := $(patsubst $(SRC_DIR)Scenes/%.cpp, $(OUTPUT_DIR)Scenes/%.o, $(SCENE_SOURCES))
GUI_OBJECTS        := $(patsubst $(SRC_DIR)GuiElements/%.cpp, $(OUTPUT_DIR)GuiElements/%.o, $(GUI_SOURSES))
SERIAL_OBJECTS     := $(patsubst $(SRC_DIR)Serialization/%.cpp, $(OUTPUT_DIR)Serialization/%.o, $(SERIAL_SOURCES))
GLAD_OBJECTS       := $(patsubst $(GLAD_DIR)%.c, $(OUTPUT_DIR)vendor/%.o, $(GLAD_SOURCES))
IMGUI_OBJECTS      := $(patsubst $(IMGUI_DIR)%.cpp, $(OUTPUT_DIR)vendor/%.o, $(IMGUI_SOURCES))
IMGUI_FB_OBJECTS   := $(patsubst $(IMGUI_FB_DIR)%.cpp, $(OUTPUT_DIR)vendor/%.o, $(IMGUI_FB_SOURCES))
IMGUIZMO_OBJECTS   := $(patsubst $(IMGUIZMO_DIR)%.cpp, $(OUTPUT_DIR)vendor/%.o, $(IMGUIZMO_SOURCES))
YAML_OBJECTS_ONE   := $(patsubst $(YAML_DIR_ONE)%.cpp, $(OUTPUT_DIR)vendor/%.o, $(YAML_SOURCES_ONE))
YAML_SOURCES_TWO   := $(patsubst $(YAML_DIR_TWO)%.cpp, $(OUTPUT_DIR)vendor/%.o, $(YAML_SOURCES_TWO))

makebuild: make_dir Application

# Link everything together.
Application: $(CORE_OBJECTS) $(LAYERS_OBJECTS) $(GRAPHICS_OBJECTS) $(SCENE_OBJECTS) \
	$(UTILS_OBJECTS) $(GUI_OBJECTS) $(GLAD_OBJECTS) $(IMGUI_OBJECTS) $(IMGUI_FB_OBJECTS) \
  $(IMGUIZMO_OBJECTS) $(YAML_OBJECTS_ONE) $(YAML_OBJECTS_TWO) $(SERIAL_OBJECTS)
	@echo Linking the application.
	@$(COMPILE_FLAGS) -o Application $^ -ldl $(GLFW_FLAGS) $(ASSIMP_FLAGS)

# Compile the graphics engine.
$(OUTPUT_DIR)%.o: $(SRC_DIR)%.cpp
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(SRC_DIR) -c $< -o $@

# Compile GLAD.
$(OUTPUT_DIR)vendor/%.o: $(GLAD_DIR)%.c
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(GLAD_DIR) -c $< -o $@

# Compile Dear Imgui. Had to manually define GLAD and make a few modifications.
$(OUTPUT_DIR)vendor/%.o: $(IMGUI_DIR)%.cpp
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(IMGUI_DIR) -c $< -o $@

# Compile the Imgui file browser.
$(OUTPUT_DIR)vendor/%.o: $(IMGUI_FB_DIR)%.cpp
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(IMGUI_FB_DIR) -c $< -o $@

# Compile the ImGuizmos.
$(OUTPUT_DIR)vendor/%.o: $(IMGUIZMO_DIR)%.cpp
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(IMGUIZMO_DIR) -c $< -o $@

# Compile yaml-cpp.
$(OUTPUT_DIR)vendor/%.o: $(YAML_DIR_ONE)%.cpp
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(YAML_DIR_ONE) -c $< -o $@

$(OUTPUT_DIR)vendor/%.o: $(YAML_SOURCES_TWO)%.cpp
	@echo Compiling $<
	@$(COMPILE_FLAGS) -I$(YAML_SOURCES_TWO) -c $< -o $@

# Make the binary directory.
make_dir:
	@mkdir -p bin
	@mkdir -p bin/Core
	@mkdir -p bin/Graphics
	@mkdir -p bin/Utils
	@mkdir -p bin/Layers
	@mkdir -p bin/Scenes
	@mkdir -p bin/GuiElements
	@mkdir -p bin/Serialization
	@mkdir -p bin/vendor

# Delete the contents of bin.
clean:
	@echo Clearing the binary directories.
	@rm ./bin/Core/*.o
	@rm ./bin/Utils/*.o
	@rm ./bin/Graphics/*.o
	@rm ./bin/Layers/*.o
	@rm ./bin/Scenes/*.o
	@rm ./bin/GuiElements/*.o
	@rm ./bin/Serialization/*.o
