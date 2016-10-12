# set this to mame, mess or the destination you want to build
TARGET = mame
# TARGET = mess
# TARGET = neomame
# example for a tiny compile
# TARGET = tiny

# uncomment next line to include the debugger
# DEBUG = 1

# uncomment next line to include the symbols for symify
# SYMBOLS = 1

# uncomment next line to use Assembler 68k engine
# currently the Psikyo games don't work with it
# X86_ASM_68K = 1

# set this the operating system you're building for
# (actually you'll probably need your own main makefile anyways)
OS = msdos

# extension for executables
EXE = .exe

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

# compiler, linker and utilities
AR = @ar
CC = @gcc
LD = @gcc
#ASM = @nasm
ASM = @nasmw
ASMFLAGS = -f coff

ifdef DEBUG
NAME = $(TARGET)d
else
ifdef K6
NAME = $(TARGET)k6
ARCH = -march=k6
else
ifdef I686
NAME = $(TARGET)ppro
ARCH = -march=pentiumpro
else
NAME = $(TARGET)
ARCH = -march=pentium
endif
endif
endif

# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
# cleantiny isn't needed anymore, because the tiny build has its
# own object directory too.
OBJ = $(NAME).obj

EMULATOR = $(NAME)$(EXE)

DEFS = -DX86_ASM -DLSB_FIRST -DINLINE="static __inline__" -Dasm=__asm__

ifdef SYMBOLS
CFLAGS = -Isrc -Isrc/msdos -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000 \
	-O0 -pedantic -Wall -Werror -Wno-unused -g
else
CFLAGS = -Isrc -Isrc/msdos -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000 \
	-DNDEBUG \
	$(ARCH) -O3 -fomit-frame-pointer -fstrict-aliasing \
	-Werror -Wall -Wno-sign-compare -Wunused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-pedantic \
	-Wshadow \
	-Wstrict-prototypes
#	-W had to remove because of the "missing initializer" warning
#	-Wredundant-decls \
#	-Wlarger-than-27648 \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations
endif

ifdef SYMBOLS
LDFLAGS =
else
#LDFLAGS = -s -Wl,--warn-common
LDFLAGS = -s
endif

LIBS = -lalleg -laudio -lz

# check that the required libraries are available
ifeq ($(wildcard $(DJDIR)/lib/liballeg.a),)
noallegro:
	@echo Missing Allegro library! Get it from http://www.talula.demon.co.uk/allegro/
endif
ifeq ($(wildcard $(DJDIR)/lib/libaudio.a),)
noseal:
	@echo Missing SEAL library! Get it from http://www.egerter.com/
endif
ifeq ($(wildcard $(DJDIR)/lib/libz.a),)
nozlib:
	@echo Missing zlib library! Get it from http://www.cdrom.com/pub/infozip/zlib/
endif

#if obj subdirectory doesn't exist, create the tree before proceeding
ifeq ($(wildcard $(OBJ)),)
noobj: maketree all
endif

all:	$(EMULATOR) extra

# include the various .mak files
include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/$(OS)/$(OS).mak

ifdef DEBUG
DBGDEFS = -DMAME_DEBUG
else
DBGDEFS =
DBGOBJS =
endif

extra:	romcmp$(EXE) $(TOOLS) $(TEXTS)

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS)

$(EMULATOR): $(OBJS) $(COREOBJS) $(OSOBJS) $(LIBS) $(DRVLIBS)
# always recompile the version string
	$(CC) $(CDEFS) $(CFLAGS) -c src/version.c -o $(OBJ)/version.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OBJS) $(COREOBJS) $(OSOBJS) $(LIBS) $(DRVLIBS) -o $@
ifndef DEBUG
	upx $(EMULATOR)
endif

romcmp$(EXE): $(OBJ)/romcmp.o $(OBJ)/unzip.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@

$(OBJ)/%.o: src/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

# compile generated C files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE)
	@echo Compiling $(subst .o,.c,$@)...
	$(CC) $(CDEFS) $(CFLAGS) -c $*.c -o $@

# additional rule, because m68kcpu.c includes the generated m68kops.h :-/
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE)

