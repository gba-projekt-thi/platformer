cmake_minimum_required(VERSION 3.20)

project(ProjectGBA LANGUAGES NONE)

# This uses Butano's Makefile system under the hood
add_custom_target(game
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/game
    COMMENT "Building game using Butano Makefile"
    USES_TERMINAL
)

# Clean target for game
add_custom_target(game-clean
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/game clean
    COMMENT "Cleaning game"
)

# Set the default build target
add_custom_target(all_projects ALL DEPENDS game)
