/*
  Taito L-System

  Monoprocessor games (1 main z80, no sound z80)
  - Plotting
  - Puzznic
  - Palamedes
  - Cachat
  - American Horseshoes

  Dual processor games (2 main z80, 1 sound z80)
  - Fighting hawk
  - Raimais
  - Champion Wrestler

Notes:
- the system uses RAM based characters, which aren't really supported by the
  TileMap system, so we have to tilemap_mark_all_tiles_dirty() to compensate

TODO:
- sound in the multiprocessor games
- American horseshoes doesn't  have the trackball hooked  up.  X and Y
  are 9bits values each splitted  in two bytes.   Probably easy to add
  too.
- The  puzznic protection is worked around,  but I'm not happy with it
  (the 68705-returned values are wrong, I'm sure of that).
- A bunch of control registers are simply ignored
- The source of   irqs 0 and  1 is  ignored, while  2 is vblank  (0 is
  usually   ignored  by the  program,    1   leads  to  reading    the
  ports... maybe vbl-in, vbl-out and hblank ?).
- Raimais is broken.  Probably something stupid, I had it working some
  time ago.
- There may still  be some issues of  priorities in fighting  hawk, at
  the beginning of the first level (I didn't play the game far).
- Cachat's title    screen  has some  problems.   The middle tile layer
  is shifted 16 pixels to the left.


Ok,  the pipe  is   what is   used to  communicate   between the  main
processors and the sound  one.  It sits  on two adresses, the even one
being the register number, the odd data, in a very ym2149 kind of way.

Registers 0-3 are data,  register 4 is  flags.  They probably are only
4bits wide.

Each time  you read or   write to a   register the register  number is
automatically increased.  I have no reason to think that this register
number needs  to be separated for the  two processors  that access it,
given the usage patterns.

Flags of register 4 (it seems to be possible to directly write to it)
bit 0 : set to 1 when register 1 is written, set to 0 when it is read
bit 1 : set to 1 when register 3 is written, set to 0 when it is read
bit 2-3 : unknown (there may be some kind of overrun detection...)

Also, I think that  setting a bit in  the register 4  raises an irq on
the sound CPU.    I tend to think  that  even setting it thru   direct
writing does that.

Here are the adresses of the pipe for the different games:

game main cpu sound cpu
fhawk 2nd, c800 e000
raimais 1st, 8c00 e200
champwr 2nd, e800 a000


> Note: the multicpu games run very slow (w/ emulation speed at 100%). The
> interleaving factor most certainly has to be increased (since there is
> shared memory).

This  may even  help raimais.   The program on  the  second CPU is the
weirdest I've ever seen.   Feel a bit like  a  z80 rewrite of a  68705
protection program...

*/


#include "driver.h"
#include "cpu/z80/z80.h"

int taitol_vh_start(void);
void taitol_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void taitol_chardef14_m(int offset);
void taitol_chardef15_m(int offset);
void taitol_chardef16_m(int offset);
void taitol_chardef17_m(int offset);
void taitol_chardef1c_m(int offset);
void taitol_chardef1d_m(int offset);
void taitol_chardef1e_m(int offset);
void taitol_chardef1f_m(int offset);
void taitol_bg18_m(int offset);
void taitol_bg19_m(int offset);
void taitol_char1a_m(int offset);
void taitol_obj1b_m(int offset);

WRITE_HANDLER( taitol_control_w );
READ_HANDLER( taitol_control_r );
WRITE_HANDLER( taitol_bankg_w );
READ_HANDLER( taitol_bankg_r );
WRITE_HANDLER( taitol_bankc_w );
READ_HANDLER( taitol_bankc_r );

static void (*rambank_modify_notifiers[12])(int) = {
	taitol_chardef14_m,	// 14
	taitol_chardef15_m,	// 15
	taitol_chardef16_m,	// 16
	taitol_chardef17_m,	// 17

	taitol_bg18_m,		// 18
	taitol_bg19_m,		// 19
	taitol_char1a_m,	// 1a
	taitol_obj1b_m,		// 1b

	taitol_chardef1c_m,	// 1c
	taitol_chardef1d_m,	// 1d
	taitol_chardef1e_m,	// 1e
	taitol_chardef1f_m,	// 1f
};

static void (*current_notifier[4])(int);
static unsigned char *current_base[4];

static int cur_rombank, cur_rombank2, cur_rambank[4];
static int irq_adr_table[3];
static int irq_enable = 0;

unsigned char *taitol_rambanks;
int taitol_bg18_deltax, taitol_bg19_deltax;

static unsigned char *palette_ram;
static unsigned char *empty_ram;
static unsigned char *shared_ram;

static mem_read_handler porte0_r;
static mem_read_handler porte1_r;
static mem_read_handler portf0_r;
static mem_read_handler portf1_r;

