cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (Updater)
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

set(COMPILE_TO_DLL OFF)
set(PRODUCTION_BUILD ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)

# CROSS PLATFORM LIBRARIES
add_subdirectory(thirdparty/zip)
add_subdirectory(thirdparty/cpp-httplib)

if(ANDROID)
    message(STATUS "Building for Android")
    add_library(${PROJECT_NAME} SHARED ${MY_SOURCES})

    find_library(log-lib log)
    find_library(android-lib android)
    find_library(egl-lib EGL)
    find_library(glesv2-lib GLESv2)
    target_link_libraries(${PROJECT_NAME} ${log-lib} ${android-lib} ${egl-lib} ${glesv2-lib})
else()
    if(COMPILE_TO_DLL)
        add_library(${PROJECT_NAME} SHARED ${MY_SOURCES})
        target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILE_TO_DLL=1)
    else()
        if(WIN32)
            add_executable(${PROJECT_NAME} ${MY_SOURCES})
	        target_compile_definitions("${PROJECT_NAME}" PRIVATE WINDOWS=1) 
        elseif(APPLE)
	        add_executable(${PROJECT_NAME} ${MY_SOURCES})
	        target_compile_definitions("${PROJECT_NAME}" PRIVATE APPLE=1)
        elseif(UNIX)
            add_executable(${PROJECT_NAME} ${MY_SOURCES})
	        target_compile_definitions("${PROJECT_NAME}" PRIVATE LINUX=1)
        endif()
    endif()

    
    # DESKTOP ONLY LIBRARIES
    add_subdirectory(thirdparty/tiny-process-library)
    find_package(OpenSSL REQUIRED)

    message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
    target_link_libraries(${PROJECT_NAME} PRIVATE zip tiny-process-library httplib OpenSSL::SSL OpenSSL::Crypto)
endif()

if(PRODUCTION_BUILD)
target_compile_definitions(${PROJECT_NAME} PRIVATE PRODUCTION_BUILD=1)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME}
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

# Copying DLLs to out folder
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:YourTarget> $<TARGET_FILE_DIR:${PROJECT_NAME}>)