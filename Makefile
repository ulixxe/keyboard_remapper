# suppress command echoing globally
!CMDSWITCHES +s

# Tools (with /nologo to suppress banners)
CC   = cl /nologo
RC   = rc /nologo
LINK = link /nologo

# Flags
CFLAGS = /O2 /GL /Gw /Iinclude
LDFLAGS = /LTCG /SUBSYSTEM:CONSOLE
LIBS = user32.lib shell32.lib advapi32.lib

# Directories
SRC_DIR = src
RES_DIR = res
OBJ_DIR = build
BIN_DIR = bin

# Target
TARGET = $(BIN_DIR)\keyboard_remapper.exe

# Sources
SRCS = \
  $(SRC_DIR)\keyboard_remapper.c \
  $(SRC_DIR)\config.c \
  $(SRC_DIR)\keys.c \
  $(SRC_DIR)\debug.c \
  $(SRC_DIR)\remap.c \
  $(SRC_DIR)\mouse.c \
  $(SRC_DIR)\tray.c

# Objects
OBJS = \
  $(OBJ_DIR)\keyboard_remapper.obj \
  $(OBJ_DIR)\config.obj \
  $(OBJ_DIR)\keys.obj \
  $(OBJ_DIR)\debug.obj \
  $(OBJ_DIR)\remap.obj \
  $(OBJ_DIR)\mouse.obj \
  $(OBJ_DIR)\tray.obj \
  $(OBJ_DIR)\resource.res

# Default rule
all: dirs $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(LINK) /OUT:$@ $(OBJS) $(LIBS) $(LDFLAGS)

# Compile C files
{$(SRC_DIR)}.c{$(OBJ_DIR)}.obj:
	$(CC) $(CFLAGS) /c $< /Fo$@

# Compile resources
$(OBJ_DIR)\resource.res: $(RES_DIR)\resource.rc
	$(RC) /fo $@ /I include /I res/icons $(RES_DIR)\resource.rc

# Create directories
dirs:
	@if not exist "$(OBJ_DIR)" mkdir $(OBJ_DIR)
	@if not exist "$(BIN_DIR)" mkdir $(BIN_DIR)

# Clean
clean:
	@for %f in ($(OBJS)) do @if exist %f del /q %f 2>nul
	@if exist $(TARGET) del /q $(TARGET)