static void palette_notifier(int addr)
{
	unsigned char *p = palette_ram + (addr & ~1);
	unsigned char byte0 = *p++;
	unsigned char byte1 = *p;

	unsigned int b = (byte1 & 0xf) * 0x11;
	unsigned int g = ((byte0 & 0xf0)>>4) * 0x11;
	unsigned int r = (byte0 & 0xf) * 0x11;

	//	addr &= 0x1ff;

	if(addr > 0x200) {
logerror("Large palette ? %03x (%04x)\n", addr, cpu_get_pc());
	} else {
		//		r = g = b = ((addr & 0x1e) != 0)*255;
		palette_change_color(addr/2, r, g, b);
	}
}

static void machine_init(void)
{
	int i;

	taitol_rambanks = malloc(0x1000*12);
	palette_ram = malloc(0x1000);
	empty_ram = malloc(0x1000);

	for(i=0;i<3;i++)
		irq_adr_table[i] = 0;

	irq_enable = 0;

	for(i=0;i<4;i++) {
		cur_rambank[i] = 0x80;
		current_base[i] = palette_ram;
		current_notifier[i] = palette_notifier;
		cpu_setbank(2+i, current_base[i]);
	}
	cur_rombank = cur_rombank2 = 0;
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000);

	for(i=0;i<512;i++) {
		decodechar(Machine->gfx[2], i, taitol_rambanks,
				   Machine->drv->gfxdecodeinfo[2].gfxlayout);
		decodechar(Machine->gfx[2], i+512, taitol_rambanks + 0x4000,
				   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	}

	taitol_bg18_deltax = 0x1e4;
	taitol_bg19_deltax = 0x1da;
}

static void puzznic_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static void plotting_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static void palamed_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = 0;
	portf0_r = input_port_1_r;
	portf1_r = 0;
}

static void cachat_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = 0;
	portf0_r = input_port_1_r;
	portf1_r = 0;

	taitol_bg18_deltax = 0x1e4 + 51;
	taitol_bg19_deltax = 0x1da + 64;
}

static void horshoes_init(void)
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static void fhawk_init(void)
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static void raimais_init(void)
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static void champwr_init(void)
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}


static void irq0(int v)
{
	cpu_cause_interrupt(0, irq_adr_table[0]);
}

static void irq1(int v)
{
	cpu_cause_interrupt(0, irq_adr_table[1]);
}

static int vbl_interrupt(void)
{
	// What is really generating interrupts 0 and 1 is still to be found

	if (irq_enable & 1)
		timer_set(TIME_IN_CYCLES(20000,0), 0, irq0);

	if (irq_enable & 2)
		timer_set(TIME_IN_CYCLES(10000,0), 0, irq1);

	return (irq_enable & 4) ? irq_adr_table[2] : Z80_IGNORE_INT;
}

static WRITE_HANDLER( irq_adr_w )
{
//logerror("irq_adr_table[%d] = %02x\n",offset,data);
	irq_adr_table[offset] = data;
}

static READ_HANDLER( irq_adr_r )
{
	return irq_adr_table[offset];
}

static WRITE_HANDLER( irq_enable_w )
{
	irq_enable = data;
}

static READ_HANDLER( irq_enable_r )
{
	return irq_enable;
}


static WRITE_HANDLER( rombankswitch_w )
{
	static int high = 0;
	if(cur_rombank != data) {
		if(data>high) {
			high = data;
			logerror("New rom size : %x\n", (high+1)*0x2000);
		}

//		logerror("robs %d, %02x (%04x)\n", offset, data, cpu_get_pc());
		cur_rombank = data;
		cpu_setbank(1, memory_region(REGION_CPU1)+0x10000+0x2000*cur_rombank);
	}
}

static WRITE_HANDLER( rombank2switch_w )
{
	static int high = 0;

	data &= 0xf;

	if(cur_rombank2 != data) {
		if(data>high) {
			high = data;
			logerror("New rom2 size : %x\n", (high+1)*0x4000);
		}

//		logerror("robs2 %02x (%04x)\n", data, cpu_get_pc());

		cur_rombank2 = data;
		cpu_setbank(6, memory_region(REGION_CPU2)+0x10000+0x4000*cur_rombank2);
	}
}

static READ_HANDLER( rombankswitch_r )
{
	return cur_rombank;
}

static READ_HANDLER( rombank2switch_r )
{
	return cur_rombank2;
}

static WRITE_HANDLER( rambankswitch_w )
{
	if(cur_rambank[offset]!=data)
	{
		cur_rambank[offset]=data;
//logerror("rabs %d, %02x (%04x)\n", offset, data, cpu_get_pc());
		if(data>=0x14 && data<=0x1f)
		{
			data -= 0x14;
			current_notifier[offset] = rambank_modify_notifiers[data];
			current_base[offset] = taitol_rambanks+0x1000*data;
		} else if (data == 0x80)
		{
			current_notifier[offset] = palette_notifier;
			current_base[offset] = palette_ram;
		}
		else
		{
logerror("unknown rambankswitch %d, %02x (%04x)\n", offset, data, cpu_get_pc());
			current_notifier[offset] = 0;
			current_base[offset] = empty_ram;
		}
		cpu_setbank(2+offset, current_base[offset]);
	}
}

