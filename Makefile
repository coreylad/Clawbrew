# Clawbrew - PS2 Homebrew Game
# Requires ps2sdk toolchain

EE_BIN = clawbrew.elf
EE_OBJS = src/main.o

# ps2sdk paths (adjust for your installation)
PS2SDK ?= /usr/local/ps2dev/ps2sdk
EE_PREFIX = ee-

# Compiler flags
EE_CC = $(EE_PREFIX)gcc
EE_CFLAGS = -O2 -G0 -Wall -I$(PS2SDK)/include -I$(PS2SDK)/ee/include
EE_LDFLAGS = -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib
EE_LIBS = -lgskit -ldmakit -lpad -lpatches -lc -lm

# Build rules
all: $(EE_BIN)

$(EE_BIN): $(EE_OBJS)
	$(EE_CC) $(EE_CFLAGS) $(EE_LDFLAGS) -o $@ $^ $(EE_LIBS)

%.o: %.c
	$(EE_CC) $(EE_CFLAGS) -c $< -o $@

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

# Run on emulator (PCSX2 or ps2client)
run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

.PHONY: all clean run
