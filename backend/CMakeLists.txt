cmake_minimum_required(VERSION 3.15)
project(SudokuBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc dnn)

# Allow threading (httplib uses std::thread)
find_package(Threads REQUIRED)

# ── Sources ───────────────────────────────────────────────────────────────────
add_executable(SudokuBackend
    src/main.cpp
    src/server.cpp
    src/sudoku_solver.cpp
    src/image_intake_service.cpp
    src/vision_pipeline.cpp
)

target_include_directories(SudokuBackend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(SudokuBackend
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
)

# ── Assets (ONNX model for CNN digit recognition) ─────────────────────────────
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET SudokuBackend POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:SudokuBackend>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:SudokuBackend>/assets"
        COMMENT "Copying assets to build output directory"
    )
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/assets/models/mnist.onnx")
    message(STATUS
        "Note: assets/models/mnist.onnx not found. "
        "Image solving will fall back to pixel-ratio classification. "
        "Manual solve (/solve-manual) works without the model.")
endif()