static READ_HANDLER( rambankswitch_r )
{
	return cur_rambank[offset];
}

static WRITE_HANDLER( bank0_w )
{
	if(current_base[0][offset]!=data) {
		current_base[0][offset] = data;
		if(current_notifier[0])
			current_notifier[0](offset);
	}
}

static WRITE_HANDLER( bank1_w )
{
	if(current_base[1][offset]!=data) {
		current_base[1][offset] = data;
		if(current_notifier[1])
			current_notifier[1](offset);
	}
}

static WRITE_HANDLER( bank2_w )
{
	if(current_base[2][offset]!=data) {
		current_base[2][offset] = data;
		if(current_notifier[2])
			current_notifier[2](offset);
	}
}

static WRITE_HANDLER( bank3_w )
{
	if(current_base[3][offset]!=data) {
		current_base[3][offset] = data;
		if(current_notifier[3])
			current_notifier[3](offset);
	}
}

static WRITE_HANDLER( control2_w )
{
}

static int extport;

static READ_HANDLER( portA_r )
{
	if (extport == 0) return porte0_r(0);
	else return porte1_r(0);
}

static READ_HANDLER( portB_r )
{
	if (extport == 0) return portf0_r(0);
	else return portf1_r(0);
}

static READ_HANDLER( ym2203_data0_r )
{
	extport = 0;
	return YM2203_read_port_0_r(offset);
}

static READ_HANDLER( ym2203_data1_r )
{
	extport = 1;
	return YM2203_read_port_0_r(offset);
}

static int *mcu_reply;
static int mcu_pos = 0, mcu_reply_len = 0;
static int last_data_adr, last_data;

static int puzznic_mcu_reply[] = { 0x50, 0x1f, 0xb6, 0xba, 0x06, 0x03, 0x47, 0x05, 0x00 };

static WRITE_HANDLER( mcu_data_w )
{
	last_data = data;
	last_data_adr = cpu_get_pc();
//	logerror("mcu write %02x (%04x)\n", data, cpu_get_pc());
	switch(data) {
	case 0x43:
		mcu_pos = 0;
		mcu_reply = puzznic_mcu_reply;
		mcu_reply_len = sizeof(puzznic_mcu_reply);
		break;
	}
}

static WRITE_HANDLER( mcu_control_w )
{
//	logerror("mcu control %02x (%04x)\n", data, cpu_get_pc());
}

static READ_HANDLER( mcu_data_r )
{
//	logerror("mcu read (%04x) [%02x, %04x]\n", cpu_get_pc(), last_data, last_data_adr);
	if(mcu_pos==mcu_reply_len)
		return 0;

	return mcu_reply[mcu_pos++];
}

static READ_HANDLER( mcu_control_r )
{
//	logerror("mcu control read (%04x)\n", cpu_get_pc());
	return 0x1;
}

static WRITE_HANDLER( sound_w )
{
	logerror("Sound_w %02x (%04x)\n", data, cpu_get_pc());
}

static READ_HANDLER( shared_r )
{
	return shared_ram[offset];
}

static WRITE_HANDLER( shared_w )
{
	shared_ram[offset] = data;
}

static int mux_ctrl = 0;

static READ_HANDLER( mux_r )
{
	switch(mux_ctrl) {
	case 0:
		return input_port_0_r(0);
	case 1:
		return input_port_1_r(0);
	case 2:
		return input_port_2_r(0);
	case 3:
		return input_port_3_r(0);
	case 7:
		return input_port_4_r(0);
	default:
		logerror("Mux read from unknown port %d (%04x)\n", mux_ctrl, cpu_get_pc());
		return 0xff;
	}
}

static WRITE_HANDLER( mux_w )
{
	switch(mux_ctrl) {
	case 4:
		control2_w(0, data);
		break;
	default:
		logerror("Mux read to unknown port %d, %02x (%04x)\n", mux_ctrl, data, cpu_get_pc());
	}
}

static WRITE_HANDLER( mux_ctrl_w )
{
	mux_ctrl = data;
}


#define COMMON_BANKS_READ \
	{ 0x0000, 0x5fff, MRA_ROM },			\
	{ 0x6000, 0x7fff, MRA_BANK1 },			\
	{ 0xc000, 0xcfff, MRA_BANK2 },			\
	{ 0xd000, 0xdfff, MRA_BANK3 },			\
	{ 0xe000, 0xefff, MRA_BANK4 },			\
	{ 0xf000, 0xfdff, MRA_BANK5 },			\
	{ 0xfe00, 0xfe03, taitol_bankc_r },		\
	{ 0xfe04, 0xfe04, taitol_control_r },	\
	{ 0xff00, 0xff02, irq_adr_r },			\
	{ 0xff03, 0xff03, irq_enable_r },		\
	{ 0xff04, 0xff07, rambankswitch_r },	\
	{ 0xff08, 0xff08, rombankswitch_r }

