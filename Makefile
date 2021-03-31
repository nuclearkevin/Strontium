CXXFLAGS        = g++ -Wall
COMPILE_FLAGS   = g++ -Wall -I ./include
OUTPUT_DIR_FLAG = -o ./bin/
OUTPUT_DIR      = ./bin/
OGL_FLAGS   		= -lglfw -lGLEW -lGLU -lGL

makebuild: make_dir app

app: Application.o Shaders.o VertexArray.o Buffers.o Renderer.o Meshs.o Camera.o Lighting.o Textures.o GuiHandler.o GUI.o
	 $(CXXFLAGS) -o ./Application $(OUTPUT_DIR)Application.o $(OUTPUT_DIR)Shaders.o\
	 $(OUTPUT_DIR)VertexArray.o $(OUTPUT_DIR)Buffers.o $(OUTPUT_DIR)Renderer.o \
	 $(OUTPUT_DIR)Meshs.o $(OUTPUT_DIR)Camera.o $(OUTPUT_DIR)Lighting.o \
	 $(OUTPUT_DIR)Textures.o $(OUTPUT_DIR)GuiHandler.o $(OUTPUT_DIR)GUI.o $(OGL_FLAGS)

Application.o:
	$(COMPILE_FLAGS) -c ./src/Application.cpp $(OUTPUT_DIR_FLAG)Application.o

Shaders.o:
	$(COMPILE_FLAGS) -c ./src/Shaders.cpp $(OUTPUT_DIR_FLAG)Shaders.o

Buffers.o:
	$(COMPILE_FLAGS) -c ./src/Buffers.cpp $(OUTPUT_DIR_FLAG)Buffers.o

VertexArray.o:
	$(COMPILE_FLAGS) -c ./src/VertexArray.cpp $(OUTPUT_DIR_FLAG)VertexArray.o

Renderer.o:
	$(COMPILE_FLAGS) -c ./src/Renderer.cpp $(OUTPUT_DIR_FLAG)Renderer.o

Meshs.o:
	$(COMPILE_FLAGS) -c ./src/Meshes.cpp $(OUTPUT_DIR_FLAG)Meshs.o

Camera.o:
	$(COMPILE_FLAGS) -c ./src/Camera.cpp $(OUTPUT_DIR_FLAG)Camera.o

GuiHandler.o:
	$(COMPILE_FLAGS) -c ./src/GuiHandler.cpp $(OUTPUT_DIR_FLAG)GuiHandler.o

Lighting.o:
	$(COMPILE_FLAGS) -c ./src/Lighting.cpp $(OUTPUT_DIR_FLAG)Lighting.o

Textures.o:
	$(COMPILE_FLAGS) -c ./src/Textures.cpp $(OUTPUT_DIR_FLAG)Textures.o

GUI.o:
	$(COMPILE_FLAGS) -c ./src/imgui/imgui.cpp $(OUTPUT_DIR_FLAG)a.o
	$(COMPILE_FLAGS) -c ./src/imgui/imgui_draw.cpp $(OUTPUT_DIR_FLAG)b.o
	$(COMPILE_FLAGS) -c ./src/imgui/imgui_impl_glfw.cpp $(OUTPUT_DIR_FLAG)c.o
	$(COMPILE_FLAGS) -c ./src/imgui/imgui_tables.cpp $(OUTPUT_DIR_FLAG)d.o
	$(COMPILE_FLAGS) -c ./src/imgui/imgui_widgets.cpp $(OUTPUT_DIR_FLAG)e.o
	$(COMPILE_FLAGS) -c ./src/imgui/imgui_impl_opengl3.cpp $(OUTPUT_DIR_FLAG)f.o
	$(COMPILE_FLAGS) -c ./src/imgui/ImGuiFileBrowser.cpp $(OUTPUT_DIR_FLAG)g.o
	ld -relocatable $(OUTPUT_DIR)a.o $(OUTPUT_DIR)b.o $(OUTPUT_DIR)c.o\
	 $(OUTPUT_DIR)d.o $(OUTPUT_DIR)e.o $(OUTPUT_DIR)f.o $(OUTPUT_DIR)g.o\
	 $(OUTPUT_DIR_FLAG)GUI.o

make_dir:
	mkdir -p bin

clean:
	rm ./bin/*.o
