#---------------------------------------------------------------------------------------------------------------------
# TARGET is the name of the output.
# BUILD is the directory where object files & intermediate files will be placed.
# LIBBUTANO is the main directory of butano library (https://github.com/GValiente/butano).
# PYTHON is the path to the python interpreter.
# SOURCES is a list of directories containing source code.
# INCLUDES is a list of directories containing extra header files.
# DATA is a list of directories containing binary data files with *.bin extension.
# GRAPHICS is a list of files and directories containing files to be processed by grit.
# AUDIO is a list of files and directories containing files to be processed by the audio backend.
# AUDIOBACKEND specifies the backend used for audio playback. Supported backends: maxmod, aas, null.
# AUDIOTOOL is the path to the tool used process the audio files.
# DMGAUDIO is a list of files and directories containing files to be processed by the DMG audio backend.
# DMGAUDIOBACKEND specifies the backend used for DMG audio playback. Supported backends: default, null.
# ROMTITLE is a uppercase ASCII, max 12 characters text string containing the output ROM title.
# ROMCODE is a uppercase ASCII, max 4 characters text string containing the output ROM code.
# USERFLAGS is a list of additional compiler flags:
#     Pass -flto to enable link-time optimization.
#     Pass -O0 or -Og to try to make debugging work.
# USERCXXFLAGS is a list of additional compiler flags for C++ code only.
# USERASFLAGS is a list of additional assembler flags.
# USERLDFLAGS is a list of additional linker flags:
#     Pass -flto=<number_of_cpu_cores> to enable parallel link-time optimization.
# USERLIBDIRS is a list of additional directories containing libraries.
#     Each libraries directory must contains include and lib subdirectories.
# USERLIBS is a list of additional libraries to link with the project.
# DEFAULTLIBS links standard system libraries when it is not empty.
# STACKTRACE enables stack trace logging when it is not empty.
# USERBUILD is a list of additional directories to remove when cleaning the project.
# EXTTOOL is an optional command executed before processing audio, graphics and code files.
#
# All directories are specified relative to the project directory where the makefile is found.
#---------------------------------------------------------------------------------------------------------------------
TARGET      	:=  platformer
BUILD       	:=  build
LIBBUTANO   	:=  extern/engine/extern/butano/butano
PYTHON      	:=  python
SOURCES     	:=  game/src game/src/traps game/src/player game/src/scene extern/engine/core/src extern/engine/extern/butano/common/src extern/engine/src/save_system/src
INCLUDES    	:=  game/include game/include/player game/include/traps game/include/level game/include/scene extern/engine/core/include extern/engine/extern/butano/common/include extern/engine/src/save_system/include
DATA        	:=
GRAPHICS_ROOT 	:= game/graphics
GRAPHICS      	:= $(shell find $(GRAPHICS_ROOT) -type d) \ extern/engine/extern/butano/common/graphics
#GRAPHICS 		:=  game/graphics game/graphics/global game/graphics/level1 game/graphics/level2 extern/engine/extern/butano/common/graphics
AUDIO       	:=  game/audio extern/engine/extern/butano/common/audio
AUDIOBACKEND	:=  maxmod
AUDIOTOOL		:=  
DMGAUDIO    	:=  game/dmg_audio extern/engine/extern/butano/common/dmg_audio
DMGAUDIOBACKEND	:=  default
ROMTITLE    	:=  BUTANO SPRTS
ROMCODE     	:=  SBTP
USERFLAGS   	:=  
USERCXXFLAGS	:=  
USERASFLAGS 	:=  
USERLDFLAGS 	:=  
USERLIBDIRS 	:=  
USERLIBS    	:=  
DEFAULTLIBS 	:=  
STACKTRACE		:=	
USERBUILD   	:=  
EXTTOOL     	:=  

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------------------------------------------
# Tests (no ARM toolchain needed, so Butano's makefile is skipped for these goals):
#   make test       host unit tests + Python tool tests + level data checks
#   make checks     level data checks only
#   make coverage   host unit test line coverage of game/
#---------------------------------------------------------------------------------------------------------------------
TEST_GOALS	:=	test checks coverage

ifneq ($(MAKECMDGOALS),)
ifeq ($(filter-out $(TEST_GOALS),$(MAKECMDGOALS)),)
ONLY_TESTS	:=	1
endif
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
ifndef ONLY_TESTS
include $(LIBBUTANOABS)/butano.mak
endif

.PHONY: $(TEST_GOALS)

test:
	$(MAKE) -C tests/host
	python3 -m unittest discover -s tools -p 'test_*.py'
	$(MAKE) checks

checks:
	python3 tools/Generate_levels.py --check
	python3 tools/replays.py generate --check
	python3 tools/replays.py layouts
	python3 tools/Make_boss_music.py --check
	python3 tools/Check_trigger_references.py
	python3 tools/Check_level_hints.py
	python3 tools/Check_world_index.py
	python3 tools/Check_checkpoints.py
	python3 tools/Check_sprite_budget.py
	python3 tools/Check_hitboxes.py
	python3 tools/Check_level_reachability.py --all

coverage:
	$(MAKE) -C tests/host coverage

#---------------------------------------------------------------------------------------------------------------------
# Emulator tests (ARM toolchain needed, see tools/replays.py):
#   make record   platformer.record/platformer.gba records the duck's buttons into its save file
#   make replay   plays every recorded level headless; fails unless each one reaches its door
# (Butano needs a one-level build directory; platformer.* is already ignored by git.)
#---------------------------------------------------------------------------------------------------------------------
.PHONY: record replay tour

record:
	$(MAKE) TARGET=platformer.record/platformer BUILD=platformer.record USERFLAGS=-DPLATFORMER_RECORD

# A scripted walk through the real game, photographed in mGBA (tools/tour.py);
# the pictures and report.html land in platformer.tour/shots.
tour:
	python3 tools/replays.py generate
	python3 tools/tour.py generate
	$(MAKE) TARGET=platformer.tour/platformer BUILD=platformer.tour USERFLAGS="-DPLATFORMER_TOUR -I$(CURDIR)/platformer.tour"
	python3 tools/tour.py run platformer.tour/platformer.gba platformer.tour/shots

replay:
	python3 tools/replays.py generate
	$(MAKE) TARGET=platformer.replay/platformer BUILD=platformer.replay USERFLAGS=-DPLATFORMER_REPLAY
	python3 tools/replays.py run platformer.replay/platformer.gba