#define COMMON_BANKS_WRITE \
	{ 0x0000, 0x7fff, MWA_ROM },			\
	{ 0xc000, 0xcfff, bank0_w },			\
	{ 0xd000, 0xdfff, bank1_w },			\
	{ 0xe000, 0xefff, bank2_w },			\
	{ 0xf000, 0xfdff, bank3_w },			\
	{ 0xfe00, 0xfe03, taitol_bankc_w },		\
	{ 0xfe04, 0xfe04, taitol_control_w },	\
	{ 0xff00, 0xff02, irq_adr_w },			\
	{ 0xff03, 0xff03, irq_enable_w },		\
	{ 0xff04, 0xff07, rambankswitch_w },	\
	{ 0xff08, 0xff08, rombankswitch_w }

#define COMMON_SINGLE_READ \
	{ 0xa000, 0xa000, YM2203_status_port_0_r },	\
	{ 0xa001, 0xa001, ym2203_data0_r },			\
	{ 0xa003, 0xa003, ym2203_data1_r },			\
	{ 0x8000, 0x9fff, MRA_RAM }

#define COMMON_SINGLE_WRITE \
	{ 0xa000, 0xa000, YM2203_control_port_0_w },	\
	{ 0xa001, 0xa001, YM2203_write_port_0_w },		\
	{ 0x8000, 0x9fff, MWA_RAM }

#define COMMON_DUAL_READ \
	{ 0x8000, 0x9fff, MRA_RAM, &shared_ram  }, \
	{ 0xa000, 0xbfff, MRA_RAM, &shared2_ram }

#define COMMON_DUAL_WRITE \
	{ 0x8000, 0xbfff, MWA_RAM }

static struct MemoryReadAddress puzznic_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, MRA_NOP },	// Watchdog
	{ 0xb000, 0xb7ff, MRA_RAM },	// Wrong, used to overcome protection
	{ 0xb800, 0xb800, mcu_data_r },
	{ 0xb801, 0xb801, mcu_control_r },
	{ -1 }
};

static struct MemoryWriteAddress puzznic_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xb000, 0xb7ff, MWA_RAM },	// Wrong, used to overcome protection
	{ 0xb800, 0xb800, mcu_data_w },
	{ 0xb801, 0xb801, mcu_control_w },
	{ 0xbc00, 0xbc00, MWA_NOP },	// Control register, function unknown
	{ -1 }
};


static struct MemoryReadAddress plotting_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ -1 }
};

static struct MemoryWriteAddress plotting_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa800, 0xa800, MWA_NOP },	// Watchdog or interrupt ack
	{ 0xb800, 0xb800, MWA_NOP },	// Control register, function unknown
	{ -1 }
};


static struct MemoryReadAddress palamed_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xa801, 0xa801, input_port_3_r },
	{ 0xa802, 0xa802, input_port_4_r },
	{ 0xb001, 0xb001, MRA_NOP },	// Watchdog or interrupt ack
	{ -1 }
};

static struct MemoryWriteAddress palamed_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa803, 0xa803, MWA_NOP },	// Control register, function unknown
	{ 0xb000, 0xb000, MWA_NOP },	// Control register, function unknown (copy of 8822)
	{ -1 }
};


static struct MemoryReadAddress cachat_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xa801, 0xa801, input_port_3_r },
	{ 0xa802, 0xa802, input_port_4_r },
	{ 0xb001, 0xb001, MRA_NOP },	// Watchdog or interrupt ack (value ignored)
	{ 0xfff8, 0xfff8, rombankswitch_r },
	{ -1 }
};

static struct MemoryWriteAddress cachat_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa803, 0xa803, MWA_NOP },	// Control register, function unknown
	{ 0xb000, 0xb000, MWA_NOP },	// Control register, function unknown
	{ 0xfff8, 0xfff8, rombankswitch_w },
	{ -1 }
};


static struct MemoryReadAddress horshoes_readmem[] = {
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_4_r },
	{ 0xa804, 0xa804, input_port_5_r },
	{ 0xa808, 0xa808, input_port_6_r },
	{ 0xa80c, 0xa80c, input_port_7_r },
	{ 0xb801, 0xb801, MRA_NOP },	// Watchdog or interrupt ack
	{ -1 }
};

static struct MemoryWriteAddress horshoes_writemem[] = {
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xb802, 0xb802, taitol_bankg_w },
	{ 0xbc00, 0xbc00, MWA_NOP },
	{ -1 }
};



static struct MemoryReadAddress fhawk_readmem[] = {
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress fhawk_writemem[] = {
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x9fff, MWA_RAM, &shared_ram },
	{ 0xa000, 0xbfff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress fhawk_2_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK6 },
	{ 0xc800, 0xc801, MRA_NOP }, // Pipe
	{ 0xe000, 0xffff, shared_r },
	{ 0xd000, 0xd000, input_port_0_r },
	{ 0xd001, 0xd001, input_port_1_r },
	{ 0xd002, 0xd002, input_port_2_r },
	{ 0xd003, 0xd003, input_port_3_r },
	{ 0xd007, 0xd007, input_port_4_r },
	{ -1 }
};

