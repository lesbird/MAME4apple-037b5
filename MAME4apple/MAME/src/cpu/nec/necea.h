/* 2do: Check the bus wait cycles given here.....*/

static unsigned EA;
static unsigned EO; /* HJB 12/13/98 effective offset of the address (before segment is added) */

static unsigned EA_000(void) { nec_ICount-=7; EO=(WORD)(I.regs.w[BW]+I.regs.w[IX]); EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_001(void) { nec_ICount-=8; EO=(WORD)(I.regs.w[BW]+I.regs.w[IY]); EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_002(void) { nec_ICount-=8; EO=(WORD)(I.regs.w[BP]+I.regs.w[IX]); EA=DefaultBase(SS)+EO; return EA; }
static unsigned EA_003(void) { nec_ICount-=7; EO=(WORD)(I.regs.w[BP]+I.regs.w[IY]); EA=DefaultBase(SS)+EO; return EA; }
static unsigned EA_004(void) { nec_ICount-=5; EO=I.regs.w[IX]; EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_005(void) { nec_ICount-=5; EO=I.regs.w[IY]; EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_006(void) { nec_ICount-=6; EO=FETCHOP; EO+=FETCHOP<<8; EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_007(void) { nec_ICount-=5; EO=I.regs.w[BW]; EA=DefaultBase(DS)+EO; return EA; }

static unsigned EA_100(void) { nec_ICount-=11; EO=(WORD)(I.regs.w[BW]+I.regs.w[IX]+(INT8)FETCHOP); EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_101(void) { nec_ICount-=12; EO=(WORD)(I.regs.w[BW]+I.regs.w[IY]+(INT8)FETCHOP); EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_102(void) { nec_ICount-=12; EO=(WORD)(I.regs.w[BP]+I.regs.w[IX]+(INT8)FETCHOP); EA=DefaultBase(SS)+EO; return EA; }
static unsigned EA_103(void) { nec_ICount-=11; EO=(WORD)(I.regs.w[BP]+I.regs.w[IY]+(INT8)FETCHOP); EA=DefaultBase(SS)+EO; return EA; }
static unsigned EA_104(void) { nec_ICount-=9; EO=(WORD)(I.regs.w[IX]+(INT8)FETCHOP); EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_105(void) { nec_ICount-=9; EO=(WORD)(I.regs.w[IY]+(INT8)FETCHOP); EA=DefaultBase(DS)+EO; return EA; }
static unsigned EA_106(void) { nec_ICount-=9; EO=(WORD)(I.regs.w[BP]+(INT8)FETCHOP); EA=DefaultBase(SS)+EO; return EA; }
static unsigned EA_107(void) { nec_ICount-=9; EO=(WORD)(I.regs.w[BW]+(INT8)FETCHOP); EA=DefaultBase(DS)+EO; return EA; }

static unsigned EA_200(void) { nec_ICount-=11; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[BW]+I.regs.w[IX]; EA=DefaultBase(DS)+(WORD)EO; return EA; }
static unsigned EA_201(void) { nec_ICount-=12; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[BW]+I.regs.w[IY]; EA=DefaultBase(DS)+(WORD)EO; return EA; }
static unsigned EA_202(void) { nec_ICount-=12; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[BP]+I.regs.w[IX]; EA=DefaultBase(SS)+(WORD)EO; return EA; }
static unsigned EA_203(void) { nec_ICount-=11; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[BP]+I.regs.w[IY]; EA=DefaultBase(SS)+(WORD)EO; return EA; }
static unsigned EA_204(void) { nec_ICount-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[IX]; EA=DefaultBase(DS)+(WORD)EO; return EA; }
static unsigned EA_205(void) { nec_ICount-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[IY]; EA=DefaultBase(DS)+(WORD)EO; return EA; }
static unsigned EA_206(void) { nec_ICount-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[BP]; EA=DefaultBase(SS)+(WORD)EO; return EA; }
static unsigned EA_207(void) { nec_ICount-=9; EO=FETCHOP; EO+=FETCHOP<<8; EO+=I.regs.w[BW]; EA=DefaultBase(DS)+(WORD)EO; return EA; }

static unsigned (*GetEA[192])(void)={
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,
	EA_000, EA_001, EA_002, EA_003, EA_004, EA_005, EA_006, EA_007,

	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,
	EA_100, EA_101, EA_102, EA_103, EA_104, EA_105, EA_106, EA_107,

	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207,
	EA_200, EA_201, EA_202, EA_203, EA_204, EA_205, EA_206, EA_207
};
