# --- Compiler and Tools ---
CC = gcc
AR = ar
ARFLAGS = rcs
RM = rm -f

# --- Directories ---
CJSON_DIR = ../.pio/libdeps/display-emulator-64bits/cJSON/

# --- Compiler and Linker Flags ---
# Base Flags (Warnings, Debug Symbols, Optimization)
CFLAGS_BASE = -Wall -Wextra -g -O2
# Include Paths
CFLAGS_INC = -I. -I$(LVGL_DIR) -I$(LVDRIVERS_DIR) -I$(CJSON_DIR)
# Put them together
CFLAGS = $(CFLAGS_BASE) $(CFLAGS_INC)

# Linker Flags (Library Paths, etc. - often empty for static linking)
LDFLAGS =
# Base Libraries (Math is often needed)
LIBS_BASE = -lm

# --- Source Files ---

# cJSON (needed by both viewer and emulation lib client linkage)
CJSON_SRC = $(CJSON_DIR)/cJSON.c
CJSON_OBJ = $(CJSON_SRC:.c=.o)

# LVGL Emulation Library
EMUL_SRC = emul_lvgl.c
EMUL_OBJ = $(EMUL_SRC:.c=.o)
EMUL_LIB = lvgl_emul.a

CLIENT_SRC = client_main.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
CLIENT_EXE = my_client_app
# Client links against the emulation lib and cJSON object
CLIENT_LIBS = $(LIBS_BASE) # Client might not need SDL, etc.

# --- Targets ---

# Default target: Build the viewer and the emulation library
.PHONY: all
all: $(EMUL_LIB)

# Target to build only the emulation library
.PHONY: emul_lib
emul_lib: $(EMUL_LIB)

# Target to build only the example client
.PHONY: client
client: $(CLIENT_EXE)

# --- Rules ---

# Build the LVGL Emulation static library
# Note: It does *not* include cJSON - cJSON must be linked by the final executable.
$(EMUL_LIB): $(EMUL_OBJ)
	@echo "Archiving $@..."
	$(AR) $(ARFLAGS) $@ $(EMUL_OBJ)
	@echo "$@ built successfully."

# Build the Example Client executable
# Links client object file, cJSON object file, and the emulation library
$(CLIENT_EXE): $(CLIENT_OBJ) $(CJSON_OBJ) $(EMUL_LIB)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) $(CLIENT_OBJ) $(CJSON_OBJ) $(EMUL_LIB) -o $@ $(CLIENT_LIBS)
	@echo "$@ built successfully."


# Generic rule to compile C source files into object files
# Apply viewer-specific CFLAGS only when compiling viewer sources
$(filter %.o, $(VIEWER_OBJ)): %.o: %.c
	@echo "Compiling $< -> $@"
	$(CC) $(VIEWER_CFLAGS) -c $< -o $@

# Generic rule for other C sources (emulation lib, client, cJSON)
# Excludes viewer sources which have their own rule above
$(filter-out $(VIEWER_OBJ), $(EMUL_OBJ) $(CLIENT_OBJ) $(CJSON_OBJ)): %.o: %.c
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@


# --- Cleanup ---
.PHONY: clean
clean:
	@echo "Cleaning build files..."
	$(RM) $(VIEWER_EXE) $(CLIENT_EXE) $(EMUL_LIB)
	$(RM) $(VIEWER_OBJ) $(CLIENT_OBJ) $(EMUL_OBJ) $(CJSON_OBJ)
	@echo "Clean complete."