static struct MemoryWriteAddress fhawk_2_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, rombank2switch_w },
	{ 0xc800, 0xc801, MWA_NOP }, // Pipe
	{ 0xd000, 0xd000, MWA_NOP },	// Direct copy of input port 0
	{ 0xd004, 0xd004, control2_w },
	{ 0xd005, 0xd006, MWA_NOP },	// Always 0
	{ 0xe000, 0xffff, shared_w },
	{ -1 }
};

static struct MemoryReadAddress fhawk_3_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xe000, 0xe001, MRA_NOP }, // Pipe
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ -1 }
};

static struct MemoryWriteAddress fhawk_3_writemem[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xe000, 0xe001, MWA_NOP }, // Pipe
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ -1 }
};

static struct MemoryReadAddress raimais_readmem[] = {
	COMMON_BANKS_READ,
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, mux_r },
	{ 0x8801, 0x8801, MRA_NOP },	// Watchdog or interrupt ack (value ignored)
	{ 0x8c00, 0x8c01, MRA_NOP }, // Pipe
	{ 0xa000, 0xbfff, MRA_RAM },
	{ -1 }
};
static struct MemoryWriteAddress raimais_writemem[] = {
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x87ff, MWA_RAM, &shared_ram },
	{ 0x8800, 0x8800, mux_w },
	{ 0x8801, 0x8801, mux_ctrl_w },
	{ 0x8c00, 0x8c01, MWA_NOP }, // Pipe
	{ 0xa000, 0xbfff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress raimais_2_readmem[] = {
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, shared_r },
	{ -1 }
};

static struct MemoryWriteAddress raimais_2_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, shared_w },
	{ -1 }
};

static struct MemoryReadAddress raimais_3_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2203_status_port_0_r },
	{ 0xe200, 0xe201, MRA_NOP }, // Pipe
	{ -1 }
};

static struct MemoryWriteAddress raimais_3_writemem[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe200, 0xe201, MWA_NOP }, // Pipe
	{ -1 }
};


static struct MemoryReadAddress champwr_readmem[] = {
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ -1 }
};


static struct MemoryWriteAddress champwr_writemem[] = {
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_RAM, &shared_ram },
	{ -1 }
};

static struct MemoryReadAddress champwr_2_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, shared_r },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, input_port_1_r },
	{ 0xe002, 0xe002, input_port_2_r },
	{ 0xe003, 0xe003, input_port_3_r },
	{ 0xe007, 0xe007, input_port_4_r },
	{ 0xe008, 0xe00f, MRA_NOP },
	{ 0xf000, 0xf000, rombank2switch_r },
	{ -1 }
};

static struct MemoryWriteAddress champwr_2_writemem[] = {
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, shared_w },
	{ 0xe000, 0xe000, MWA_NOP },	// Watchdog
	{ 0xe004, 0xe004, control2_w },
	{ 0xf000, 0xf000, rombank2switch_w },
	{ -1 }
};

static struct MemoryReadAddress champwr_3_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa000, 0xa001, MRA_NOP }, // Pipe
	{ -1 }
};

static struct MemoryWriteAddress champwr_3_writemem[] = {
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa001, MWA_NOP }, // Pipe
	{ -1 }
};



