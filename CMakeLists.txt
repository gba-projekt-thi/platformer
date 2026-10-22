cmake_minimum_required(VERSION 3.20)

project(ProjectGBA LANGUAGES NONE)

# This uses Butano's Makefile system under the hood
add_custom_target(game ALL
    COMMAND make -C ${CMAKE_SOURCE_DIR}
    COMMENT "Building game using Butano Makefile"
    USES_TERMINAL
)

# Clean target for game
add_custom_target(game-clean
    COMMAND make -C ${CMAKE_SOURCE_DIR} clean
    COMMENT "Cleaning game"
)