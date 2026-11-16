cmake_minimum_required(VERSION 3.16)

project(lacueva-multistream VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Encontrar Qt (OBS 28+ utiliza Qt 6 por defecto)
find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)
set(QT_TARGET_LIBS Qt6::Core Qt6::Widgets Qt6::Gui)

# Ubicación del SDK de OBS Studio
if(DEFINED ENV{OBS_SRC_DIR})
    set(OBS_SRC_DIR $ENV{OBS_SRC_DIR})
elseif(NOT DEFINED OBS_SRC_DIR)
    set(OBS_SRC_DIR "C:/obs-studio-src")
endif()

if(DEFINED ENV{OBS_BIN_DIR})
    set(OBS_BIN_DIR $ENV{OBS_BIN_DIR})
elseif(NOT DEFINED OBS_BIN_DIR)
    set(OBS_BIN_DIR "C:/obs-studio-bin/bin/64bit")
endif()

# Intentar find_package oficial primero; si no, usar SDK descargado
find_package(libobs QUIET)
find_package(obs-frontend-api QUIET)

# Archivos fuente del plugin
set(PLUGIN_SOURCES
    src/plugin-main.cpp
    src/core/multi-output-manager.h
    src/core/multi-output-manager.cpp
    src/core/platform-config.h
    src/core/settings-manager.h
    src/core/settings-manager.cpp
    src/platforms/platform-validator.h
    src/platforms/platform-validator.cpp
    src/platforms/platform-presets.h
    src/platforms/platform-presets.cpp
    src/platforms/bandwidth-optimizer.h
    src/platforms/bandwidth-optimizer.cpp
    src/platforms/resource-monitor.h
    src/platforms/resource-monitor.cpp
    src/ui/multistream-dock.h
    src/ui/multistream-dock.cpp
    src/ui/status-widget.h
    src/ui/status-widget.cpp
    src/ui/settings-dialog.h
    src/ui/settings-dialog.cpp
)

add_library(lacueva-multistream MODULE ${PLUGIN_SOURCES})

target_include_directories(lacueva-multistream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platforms
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

if(TARGET OBS::libobs)
    target_link_libraries(lacueva-multistream PRIVATE
        OBS::libobs
        OBS::obs-frontend-api
        ${QT_TARGET_LIBS}
    )
else()
    # Usar headers directos de OBS Studio
    target_include_directories(lacueva-multistream PRIVATE
        "${OBS_SRC_DIR}/libobs"
        "${OBS_SRC_DIR}/UI/obs-frontend-api"
    )

    if(WIN32)
        # Importar librerías de enlace dinámico de OBS
        find_library(OBS_LIB NAMES obs obs.lib PATHS "${OBS_BIN_DIR}" "${OBS_SRC_DIR}/build/libobs/Release")
        find_library(OBS_FRONTEND_LIB NAMES obs-frontend-api obs-frontend-api.lib PATHS "${OBS_BIN_DIR}" "${OBS_SRC_DIR}/build/UI/obs-frontend-api/Release")

        target_link_libraries(lacueva-multistream PRIVATE
            ${OBS_LIB}
            ${OBS_FRONTEND_LIB}
            ${QT_TARGET_LIBS}
            ws2_32
            iphlpapi
            dxgi
        )
    endif()
endif()

if(WIN32)
    set_target_properties(lacueva-multistream PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
endif()