INPUT_PORTS_START( puzznic )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPSETTING(    0x01, "50" )
	PORT_DIPSETTING(    0x03, "60" )
	PORT_DIPSETTING(    0x02, "70" )
	PORT_DIPNAME( 0x0c, 0x0c, "Retries" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x10, 0x10, "Bombs" )
	PORT_DIPSETTING(    0x10, "0" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x20, 0x20, "Girls" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
    /* These two last dips seems to be dependant */
    /* I think that when 0x40 is 0x00, 0x80 should be 0x80 */
    /* and when 0x40 is 0x40, 0x80 should be 0x00 */
    /* Maybe these two dips could be joined with on two values: */
    /* Retry from Begining with Timer Reset */
    /* Retry Before Last Explosion without Reseting Timer */
	PORT_DIPNAME( 0x40, 0x00, "Reset Timer on Retry" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Retry" )
	PORT_DIPSETTING(    0x80, "From Begining" )
	PORT_DIPSETTING(    0x00, "Before Last Explosion" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* Not read yet. Coin 2 and Tilt are probably here */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( plotting )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Max Players" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Misses" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( palamed )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPSETTING(    0x01, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Versus Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cachat )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( horshoes )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0a" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1a" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2a" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3a" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4a" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5a" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6a" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7a" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0b" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1b" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2b" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3b" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4b" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5b" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6b" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7b" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0c" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1c" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2c" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3c" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4c" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5c" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6c" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7c" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Bit 0d" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bit 1d" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit 2d" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Bit 3d" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Bit 4d" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bit 5d" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Bit 6d" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit 7d" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( fhawk )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 4 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START( raimais )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

INPUT_PORTS_START( champwr )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(    0x08, "2 minutes" )
	PORT_DIPSETTING(    0x0c, "3 minutes" )
	PORT_DIPSETTING(    0x04, "4 minutes" )
	PORT_DIPSETTING(    0x00, "5 minutes" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

 	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
INPUT_PORTS_END




static struct GfxLayout bg1_layout =
{
	8, 8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*8*2
};

static struct GfxLayout bg2_layout =
{
	8, 8,
	49152,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

#define O 8*8*2
#define O2 2*O
static struct GfxLayout sp1_layout =
{
	16, 16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0, O+3, O+2, O+1, O+0, O+8+3, O+8+2, O+8+1, O+8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, O2+0*16, O2+1*16, O2+2*16, O2+3*16, O2+4*16, O2+5*16, O2+6*16, O2+7*16 },
	8*8*2*4
};
#undef O
#undef O2

#define O 8*8*4
#define O2 2*O
static struct GfxLayout sp2_layout =
{
	16, 16,
	49152/4,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16, O+3, O+2, O+1, O+0, O+19, O+18, O+17, O+16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, O2+0*32, O2+1*32, O2+2*32, O2+3*32, O2+4*32, O2+5*32, O2+6*32, O2+7*32 },
	8*8*4*4
};
#undef O
#undef O2

static struct GfxLayout char_layout =
{
	8, 8,
	1024,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo1[] =
{
	{ REGION_GFX1, 0, &bg1_layout, 0, 16 },
	{ REGION_GFX1, 0, &sp1_layout, 0, 16 },
	{ 0,           0, &char_layout,  0, 16 },  // Ram-based
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo2[] =
{
	{ REGION_GFX1, 0, &bg2_layout, 0, 16 },
	{ REGION_GFX1, 0, &sp2_layout, 0, 16 },
	{ 0,           0, &char_layout,  0, 16 },  // Ram-based
	{ -1 }
};



static struct YM2203interface ym2203_interface_single =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(50,50) },
	{ portA_r },
	{ portB_r },
	{ 0 },
	{ 0 },
	{ 0 }
};


static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( portA_w )
{
}

static struct YM2203interface ym2203_interface_double =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ portA_w },
	{ 0 },
	{ irqhandler }
};



#define MCH_SINGLE(name) \
static struct MachineDriver machine_driver_##name =	\
{														\
	{													\
		{												\
			CPU_Z80,									\
			3332640,	/* ? xtal is 13.33056 */		\
			name ## _readmem, name ## _writemem, 0, 0,	\
			vbl_interrupt, 1							\
		}												\
	},													\
	60, DEFAULT_60HZ_VBLANK_DURATION,					\
	1,													\
	name ## _init,										\
														\
	320, 256, { 0, 319, 16, 239 },						\
	gfxdecodeinfo1,										\
	256, 256, 0,										\
														\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,			\
	0,													\
	taitol_vh_start,									\
	0,													\
	taitol_vh_screenrefresh,							\
														\
	0,0,0,0,											\
	{													\
		{												\
			SOUND_YM2203,								\
			&ym2203_interface_single					\
		}												\
	}													\
};

#define MCH_DOUBLE(name) \
static struct MachineDriver machine_driver_##name =		\
{															\
	{														\
		{													\
			CPU_Z80,										\
			3332640,	/* ? xtal is 13.33056 */			\
			name ## _readmem, name ## _writemem, 0, 0,		\
			vbl_interrupt, 1								\
		},													\
		{													\
			CPU_Z80,										\
			3332640,	/* ? xtal is 13.33056 */			\
			name ## _2_readmem, name ## _2_writemem, 0, 0,	\
			interrupt, 1									\
		},													\
		{													\
			CPU_Z80 | CPU_AUDIO_CPU,						\
			3332640,	/* ? xtal is 13.33056 */			\
			name ## _3_readmem, name ## _3_writemem, 0, 0,	\
			ignore_interrupt, 0								\
		}													\
	},														\
	60, DEFAULT_60HZ_VBLANK_DURATION,						\
	1,														\
	name ## _init,											\
															\
	320, 256, { 0, 319, 16, 239 },							\
	gfxdecodeinfo2,											\
	256, 256, 0,											\
															\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,				\
	0,														\
	taitol_vh_start,										\
	0,														\
	taitol_vh_screenrefresh,								\
															\
	0,0,0,0,												\
	{														\
		{													\
			SOUND_YM2203,									\
			&ym2203_interface_double						\
		}													\
	}														\
};


MCH_DOUBLE(fhawk)
MCH_DOUBLE(raimais)
MCH_DOUBLE(champwr)

MCH_SINGLE(puzznic)
MCH_SINGLE(plotting)
MCH_SINGLE(palamed)
MCH_SINGLE(horshoes)
MCH_SINGLE(cachat)



ROM_START( fhawk )
	ROM_REGION( 0xb0000, REGION_CPU1 )
	ROM_LOAD( "b70-07.bin", 0x00000, 0x20000, 0x939114af )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "b70-03.bin", 0x30000, 0x80000, 0x42d5a9b8 )

	ROM_REGION( 0x30000, REGION_CPU2 )
	ROM_LOAD( "b70-08.bin", 0x00000, 0x20000, 0x4d795f48 )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU3 )
	ROM_LOAD( "b70-09.bin", 0x00000, 0x10000, 0x85cccaa2 )

	ROM_REGION( 0x180000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b70-01.bin", 0x00000, 0x80000, 0xfcdf67e2 )
	ROM_LOAD( "b70-02.bin", 0x80000, 0x80000, 0x35f7172e )
ROM_END

ROM_START( raimais )
	ROM_REGION( 0xb0000, REGION_CPU1 )
	ROM_LOAD( "b36-08-1.bin", 0x00000, 0x20000, 0x6cc8f79f )
	ROM_RELOAD(               0x10000, 0x20000 )
	ROM_LOAD( "b36-03.bin",   0x30000, 0x80000, 0x96166516 )

	ROM_REGION( 0x10000, REGION_CPU2 )
	ROM_LOAD( "b36-07.bin",   0x00000, 0x10000, 0x4f3737e6 )

	ROM_REGION( 0x10000, REGION_CPU3 )
	ROM_LOAD( "b36-06.bin",   0x00000, 0x10000, 0x29bbc4f8 )

	ROM_REGION( 0x180000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "b36-01.bin",   0x00000, 0x80000, 0x89355cb2 )
	ROM_LOAD( "b36-02.bin",   0x80000, 0x80000, 0xe71da5db )
ROM_END

ROM_START( champwr )
	ROM_REGION( 0xf0000, REGION_CPU1 )
	ROM_LOAD( "c01-13.rom", 0x00000, 0x20000, 0x7ef47525 )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom", 0x30000, 0x20000, 0x358bd076 )
	ROM_LOAD( "c01-05.rom", 0x50000, 0x20000, 0x22efad4a )

	ROM_REGION( 0x30000, REGION_CPU2 )
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, 0x5117c98f )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU3 )
	ROM_LOAD( "c01-08.rom", 0x00000, 0x10000, 0x810efff8 )

	ROM_REGION( 0x180000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, 0xf302e6e9 )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, 0x1e0476c4 )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, 0x2a142dbc )
