cmake_minimum_required(VERSION 3.20)
project(factoman VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/http_client.cpp
    src/factorio_zone_client.cpp
    src/supabase_sync.cpp
    src/ui/main_window.cpp
    src/ui/settings_dialog.cpp
)

set(HEADERS
    src/config.hpp
    src/http_client.hpp
    src/factorio_zone_client.hpp
    src/supabase_sync.hpp
    src/ui/main_window.hpp
    src/ui/settings_dialog.hpp
)

add_executable(factoman ${SOURCES} ${HEADERS})

target_include_directories(factoman PRIVATE src)

# Find Packages
find_package(FLTK REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(factoman PRIVATE
    fltk
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(factoman PRIVATE
        ws2_32
        crypt32
        wldap32
        ole32
        comctl32
        winspool
        gdi32
        user32
    )
    # Enable Windows subsystem (or leave console enabled for debugging)
    # set_target_properties(factoman PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
