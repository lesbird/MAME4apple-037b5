# only MS-DOS specific output files and rules
OSOBJS = $(OBJ)/msdos/msdos.o $(OBJ)/msdos/video.o $(OBJ)/msdos/blit.o $(OBJ)/msdos/asmblit.o \
	$(OBJ)/msdos/gen15khz.o $(OBJ)/msdos/ati15khz.o \
	$(OBJ)/msdos/sound.o $(OBJ)/msdos/input.o $(OBJ)/msdos/fileio.o \
	$(OBJ)/msdos/ticker.o $(OBJ)/msdos/config.o $(OBJ)/msdos/fronthlp.o

# video blitting functions
$(OBJ)/msdos/asmblit.o: src/msdos/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# check if this is a MESS build
ifdef MESS
# additional OS specific object files
OSOBJS += $(OBJ)/mess/msdos.o $(OBJ)/msdos/nec765.o
endif

