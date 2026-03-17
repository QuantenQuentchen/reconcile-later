cmake_minimum_required(VERSION 3.20)
project(N_1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/std:c++latest)
endif()
include(FetchContent)

# Keep raylib dependency lean for local app builds.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
)

FetchContent_Declare(
        raylib_cpp
        GIT_REPOSITORY https://github.com/RobLoach/raylib-cpp.git
        GIT_TAG v5.5.0
)

FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(raylib raylib_cpp entt)

add_executable(N_1 main.cpp
        external/nanosvg.h
        external/nanosvgrast.h
        Components/Transform.h
        Components/Markers.h
        Systems/movementSystem.h
        Components/Visual.h
        settings.h
        Components/Upgrades.h
        InputMappings.h
        Constants/RenderConstants.h
        Systems/render.h
        Resources/RenderCache.h
        Systems/effects.h
        Components/Effects.h)
target_link_libraries(N_1 PRIVATE raylib raylib_cpp EnTT::EnTT)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET N_1 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:N_1>/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:N_1>/assets"
            COMMENT "Copying assets to output directory"
    )
endif ()

