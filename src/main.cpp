#include <FL/Fl.H>
#include <iostream>
#include "http_client.hpp"
#include "ui/main_window.hpp"

int main(int argc, char** argv) {
    // Initialize libcurl
    HttpClient::global_init();

    // Enable multi-threading in FLTK
    Fl::lock();

    // Create and display main application window
    MainWindow window(760, 615, "FactoMan");
    window.show(argc, argv);

    int result = Fl::run();

    // Clean up
    HttpClient::global_cleanup();

    return result;
}
