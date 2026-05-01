TARGET = Forms3d

OBJS = src/main.o \
       src/platform/platform_core.o \
       src/platform/platform_sdl.o \
       src/platform/platform_psp.o \
       src/engine/math3d.o \
       src/engine/render3d.o \
       src/ui/logger.o \
       src/ui/text_renderer.o \
       src/ui/fps_counter.o \
       src/ui/controls_guide.o \
       src/audio/audio_core.o \
       src/audio/audio_sdl.o \
       src/audio/audio_psp.o

# PSP SDK Configuration
PSPSDK = $(shell psp-config --pspsdk-path)
PSPDIR = $(shell psp-config --psp-prefix)

# Compilation Flags
CFLAGS  = -O2 -G0 -Wall -Iinclude -Isrc
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

# Libraries — SDL2 + PSP GU/Audio/RTC all linked together
# (both backends are compiled into the same binary for runtime switching)
LIBS = -lSDL2_mixer -lvorbisidec -logg -lmodplug -lstdc++ \
       -lSDL2 -lGL \
       -lpspgu -lpspgum -lpspvfpu -lpspvram \
       -lpspaudio -lpsprtc -lpspdisplay \
       -lpsphprm -lpsppower -lm

LDFLAGS =

# EBOOT Metadata
EXTRA_TARGETS   = EBOOT.PBP
PSP_EBOOT_TITLE = Forms 3D

# Build Rules
include $(PSPSDK)/lib/build.mak