ROM_END

ROM_START( champwru )
	ROM_REGION( 0xf0000, REGION_CPU1 )
	ROM_LOAD( "c01-12.rom", 0x00000, 0x20000, 0x09f345b3 )
	ROM_RELOAD(              0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom",  0x30000, 0x20000, 0x358bd076 )
	ROM_LOAD( "c01-05.rom",  0x50000, 0x20000, 0x22efad4a )

	ROM_REGION( 0x30000, REGION_CPU2 )
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, 0x5117c98f )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU3 )
	ROM_LOAD( "c01-08.rom", 0x00000, 0x10000, 0x810efff8 )

	ROM_REGION( 0x180000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, 0xf302e6e9 )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, 0x1e0476c4 )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, 0x2a142dbc )
ROM_END

ROM_START( champwrj )
	ROM_REGION( 0xf0000, REGION_CPU1 )
	ROM_LOAD( "c01-06.bin", 0x00000, 0x20000, 0x90fa1409 )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom", 0x30000, 0x20000, 0x358bd076 )
	ROM_LOAD( "c01-05.rom", 0x50000, 0x20000, 0x22efad4a )

	ROM_REGION( 0x30000, REGION_CPU2 )
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, 0x5117c98f )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x10000, REGION_CPU3 )
	ROM_LOAD( "c01-08.rom", 0x00000, 0x10000, 0x810efff8 )

	ROM_REGION( 0x180000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, 0xf302e6e9 )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, 0x1e0476c4 )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, 0x2a142dbc )
ROM_END


ROM_START( puzznic )
	ROM_REGION( 0x30000, REGION_CPU1 )
	ROM_LOAD( "u11.rom",  0x00000, 0x20000, 0xa4150b6c )
	ROM_RELOAD(           0x10000, 0x20000 )

	ROM_REGION( 0x0800, REGION_CPU2 )	/* 2k for the microcontroller */
	ROM_LOAD( "mc68705p", 0x0000, 0x0800, 0x00000000 )

	ROM_REGION( 0x40000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "u10.rom",  0x00000, 0x20000, 0x4264056c )
	ROM_LOAD( "u09.rom",  0x20000, 0x20000, 0x3c115f8b )
ROM_END

ROM_START( plotting )
	ROM_REGION( 0x20000, REGION_CPU1 )
	ROM_LOAD( "plot01.bin", 0x00000, 0x10000, 0x5b30bc25 )
	ROM_RELOAD(             0x10000, 0x10000 )

	ROM_REGION( 0x20000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "plot07.bin", 0x00000, 0x10000, 0x6e0bad2a )
	ROM_LOAD( "plot08.bin", 0x10000, 0x10000, 0xfb5f3ca4 )
ROM_END

