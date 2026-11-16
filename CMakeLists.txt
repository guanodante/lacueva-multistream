cmake_minimum_required(VERSION 3.16)

project(lacueva-multistream VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Encontrar dependencias de OBS
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)

# Encontrar Qt (OBS 28+ utiliza Qt 6 por defecto)
find_package(Qt6 COMPONENTS Core Widgets Gui QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)
    set(QT_TARGET_LIBS Qt5::Core Qt5::Widgets Qt5::Gui)
else()
    set(QT_TARGET_LIBS Qt6::Core Qt6::Widgets Qt6::Gui)
endif()

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

# Definir la librería del módulo OBS
add_library(lacueva-multistream MODULE ${PLUGIN_SOURCES})

target_include_directories(lacueva-multistream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platforms
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(lacueva-multistream PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    ${QT_TARGET_LIBS}
)

# Bibliotecas de plataforma específicas
if(WIN32)
    target_link_libraries(lacueva-multistream PRIVATE
        ws2_32
        iphlpapi
        dxgi
    )
    set_target_properties(lacueva-multistream PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties(lacueva-multistream PROPERTIES
        PREFIX ""
        SUFFIX ".so"
    )
elseif(UNIX)
    set_target_properties(lacueva-multistream PROPERTIES
        PREFIX ""
        SUFFIX ".so"
    )
endif()

# Instalación
install(TARGETS lacueva-multistream
    LIBRARY DESTINATION "${OBS_PLUGIN_DESTINATION}"
    RUNTIME DESTINATION "${OBS_PLUGIN_DESTINATION}"
)

# Copiar archivos de idioma (data/locale)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/locale"
    DESTINATION "${OBS_DATA_DESTINATION}/lacueva-multistream"
)