# generate C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kmake$(EXE): src/cpu/m68000/m68kmake.c
	@echo M68K make $<...
	$(CC) $(CDEFS) $(CFLAGS) -DDOS -o $(OBJ)/cpu/m68000/m68kmake$(EXE) $<
	@echo Generating M68K source files...
	$(OBJ)/cpu/m68000/m68kmake $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# generate asm source files for the 68000 emulator
$(OBJ)/cpu/m68000/68kem.asm:  src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	@$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/comptab.asm

# generated asm files for the 68000 emulator
$(OBJ)/cpu/m68000/68kem.o:  $(OBJ)/cpu/m68000/68kem.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/%.a:
	@echo Archiving $@...
	$(AR) cr $@ $^

makedir:
	@echo make makedir is no longer necessary, just type make

maketree:
	@echo Making MAME object tree in $(OBJ)...
	@md $(OBJ)
	@md $(OBJ)\cpu
	@md $(OBJ)\cpu\z80
	@md $(OBJ)\cpu\z80gb
	@md $(OBJ)\cpu\m6502
	@md $(OBJ)\cpu\h6280
	@md $(OBJ)\cpu\i86
	@md $(OBJ)\cpu\nec
	@md $(OBJ)\cpu\i8039
	@md $(OBJ)\cpu\i8085
	@md $(OBJ)\cpu\m6800
	@md $(OBJ)\cpu\m6805
	@md $(OBJ)\cpu\m6809
	@md $(OBJ)\cpu\konami
	@md $(OBJ)\cpu\m68000
	@md $(OBJ)\cpu\s2650
	@md $(OBJ)\cpu\t11
	@md $(OBJ)\cpu\tms34010
	@md $(OBJ)\cpu\tms9900
	@md $(OBJ)\cpu\z8000
	@md $(OBJ)\cpu\tms32010
	@md $(OBJ)\cpu\ccpu
	@md $(OBJ)\cpu\adsp2100
	@md $(OBJ)\cpu\pdp1
	@md $(OBJ)\cpu\mips
	@md $(OBJ)\cpu\sc61860
	@md $(OBJ)\sound
	@md $(OBJ)\msdos
	@md $(OBJ)\drivers
	@md $(OBJ)\machine
	@md $(OBJ)\vidhrdw
	@md $(OBJ)\sndhrdw
ifdef MESS
	@echo Making MESS object tree in $(OBJ)\mess...
	@md $(OBJ)\mess
	@md $(OBJ)\mess\systems
	@md $(OBJ)\mess\machine
	@md $(OBJ)\mess\vidhrdw
	@md $(OBJ)\mess\sndhrdw
	@md $(OBJ)\mess\tools
endif

clean:
	@echo Deleting object tree $(OBJ)...
	deltree /Y $(OBJ)
	@echo Deleting $(EMULATOR)...
	@del $(EMULATOR)

cleandebug:
	@echo Deleting debug obj tree...
	@del $(OBJ)\*.o
	@del $(OBJ)\cpu\z80\*.o
	@del $(OBJ)\cpu\z80gb\*.o
	@del $(OBJ)\cpu\m6502\*.o
	@del $(OBJ)\cpu\h6280\*.o
	@del $(OBJ)\cpu\i86\*.o
	@del $(OBJ)\cpu\nec\*.o
	@del $(OBJ)\cpu\i8039\*.o
	@del $(OBJ)\cpu\i8085\*.o
	@del $(OBJ)\cpu\m6800\*.o
	@del $(OBJ)\cpu\m6805\*.o
	@del $(OBJ)\cpu\m6809\*.o
	@del $(OBJ)\cpu\konami\*.o
	@del $(OBJ)\cpu\m68000\*.o
	@del $(OBJ)\cpu\m68000\*.c
	@del $(OBJ)\cpu\s2650\*.o
	@del $(OBJ)\cpu\t11\*.o
	@del $(OBJ)\cpu\tms34010\*.o
	@del $(OBJ)\cpu\tms9900\*.o
	@del $(OBJ)\cpu\z8000\*.o
	@del $(OBJ)\cpu\tms32010\*.o
	@del $(OBJ)\cpu\ccpu\*.o
	@del $(OBJ)\cpu\adsp2100\*.o
	@del $(OBJ)\cpu\pdp1\*.o
	@del $(OBJ)\cpu\mips\*.o
	@del $(EMULATOR)

