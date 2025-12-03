cmake_minimum_required(VERSION 3.20)
project(ShapeyTower VERSION 1.5.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Don't use find_package for raylib - we want to control static vs dynamic linking
# find_package(raylib QUIET)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# WIN32 flag makes it a Windows GUI application (no console window)
if(WIN32)
    add_executable(shapeytower WIN32 ${SRC_FILES})
else()
    add_executable(shapeytower ${SRC_FILES})
endif()

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

# Link raylib and system libraries
if (WIN32 AND MINGW)
    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    
    # Find raylib dynamically
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        message(WARNING "raylib not found via find_package, trying direct link")
        target_link_libraries(shapeytower PRIVATE raylib)
    else()
        message(STATUS "Found raylib: ${raylib_VERSION}")
        target_link_libraries(shapeytower PRIVATE raylib)
    endif()
    
    # System libraries needed by raylib
    target_link_libraries(shapeytower PRIVATE opengl32 winmm gdi32)
    
    # Hide console + static C++ runtime
    target_link_options(shapeytower PRIVATE 
        -mwindows 
        -static-libgcc 
        -static-libstdc++
    )
    
    # Copy required DLLs to build directory for distribution
    set(REQUIRED_DLLS
        "${MINGW_BIN_DIR}/libraylib.dll"
        "${MINGW_BIN_DIR}/glfw3.dll"
        "${MINGW_BIN_DIR}/libwinpthread-1.dll"
    )
    
    foreach(DLL_FILE ${REQUIRED_DLLS})
        if(EXISTS "${DLL_FILE}")
            add_custom_command(TARGET shapeytower POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_FILE}" $<TARGET_FILE_DIR:shapeytower>
                COMMENT "Copying ${DLL_FILE}"
            )
        endif()
    endforeach()
elseif(WIN32)
    target_link_libraries(shapeytower PRIVATE raylib opengl32 winmm gdi32)
    set_target_properties(shapeytower PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
else()
    target_link_libraries(shapeytower PRIVATE raylib)
endif()

target_include_directories(shapeytower PRIVATE include)

# Copy assets next to the built executable (Release/Debug)
add_custom_command(TARGET shapeytower POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:shapeytower>/assets
    COMMENT "Copy assets/ next to the executable"
)

# Note: For distribution, these DLLs are required alongside the executable:
# - libraylib.dll, glfw3.dll, libwinpthread-1.dll
# The DLLs are automatically copied to the build directory.

# ----- Install rules for packaging / staging -----
install(TARGETS shapeytower RUNTIME DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION .)
if(EXISTS "${CMAKE_SOURCE_DIR}/settings.cfg")
    install(FILES ${CMAKE_SOURCE_DIR}/settings.cfg DESTINATION .)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/highscore.txt")
    install(FILES ${CMAKE_SOURCE_DIR}/highscore.txt DESTINATION .)
endif()

# ----- CPack (ZIP) configuration for one-click packaging -----
set(CPACK_PACKAGE_NAME "ShapeyTower")
set(CPACK_PACKAGE_VENDOR "al3ksh")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_GENERATOR "ZIP")
include(CPack)
