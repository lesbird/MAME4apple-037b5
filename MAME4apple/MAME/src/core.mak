# the core object files (without target specific objects;
# those are added in the target.mak files)
COREOBJS = $(OBJ)/version.o $(OBJ)/mame.o \
	$(OBJ)/drawgfx.o $(OBJ)/common.o $(OBJ)/usrintrf.o $(OBJ)/ui_text.o \
	$(OBJ)/cpuintrf.o $(OBJ)/memory.o $(OBJ)/timer.o $(OBJ)/palette.o \
	$(OBJ)/input.o $(OBJ)/inptport.o $(OBJ)/cheat.o $(OBJ)/unzip.o \
	$(OBJ)/audit.o $(OBJ)/info.o $(OBJ)/png.o $(OBJ)/artwork.o \
	$(OBJ)/tilemap.o $(OBJ)/sprite.o $(OBJ)/gfxobj.o \
	$(OBJ)/state.o $(OBJ)/datafile.o $(OBJ)/hiscore.o \
	$(sort $(CPUOBJS)) \
	$(OBJ)/sndintrf.o \
	$(OBJ)/sound/streams.o $(OBJ)/sound/mixer.o \
	$(sort $(SOUNDOBJS)) \
	$(OBJ)/sound/votrax.o \
	$(OBJ)/machine/z80fmly.o $(OBJ)/machine/6821pia.o \
	$(OBJ)/machine/8255ppi.o \
	$(OBJ)/vidhrdw/generic.o $(OBJ)/vidhrdw/vector.o \
	$(OBJ)/vidhrdw/avgdvg.o $(OBJ)/machine/mathbox.o \
	$(OBJ)/machine/ticket.o $(OBJ)/machine/eeprom.o \
	$(OBJ)/mamedbg.o $(OBJ)/window.o \
	$(OBJ)/profiler.o \
	$(sort $(DBGOBJS))

TOOLS = romcmp$(EXE)
TEXTS = gamelist.txt

