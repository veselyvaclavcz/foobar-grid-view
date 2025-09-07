cmake_minimum_required(VERSION 3.20)
project(foo_albumart_grid VERSION 11.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find foobar2000 SDK (you need to set FB2K_SDK_PATH)
if(NOT DEFINED ENV{FB2K_SDK_PATH})
    message(FATAL_ERROR "FB2K_SDK_PATH environment variable not set. Please set it to the foobar2000 SDK directory.")
endif()

set(FB2K_SDK_PATH $ENV{FB2K_SDK_PATH})

# Component sources
set(SOURCES
    src/main.cpp
    
    # Foundation layer
    src/foundation/lifecycle_manager.cpp
    src/foundation/callback_manager.cpp
    
    # Integration layer
    src/integration/sdk_integration.cpp
    
    # Core layer
    src/core/album_data_model.cpp
    
    # Resources layer
    src/resources/thumbnail_cache.cpp
    src/resources/album_art_loader.cpp
    
    # UI layer
    src/ui/grid_window.cpp
    src/ui/grid_renderer.cpp
    src/ui/input_handler.cpp
    src/ui/context_menu.cpp
)

# Component headers
set(HEADERS
    src/foundation/lifecycle_manager.h
    src/foundation/callback_manager.h
    src/integration/sdk_integration.h
    src/core/album_data_model.h
    src/resources/thumbnail_cache.h
    src/resources/album_art_loader.h
    src/ui/grid_window.h
    src/ui/grid_renderer.h
    src/ui/input_handler.h
    src/ui/context_menu.h
)

# Create the component DLL
add_library(foo_albumart_grid SHARED ${SOURCES} ${HEADERS})

# Set output name
set_target_properties(foo_albumart_grid PROPERTIES
    OUTPUT_NAME "foo_albumart_grid"
    PREFIX ""
    SUFFIX ".dll"
)

# Include directories
target_include_directories(foo_albumart_grid PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FB2K_SDK_PATH}
    ${FB2K_SDK_PATH}/foobar2000
)

# Windows specific settings
if(WIN32)
    target_compile_definitions(foo_albumart_grid PRIVATE
        WIN32
        _WINDOWS
        _USRDLL
        UNICODE
        _UNICODE
    )
    
    # Compiler flags
    if(MSVC)
        target_compile_options(foo_albumart_grid PRIVATE
            /W3          # Warning level 3
            /WX-         # Don't treat warnings as errors
            /MP          # Multi-processor compilation
            /EHsc        # Enable C++ exceptions
            /GR          # Enable RTTI
            /Zi          # Debug information
            /O2          # Optimize for speed
            /MD          # Multi-threaded DLL runtime
        )
        
        # Linker flags
        target_link_options(foo_albumart_grid PRIVATE
            /SUBSYSTEM:WINDOWS
            /DEBUG
            /OPT:REF
            /OPT:ICF
        )
    endif()
endif()

# Link libraries
target_link_libraries(foo_albumart_grid PRIVATE
    # Windows libraries
    kernel32
    user32
    gdi32
    winspool
    comdlg32
    advapi32
    shell32
    ole32
    oleaut32
    uuid
    odbc32
    odbccp32
    gdiplus
    
    # foobar2000 SDK libraries (you need to build these first)
    ${FB2K_SDK_PATH}/foobar2000/shared/shared.lib
    ${FB2K_SDK_PATH}/foobar2000/SDK/foobar2000_SDK.lib
    ${FB2K_SDK_PATH}/pfc/pfc.lib
)

# Post-build: Create .fb2k-component file
add_custom_command(TARGET foo_albumart_grid POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        $<TARGET_FILE:foo_albumart_grid>
        ${CMAKE_BINARY_DIR}/foo_albumart_grid.fb2k-component
    COMMENT "Creating .fb2k-component file"
)

# Install target
install(TARGETS foo_albumart_grid
    RUNTIME DESTINATION .
)

install(FILES ${CMAKE_BINARY_DIR}/foo_albumart_grid.fb2k-component
    DESTINATION .
)
