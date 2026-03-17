cmake_minimum_required(VERSION 3.20)

project(ProjectGBA LANGUAGES NONE)

# GBA Sprites Example
# This uses Butano's Makefile system under the hood
add_custom_target(sprites
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/src/sprites
    COMMENT "Building sprites example using Butano Makefile"
    USES_TERMINAL
)

# Clean target for sprites
add_custom_target(sprites-clean
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/src/sprites clean
    COMMENT "Cleaning sprites example"
)

# Set the default build target
add_custom_target(all_projects ALL DEPENDS sprites)
