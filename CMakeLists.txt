cmake_minimum_required(VERSION 3.20)
project(ShapeyTower LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib discover or fallback
find_package(raylib QUIET)
if(NOT raylib_FOUND)
    message(STATUS "raylib not found - expecting vendor or system install. Define RAYLIB_ROOT if needed.")
endif()

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_executable(shapeytower ${SRC_FILES})

# Windows embedded icon resource (Explorer/taskbar) - requires shapeyicon.ico
if(WIN32)
    set(APP_ICON_RC "${CMAKE_SOURCE_DIR}/resources/appicon.rc")
    # Optional: auto-generate .ico from PNG if ImageMagick is available
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/icons/shapeyicon.png" AND NOT EXISTS "${CMAKE_SOURCE_DIR}/resources/shapeyicon.ico")
        find_program(MAGICK_EXE NAMES magick)
        if(MAGICK_EXE)
            message(STATUS "Generating resources/shapeyicon.ico from PNG via ImageMagick...")
            execute_process(
                COMMAND ${MAGICK_EXE} "${CMAKE_SOURCE_DIR}/assets/icons/shapeyicon.png" -define icon:auto-resize=256,128,64,48,32,16 "${CMAKE_SOURCE_DIR}/resources/shapeyicon.ico"
                RESULT_VARIABLE ICO_GEN_RES
                OUTPUT_QUIET ERROR_QUIET)
            if(ICO_GEN_RES EQUAL 0)
                message(STATUS "Icon generated: resources/shapeyicon.ico")
            else()
                message(WARNING "ImageMagick present but .ico generation failed; continuing without embedded icon.")
            endif()
        else()
            message(STATUS "ImageMagick not found; skip PNG→ICO conversion.")
        endif()
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/shapeyicon.ico")
        # If the .ico is in resources folder alongside appicon.rc, reference it directly
        message(STATUS "Windows icon: embedding resources/shapeyicon.ico")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/assets/icons/shapeyicon.ico")
        # If user put ico under assets/icons, copy at configure time for RC script path
        file(COPY "${CMAKE_SOURCE_DIR}/assets/icons/shapeyicon.ico" DESTINATION "${CMAKE_SOURCE_DIR}/resources")
        message(STATUS "Windows icon: copied assets/icons/shapeyicon.ico -> resources/")
    endif()
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/shapeyicon.ico")
        target_sources(shapeytower PRIVATE ${APP_ICON_RC})
        set_source_files_properties(${APP_ICON_RC} PROPERTIES LANGUAGE RC)
    else()
        message(STATUS "Windows icon (.ico) not found; runtime window icon via raylib only.")
    endif()
endif()

if(raylib_FOUND)
    target_link_libraries(shapeytower PRIVATE raylib)
else()
    # Fallback manual link (Windows typical) - adjust paths if needed
    if (WIN32)
    # Assume raylib is available to the linker (system/vcpkg/MSYS2). Link opengl32 + system libs.
    target_link_libraries(shapeytower PRIVATE raylib opengl32 winmm gdi32)
    endif()
endif()

target_include_directories(shapeytower PRIVATE include)

# Copy assets next to the built executable (Release/Debug)
add_custom_command(TARGET shapeytower POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:shapeytower>/assets
    COMMENT "Copy assets/ next to the executable"
)
