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

find_package(nlohmann_json REQUIRED)

if(WIN32)
    # Define CURL_STATICLIB so curl functions link statically
    target_compile_definitions(factoman PRIVATE CURL_STATICLIB)

    # 100% Single-file static standalone executable
    target_link_options(factoman PRIVATE -static -static-libgcc -static-libstdc++)

    target_link_libraries(factoman PRIVATE
        C:/msys64/ucrt64/lib/libfltk.a
        C:/msys64/ucrt64/lib/libcurl.a
        C:/msys64/ucrt64/lib/libnghttp2.a
        C:/msys64/ucrt64/lib/libnghttp3.a
        C:/msys64/ucrt64/lib/libngtcp2.a
        C:/msys64/ucrt64/lib/libngtcp2_crypto_ossl.a
        C:/msys64/ucrt64/lib/libssl.a
        C:/msys64/ucrt64/lib/libcrypto.a
        C:/msys64/ucrt64/lib/libssh2.a
        C:/msys64/ucrt64/lib/libzstd.a
        C:/msys64/ucrt64/lib/libbrotlidec.a
        C:/msys64/ucrt64/lib/libbrotlicommon.a
        C:/msys64/ucrt64/lib/libidn2.a
        C:/msys64/ucrt64/lib/libpsl.a
        C:/msys64/ucrt64/lib/libunistring.a
        C:/msys64/ucrt64/lib/libiconv.a
        C:/msys64/ucrt64/lib/libz.a
        nlohmann_json::nlohmann_json
        comdlg32
        ole32
        uuid
        comctl32
        winspool
        gdi32
        user32
        shell32
        ws2_32
        crypt32
        wldap32
        bcrypt
        advapi32
        secur32
        iphlpapi
    )
else()
    # Linux / macOS cross-platform build
    find_package(FLTK REQUIRED)
    find_package(CURL REQUIRED)
    target_link_libraries(factoman PRIVATE
        fltk
        CURL::libcurl
        nlohmann_json::nlohmann_json
    )
endif()
