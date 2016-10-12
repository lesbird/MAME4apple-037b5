/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"

unsigned char *ddrible_sharedram;
unsigned char *ddrible_snd_sharedram;
int ddrible_int_enable_0, ddrible_int_enable_1;

void ddrible_init_machine( void )
{
	ddrible_int_enable_0 = ddrible_int_enable_1 = 0;
}

WRITE_HANDLER( ddrible_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x0f) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}

int ddrible_interrupt_0( void )
{
	if (ddrible_int_enable_0)
		return M6809_INT_FIRQ;
	return ignore_interrupt();
}

int ddrible_interrupt_1( void )
{
	if (ddrible_int_enable_1)
		return M6809_INT_FIRQ;
	return ignore_interrupt();
}

WRITE_HANDLER( int_0_w )
{
	if (data & 0x02)
		ddrible_int_enable_0 = 1;
	else
		ddrible_int_enable_0 = 0;
}

WRITE_HANDLER( int_1_w )
{
	if (data & 0x02)
		ddrible_int_enable_1 = 1;
	else
		ddrible_int_enable_1 = 0;
}

READ_HANDLER( ddrible_sharedram_r )
{
	return ddrible_sharedram[offset];
}

WRITE_HANDLER( ddrible_sharedram_w )
{
	ddrible_sharedram[offset] = data;
}

READ_HANDLER( ddrible_snd_sharedram_r )
{
	return ddrible_snd_sharedram[offset];
}

WRITE_HANDLER( ddrible_snd_sharedram_w )
{
	ddrible_snd_sharedram[offset] = data;
}

WRITE_HANDLER( ddrible_coin_counter_w )
{
	/* b4-b7: unused */
	/* b2-b3: unknown */
	/* b1: coin counter 2 */
	/* b0: coin counter 1 */

	coin_counter_w(0,(data) & 0x01);
	coin_counter_w(1,(data >> 1) & 0x01);
}