ROM_START( palamed )
	ROM_REGION( 0x30000, REGION_CPU1 )
	ROM_LOAD( "c63.02", 0x00000, 0x20000, 0x55a82bb2 )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION( 0x40000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "c64.04", 0x00000, 0x20000, 0xc7bbe460 )
	ROM_LOAD( "c63.03", 0x20000, 0x20000, 0xfcd86e44 )
ROM_END

ROM_START( horshoes )
	ROM_REGION( 0x30000, REGION_CPU1 )
	ROM_LOAD( "c47.03", 0x00000, 0x20000, 0x37e15b20 )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "c47.02", 0x00000, 0x10000, 0x35f96526 )
	ROM_CONTINUE (      0x20000, 0x10000 )
	ROM_LOAD( "c47.04", 0x40000, 0x10000, 0xaeac7121 )
	ROM_CONTINUE (      0x60000, 0x10000 )
	ROM_LOAD( "c47.01", 0x10000, 0x10000, 0x031c73d8 )
	ROM_CONTINUE (      0x30000, 0x10000 )
	ROM_LOAD( "c47.05", 0x50000, 0x10000, 0xb2a3dafe )
	ROM_CONTINUE (      0x70000, 0x10000 )
ROM_END

ROM_START( cachat )
	ROM_REGION( 0x30000, REGION_CPU1 )
	ROM_LOAD( "cac6",  0x00000, 0x20000, 0x8105cf5f )
	ROM_RELOAD(        0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cac9",  0x00000, 0x20000, 0xbc462914 )
	ROM_LOAD( "cac10", 0x20000, 0x20000, 0xecc64b31 )
	ROM_LOAD( "cac7",  0x40000, 0x20000, 0x7fb71578 )
	ROM_LOAD( "cac8",  0x60000, 0x20000, 0xd2a63799 )
ROM_END

ROM_START( plgirls )
	ROM_REGION( 0x50000, REGION_CPU1 )
	ROM_LOAD( "pg03.ic6",  0x00000, 0x40000, 0x6ca73092 )
	ROM_RELOAD(            0x10000, 0x40000 )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "pg02.ic9",  0x00000, 0x40000, 0x3cf05ca9 )
	ROM_LOAD( "pg01.ic7",  0x40000, 0x40000, 0x79e41e74 )
ROM_END

ROM_START( plgirls2 )
	ROM_REGION( 0x50000, REGION_CPU1 )
	ROM_LOAD( "pg2_1j.ic6",  0x00000, 0x40000, 0xf924197a )
	ROM_RELOAD(              0x10000, 0x40000 )

	ROM_REGION( 0x100000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "cho-l.ic9",  0x00000, 0x80000, 0x956384ec )
	ROM_LOAD( "cho-h.ic7",  0x80000, 0x80000, 0x992f99b1 )
ROM_END



// bits 7..0 => bits 0..7
static void init_plotting(void)
{
	unsigned char tab[256];
	unsigned char *p;
	int i;

	for(i=0;i<256;i++) {
		int j, v=0;
		for(j=0;j<8;j++)
			if(i & (1<<j))
				v |= 1<<(7-j);
		tab[i] = v;
	}
	p = memory_region(REGION_CPU1);
	for(i=0;i<0x20000;i++) {
		*p = tab[*p];
		p++;
	}
}



GAMEX( 1988, fhawk,    0,       fhawk,    fhawk,    0,        ROT270, "Taito Corporation", "Fighting Hawk (Japan)", GAME_NO_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1988, raimais,  0,       raimais,  raimais,  0,        ROT0,   "Taito Corporation", "Raimais (Japan)", GAME_NO_SOUND | GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX( 1989, champwr,  0,       champwr,  champwr,  0,        ROT0,   "Taito Corporation Japan", "Champion Wrestler (World)", GAME_NO_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1989, champwru, champwr, champwr,  champwr,  0,        ROT0,   "Taito America Corporation", "Champion Wrestler (US)", GAME_NO_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1989, champwrj, champwr, champwr,  champwr,  0,        ROT0,   "Taito Corporation", "Champion Wrestler (Japan)", GAME_NO_SOUND | GAME_NO_COCKTAIL )
GAMEX( 1989, puzznic,  0,       puzznic,  puzznic,  0,        ROT0,   "Taito Corporation", "Puzznic (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1989, plotting, 0,       plotting, plotting, plotting, ROT0,   "Taito Corporation Japan", "Plotting (World)", GAME_NO_COCKTAIL )
GAMEX( 1990, palamed,  0,       palamed,  palamed,  0,        ROT0,   "Taito Corporation", "Palamedes (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, horshoes, 0,       horshoes, horshoes, 0,        ROT270, "Taito America Corporation", "American Horseshoes (US)", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX( 1993, cachat,   0,       cachat,   cachat,   0,        ROT0,   "Taito Corporation", "Cachat (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1992, plgirls,  0,       cachat,   palamed,  0,        ROT270, "Hot-B.", "Play Girls", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
GAMEX( 1993, plgirls2, 0,       cachat,   palamed,  0,        ROT270, "Hot-B.", "Play Girls 2", GAME_NOT_WORKING | GAME_NO_COCKTAIL )
