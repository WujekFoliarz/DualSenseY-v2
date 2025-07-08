cmake_minimum_required(VERSION 3.20.2)
project(ViGEmClient VERSION 1.21.222.0)

# Install directory setup
include(GNUInstallDirs)
set(VIGEM_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR})

option(ViGEmClient_DLL "Generate a dynamic library instead of a static library" OFF)

set(VIGEM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ViGEm/km/BusShared.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ViGEm/Client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ViGEm/Common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ViGEm/Util.h
)
set(VIGEM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ViGEmClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ViGEmClient.rc
)

function(setup_include_directories target_name)
    target_include_directories(
        ${target_name}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${VIGEM_INC_DIR}>
    )
endfunction()

add_library(ViGEmClient STATIC
    ${VIGEM_HEADERS}
    ${VIGEM_SOURCES}
)
setup_include_directories(ViGEmClient)
add_library(ViGEmClient::ViGEmClient ALIAS ViGEmClient)

# Setup dynamic library if requested and set output name to ViGEmClient.dll
if (ViGEmClient_DLL)
    add_library(ViGEmClientShared SHARED
        ${VIGEM_HEADERS}
        ${VIGEM_SOURCES}
    )
    set_target_properties(ViGEmClientShared PROPERTIES
        OUTPUT_NAME ViGEmClient
        SUFFIX .dll
    )
    target_link_libraries(ViGEmClientShared PRIVATE setupapi.lib)
    setup_include_directories(ViGEmClientShared)
    add_library(ViGEmClient::ViGEmClientShared ALIAS ViGEmClientShared)
endif()

# Setup install targets
include(CMakePackageConfigHelpers)
set(VIGEM_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/ViGEmClient)
set(version_config "${PROJECT_BINARY_DIR}/ViGEmClientConfigVersion.cmake")
set(project_config "${PROJECT_BINARY_DIR}/ViGEmClientConfig.cmake")
set(pkgconfig ${PROJECT_BINARY_DIR}/ViGEmClient.pc)
set(targets_export_name ViGEmClientTargets)
set(VIGEMCLIENT_LIB_NAME ViGEmClient)

write_basic_package_version_file(
    ${version_config}
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

set(VIGEM_LIB_DIR ${CMAKE_INSTALL_LIBDIR})
set(VIGEM_PKGCONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# Create path
string(CONCAT libdir_for_pc_file "\${exec_prefix}" "/" "${VIGEM_LIB_DIR}")
string(CONCAT includedir_for_pc_file "\${prefix}" "/" "${VIGEM_INC_DIR}")

# pkg-config support
configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/ViGEmClient.pc.in  # Input file
    ${pkgconfig}
    @ONLY
)

# CMake package config
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ViGEmClientConfig.cmake.in
    ${project_config}
    INSTALL_DESTINATION ${VIGEM_CMAKE_DIR}
)

# Setup install targets
set(INSTALL_TARGETS ViGEmClient)
if (ViGEmClient_DLL)
    list(APPEND INSTALL_TARGETS ViGEmClientShared)
endif()

# Install library and headers
install(TARGETS ${INSTALL_TARGETS} EXPORT ${targets_export_name}
        LIBRARY DESTINATION ${VIGEM_LIB_DIR}
        ARCHIVE DESTINATION ${VIGEM_LIB_DIR}
        PUBLIC_HEADER DESTINATION "${VIGEM_INC_DIR}/ViGEm"
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${VIGEM_HEADERS} DESTINATION "${VIGEM_INC_DIR}/ViGEm")

export(TARGETS ${INSTALL_TARGETS} NAMESPACE ViGEmClient::
       FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

# Install version, config and target files.
install(
    FILES ${project_config} ${version_config}
    DESTINATION ${VIGEM_CMAKE_DIR}
)
install(EXPORT ${targets_export_name} DESTINATION ${VIGEM_CMAKE_DIR}
        NAMESPACE ViGEmClient::)

# TODO: pkg-config doesn't work as expected on Windows
install(FILES "${pkgconfig}" DESTINATION "${VIGEM_PKGCONFIG_DIR}")

