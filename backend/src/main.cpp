// main.cpp — Entry point for Sudoku Solver Backend
// Reads PORT from environment (required for Render / Railway / Cloud Run)

#include <cstdlib>
#include <iostream>

// Declared in server.cpp
void startServer(int port);

int main() {
    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::atoi(portEnv) : 8080;

    std::cout << "Starting Sudoku Solver backend on port " << port << std::endl;
    startServer(port);
    return 0;
}
