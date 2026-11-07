# Builds 3D Weather Globe & Atlas Deluxe Edition 2.0.7 as a Universal 2
# (arm64 + x86_64) app from the translated sources in src/, with the original
# resources from game/.
#
#   make            build what is translated so far
#   make progress   count translated and untranslated functions
#   make clean

ARCHS    := -arch arm64 -arch x86_64
MINOS    := -mmacosx-version-min=11.0
CXX      := clang++
CXXFLAGS := $(ARCHS) $(MINOS) -std=gnu++17 -fno-objc-arc -g -O1 -Wall \
            -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function \
            -Wno-overloaded-virtual -DGL_SILENCE_DEPRECATION \
            -Isrc/LFoundation/Include
LDFLAGS  := $(ARCHS) $(MINOS)
OBJ      := build/obj

# Worksheets still made of UNTRANSLATED blocks only are skipped: a file is
# built once it has code in it (an #include).
translated = $(shell grep -l -s '^\#include' $(1))

LF_SRC := $(call translated,$(wildcard src/LFoundation/*.cpp src/LFoundation/*.mm))
LF_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(LF_SRC))

FH_SRC := $(call translated,$(wildcard src/FishHook/*.cpp src/FishHook/*.mm))
FH_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(FH_SRC))
$(FH_OBJ): CXXFLAGS += -Isrc/FishHook/Include

SQL_SRC := $(call translated,$(wildcard src/SQLiteLib/*.cpp))
SQL_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(SQL_SRC))
$(SQL_OBJ): CXXFLAGS += -Isrc/SQLiteLib/Include

WC_SRC := $(call translated,$(wildcard src/WebCamsDB_C/*.cpp))
WC_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(WC_SRC))
$(WC_OBJ): CXXFLAGS += -Isrc/SQLiteLib/Include -Isrc/WebCamsDB_C/Include

AM_SRC := $(call translated,$(wildcard src/AppModel/*.cpp src/AppModel/*.mm))
AM_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(AM_SRC))
$(AM_OBJ): CXXFLAGS += -Isrc/FishHook/Include -Isrc/SQLiteLib/Include -Isrc/WebCamsDB_C/Include \
                       -Isrc/AppModel/Include

RL_SRC := $(call translated,$(wildcard src/RenderLib/*.cpp src/RenderLib/*.mm))
RL_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(RL_SRC))
$(RL_OBJ): CXXFLAGS += -Isrc/FishHook/Include -Isrc/SQLiteLib/Include -Isrc/WebCamsDB_C/Include \
                       -Isrc/AppModel/Include -Isrc/RenderLib/Include

SS_SRC := $(call translated,$(wildcard src/ScreenSaverLib/*.cpp src/ScreenSaverLib/*.mm))
SS_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(SS_SRC))
$(SS_OBJ): CXXFLAGS += -Isrc/FishHook/Include -Isrc/SQLiteLib/Include -Isrc/WebCamsDB_C/Include \
                       -Isrc/AppModel/Include -Isrc/RenderLib/Include -Isrc/ScreenSaverLib/Include

IM_SRC := $(call translated,$(wildcard src/SMKiMovieExport/*.mm))
IM_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(IM_SRC))
$(IM_OBJ): CXXFLAGS += -Isrc/SMKiMovieExport/Include

UC_SRC := $(call translated,$(wildcard src/URLCreator/*.cpp))
UC_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(UC_SRC))

# Objective-C; every source gets the umbrella header, as the original's
# prefix header gave it the whole framework.
SMK_SRC := $(wildcard src/SMKControls/*.m)
SMK_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(SMK_SRC))
OBJCFLAGS := $(ARCHS) $(MINOS) -fno-objc-arc -g -O1 -Wall -Wno-deprecated-declarations \
             -Wno-unused-variable -Wno-unused-function -DGL_SILENCE_DEPRECATION -Isrc/Common
$(SMK_OBJ): OBJCFLAGS += -Isrc/SMKControls -include src/SMKControls/SMKControlsUmbrella.h

# The application: every file is translated (the one-line notes compile to
# nothing).
WG_SRC := $(wildcard src/WeatherGlobe/*.mm src/WeatherGlobe/*.cpp)
WG_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(WG_SRC))
WG_INC := -Isrc/FishHook/Include -Isrc/SQLiteLib/Include -Isrc/WebCamsDB_C/Include -Isrc/AppModel/Include \
          -Isrc/RenderLib/Include -Isrc/Common -Isrc/SMKControls -Isrc/SMKiMovieExport/Include -Isrc/WeatherGlobe
$(WG_OBJ): CXXFLAGS += $(WG_INC)

# The plugins.
SAVER_OBJ := $(OBJ)/GlobeScreenSaver/AtlasScreenSaverView.mm.o
MDI_OBJ := $(OBJ)/GlobeImporter/MetadataImporter.mm.o
QL_SRC := $(wildcard src/AtlasRoute/*.mm)
QL_OBJ := $(patsubst src/%,$(OBJ)/%.o,$(QL_SRC))

.PHONY: all clean progress
all: build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib build/bin/WebCamsDB_C build/bin/AppModel \
     build/bin/RenderLib build/bin/ScreenSaverLib build/bin/SMKiMovieExport \
     build/bin/URLCreator build/bin/SMKControls build/bin/WeatherGlobe \
     build/bin/GlobeScreenSaver build/bin/GlobeImporter build/bin/AtlasRoute \
     build/tools/nibconv

$(OBJ)/%.cpp.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

$(OBJ)/%.mm.o: src/%.mm
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

$(OBJ)/%.m.o: src/%.m
	@mkdir -p $(@D)
	clang -x objective-c $(OBJCFLAGS) -MMD -c $< -o $@

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)

build/bin/LFoundation: $(LF_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(LF_OBJ) \
	    -install_name @rpath/LFoundation.framework/Versions/A/LFoundation \
	    -undefined dynamic_lookup \
	    -framework Foundation -framework CoreFoundation -framework IOKit \
	    -framework Security -framework SystemConfiguration -framework CFNetwork \
	    -framework CoreServices -framework CoreGraphics -framework Accelerate -lz

build/bin/FishHook: $(FH_OBJ) build/bin/LFoundation
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(FH_OBJ) build/bin/LFoundation \
	    -install_name @rpath/FishHook.framework/Versions/A/FishHook \
	    -framework Cocoa -framework OpenGL -framework OpenAL -framework AudioToolbox \
	    -framework CoreFoundation -framework CoreServices -framework ApplicationServices -framework Accelerate

build/bin/SQLiteLib: $(SQL_OBJ) build/bin/LFoundation
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(SQL_OBJ) build/bin/LFoundation \
	    -install_name @rpath/SQLiteLib.framework/Versions/A/SQLiteLib \
	    -framework CoreFoundation -lsqlite3

build/bin/WebCamsDB_C: $(WC_OBJ) build/bin/LFoundation build/bin/SQLiteLib
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(WC_OBJ) build/bin/LFoundation build/bin/SQLiteLib \
	    -install_name @rpath/WebCamsDB_C.framework/Versions/A/WebCamsDB_C \
	    -framework CoreFoundation -framework CoreServices

build/bin/AppModel: $(AM_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib build/bin/WebCamsDB_C
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(AM_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib \
	    build/bin/WebCamsDB_C \
	    -install_name @rpath/AppModel.framework/Versions/A/AppModel \
	    -framework Foundation -framework CoreFoundation -framework CoreServices -framework CoreGraphics -framework ImageIO

build/bin/RenderLib: $(RL_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib build/bin/WebCamsDB_C \
                    build/bin/AppModel
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(RL_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib \
	    build/bin/WebCamsDB_C build/bin/AppModel \
	    -install_name @rpath/RenderLib.framework/Versions/A/RenderLib \
	    -framework Cocoa -framework OpenGL -framework AVFoundation -framework CoreMedia -framework CoreVideo \
	    -framework Accelerate -framework CoreFoundation -framework CoreServices -framework ApplicationServices

build/bin/ScreenSaverLib: $(SS_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib build/bin/AppModel \
                         build/bin/RenderLib
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(SS_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib \
	    build/bin/AppModel build/bin/RenderLib \
	    -install_name @rpath/ScreenSaverLib.framework/Versions/A/ScreenSaverLib \
	    -rpath @loader_path/../../.. \
	    -framework Cocoa -framework ScreenSaver -framework OpenGL -framework CoreFoundation

build/bin/SMKiMovieExport: $(IM_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -dynamiclib -o $@ $(IM_OBJ) \
	    -install_name @rpath/SMKiMovieExport.framework/Versions/A/SMKiMovieExport \
	    -framework Foundation -framework AVFoundation -framework CoreMedia

# A bundle (MH_BUNDLE) like the original: AppModel's DynamicEnvironment loads
# it with CFBundle from BASE/URLCreator.
build/bin/URLCreator: $(UC_OBJ) build/bin/LFoundation
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -bundle -o $@ $(UC_OBJ) build/bin/LFoundation

build/bin/SMKControls: $(SMK_OBJ)
	@mkdir -p $(@D)
	clang $(LDFLAGS) -dynamiclib -o $@ $(SMK_OBJ) \
	    -install_name @rpath/SMKControls.framework/Versions/A/SMKControls \
	    -compatibility_version 1.0.0 -current_version 211.0.0 \
	    -framework Cocoa -framework CoreServices

# Host tool: re-archives MainMenu.nib (the one typedstream nib, which 64-bit
# AppKit can't load) as a keyed nib; tools/package.sh runs it.
build/tools/nibconv: tools/nibconv.m
	@mkdir -p $(@D)
	clang -fno-objc-arc -o $@ $< -framework Cocoa

progress:
	@python3 -I tools/progress.py

clean:
	rm -rf build

build/bin/WeatherGlobe: $(WG_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib build/bin/WebCamsDB_C \
                       build/bin/AppModel build/bin/RenderLib build/bin/SMKControls build/bin/SMKiMovieExport
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o $@ $(WG_OBJ) build/bin/LFoundation build/bin/FishHook build/bin/SQLiteLib \
	    build/bin/WebCamsDB_C build/bin/AppModel build/bin/RenderLib build/bin/SMKControls build/bin/SMKiMovieExport \
	    -rpath @executable_path/../Frameworks \
	    -framework Cocoa -framework Carbon -framework AddressBook -framework Security -framework CoreServices \
	    -framework ApplicationServices -framework OpenGL -framework QuartzCore -framework AVFoundation \
	    -framework CoreMedia -framework IOKit

build/bin/GlobeScreenSaver: $(SAVER_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -bundle -o $@ $(SAVER_OBJ) -framework Cocoa -framework ScreenSaver

build/bin/GlobeImporter: $(MDI_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -bundle -o $@ $(MDI_OBJ) -framework Foundation -framework CoreServices

build/bin/AtlasRoute: $(QL_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -bundle -o $@ $(QL_OBJ) -framework Cocoa -framework QuickLook
