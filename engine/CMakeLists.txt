include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    include/*.h
    vendor/glad/*.c
    vendor/glad/*.h
    vendor/imguizmo/ImGuizmo.cpp
    vendor/imguizmo/ImGuizmo.h
    vendor/imguibrowser/FileBrowser/ImGuiFileBrowser.cpp
    vendor/imguibrowser/FileBrowser/ImGuiFileBrowser.h
)

set(INCLUDE_DIRS
    include
    vendor
    vendor/robin-hood/include
    vendor/glm
    vendor/glad/include
    vendor/entt/include
    vendor/glfw/include
    vendor/assimp/include
    vendor/yamlcpp/include
    vendor/joltphysics
)

set(LIB_LINKS
    ImGui
    glfw
    assimp
    yamlcpp
    jolt
)

add_library(Strontium ${SOURCE_FILES})
target_include_directories(Strontium PRIVATE ${INCLUDE_DIRS})
target_link_libraries(Strontium PUBLIC ${LIB_LINKS})

GroupSourcesByFolder(Strontium)

set_property(TARGET Strontium PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/engine)
set_target_properties(Strontium PROPERTIES FOLDER engine)

if (WIN32)
    target_compile_definitions(Strontium
    PUBLIC
    "_CRT_SECURE_NO_WARNINGS"
    "GLM_FORCE_RADIANS"
    "SR_WINDOWS"
    ${PLATFORM_COMPILE_DEFS}

    $<$<CONFIG:Debug>:SR_DEBUG>
    $<$<CONFIG:Release>:SR_RELEASE>
    )
endif (WIN32)

if (UNIX)
    target_compile_definitions(Strontium
    PUBLIC
    "_CRT_SECURE_NO_WARNINGS"
    "GLM_FORCE_RADIANS"
    "SR_UNIX"
    ${PLATFORM_COMPILE_DEFS}

    $<$<CONFIG:Debug>:SR_DEBUG>
    $<$<CONFIG:Release>:SR_RELEASE>
    )
endif (UNIX)
