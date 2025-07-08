cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(PRODUCTION_BUILD OFF CACHE BOOL "Make this a production build" FORCE)
project (DualSenseY)
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${MY_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${MY_SOURCES})
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

if(PRODUCTION_BUILD)
	target_compile_definitions("${PROJECT_NAME}" PUBLIC RESOURCES_PATH="./resources/") 
	target_compile_definitions("${PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=1) 
	target_compile_definitions("${PROJECT_NAME}" PUBLIC DEVELOPLEMT_BUILD=0) 
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources)
else()
	target_compile_definitions("${PROJECT_NAME}" PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
	target_compile_definitions("${PROJECT_NAME}" PUBLIC PRODUCTION_BUILD=0) 
	target_compile_definitions("${PROJECT_NAME}" PUBLIC DEVELOPLEMT_BUILD=1) 
endif()

# Subdirectiories
# add_subdirectory(thirdparty/)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/duaLib)
add_subdirectory(thirdparty/json)
add_subdirectory(thirdparty/miniaudio)
add_subdirectory(thirdparty/ViGEmClient)

# Linking libraries
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad imgui duaLib nlohmann_json miniaudio ViGEmClient)

# Copying DLLs to out folder
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:duaLib> $<TARGET_FILE_DIR:${PROJECT_NAME}>)