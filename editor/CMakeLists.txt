include(${CMAKE_SOURCE_DIR}/scripts/CMakeUtils.cmake)

file(GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    include/*.h
)

set(INCLUDE_DIRS
    include
    ../engine/include
    ../engine/vendor
    ../engine/vendor/glm
    ../engine/vendor/entt/include
)

set(LIB_LINKS
    ImGui
    Strontium
)

add_executable(StrontiumEditor ${SOURCE_FILES})
target_include_directories(StrontiumEditor PRIVATE ${INCLUDE_DIRS})
target_link_libraries(StrontiumEditor PUBLIC ${LIB_LINKS})

GroupSourcesByFolder(StrontiumEditor)

set_property(TARGET StrontiumEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(StrontiumEditor PROPERTIES FOLDER editor)

target_compile_definitions(StrontiumEditor
    PUBLIC
    "_CRT_SECURE_NO_WARNINGS"
    "GLM_FORCE_RADIANS"
    ${PLATFORM_COMPILE_DEFS}

    $<$<CONFIG:Debug>:SR_DEBUG>
    $<$<CONFIG:Release>:SR_RELEASE>
)
