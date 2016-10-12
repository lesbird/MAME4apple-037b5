/****************************************************************************/
/*            real mode i286 emulator by Fabrice Frances                    */
/*           (initial work based on David Hedley's pcemu)                   */
/*                                                                          */
/****************************************************************************/
/* OB[19.07.99] added Mode Flag V30 */

typedef enum { ES, CS, SS, DS } SREGS;
typedef enum { AW, CW, DW, BW, SP, BP, IX, IY } WREGS;

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifdef LSB_FIRST
typedef enum { AL,AH,CL,CH,DL,DH,BL,BH,SPL,SPH,BPL,BPH,IXL,IXH,IYL,IYH } BREGS;
#else
typedef enum { AH,AL,CH,CL,DH,DL,BH,BL,SPH,SPL,BPH,BPL,IXH,IXL,IYH,IYL } BREGS;
#endif

/* parameter x = result, y = source 1, z = source 2 */

#define SetTF(x)		(I.TF = (x))
#define SetIF(x)		(I.IF = (x))
#define SetDF(x)		(I.DF = (x))
#define SetMD(x)		(I.MF = (x))	/* OB [19.07.99] Mode Flag V30 */

#define SetOFW_Add(x,y,z)	(I.OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x8000)
#define SetOFB_Add(x,y,z)	(I.OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x80)
#define SetOFW_Sub(x,y,z)	(I.OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x8000)
#define SetOFB_Sub(x,y,z)	(I.OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x80)

#define SetCFB(x)		(I.CarryVal = (x) & 0x100)
#define SetCFW(x)		(I.CarryVal = (x) & 0x10000)
#define SetAF(x,y,z)	(I.AuxVal = ((x) ^ ((y) ^ (z))) & 0x10)
#define SetSF(x)		(I.SignVal = (x))
#define SetZF(x)		(I.ZeroVal = (x))
#define SetPF(x)		(I.ParityVal = (x))

#define SetSZPF_Byte(x) (I.SignVal=I.ZeroVal=I.ParityVal=(INT8)(x))
#define SetSZPF_Word(x) (I.SignVal=I.ZeroVal=I.ParityVal=(INT16)(x))

#define ADDB(dst,src) { unsigned res=dst+src; SetCFB(res); SetOFB_Add(res,src,dst); SetAF(res,src,dst); SetSZPF_Byte(res); dst=(BYTE)res; }
#define ADDW(dst,src) { unsigned res=dst+src; SetCFW(res); SetOFW_Add(res,src,dst); SetAF(res,src,dst); SetSZPF_Word(res); dst=(WORD)res; }

#define SUBB(dst,src) { unsigned res=dst-src; SetCFB(res); SetOFB_Sub(res,src,dst); SetAF(res,src,dst); SetSZPF_Byte(res); dst=(BYTE)res; }
#define SUBW(dst,src) { unsigned res=dst-src; SetCFW(res); SetOFW_Sub(res,src,dst); SetAF(res,src,dst); SetSZPF_Word(res); dst=(WORD)res; }

#define ORB(dst,src) dst|=src; I.CarryVal=I.OverVal=I.AuxVal=0; SetSZPF_Byte(dst)
#define ORW(dst,src) dst|=src; I.CarryVal=I.OverVal=I.AuxVal=0; SetSZPF_Word(dst)

#define ANDB(dst,src) dst&=src; I.CarryVal=I.OverVal=I.AuxVal=0; SetSZPF_Byte(dst)
#define ANDW(dst,src) dst&=src; I.CarryVal=I.OverVal=I.AuxVal=0; SetSZPF_Word(dst)

#define XORB(dst,src) dst^=src; I.CarryVal=I.OverVal=I.AuxVal=0; SetSZPF_Byte(dst)
#define XORW(dst,src) dst^=src; I.CarryVal=I.OverVal=I.AuxVal=0; SetSZPF_Word(dst)

#define CF		(I.CarryVal!=0)
#define SF		(I.SignVal<0)
#define ZF		(I.ZeroVal==0)
#define PF		parity_table[(BYTE)I.ParityVal]
#define AF		(I.AuxVal!=0)
#define OF		(I.OverVal!=0)
#define MD		(I.MF!=0)
/************************************************************************/

/* drop lines A16-A19 for a 64KB memory (yes, I know this should be done after adding the offset 8-) */
/* HJB 12/13/98 instead mask address lines with a driver supplied value */
#define SegBase(Seg) (I.sregs[Seg] << 4)

#define DefaultBase(Seg) ((I.seg_prefix && (Seg==DS || Seg==SS)) ? I.prefix_base : I.base[Seg])

/* ASG 971005 -- changed to cpu_readmem20/cpu_writemem20 */
#define GetMemB(Seg,Off) (nec_ICount-=6,(BYTE)cpu_readmem20((DefaultBase(Seg)+(Off))))
#define GetMemW(Seg,Off) (nec_ICount-=10,(WORD)GetMemB(Seg,Off)+(WORD)(GetMemB(Seg,(Off)+1)<<8))
#define PutMemB(Seg,Off,x) { nec_ICount-=7; cpu_writemem20((DefaultBase(Seg)+(Off)),(x)); }
#define PutMemW(Seg,Off,x) { nec_ICount-=11; PutMemB(Seg,Off,(BYTE)(x)); PutMemB(Seg,(Off)+1,(BYTE)((x)>>8)); }

#define ReadByte(ea) (nec_ICount-=6,(BYTE)cpu_readmem20((ea)))
#define ReadWord(ea) (nec_ICount-=10,cpu_readmem20((ea))+(cpu_readmem20(((ea)+1))<<8))
#define WriteByte(ea,val) { nec_ICount-=7; cpu_writemem20((ea),val); }
#define WriteWord(ea,val) { nec_ICount-=11; cpu_writemem20((ea),(BYTE)(val)); cpu_writemem20(((ea)+1),(val)>>8); }

#define read_port(port) cpu_readport(port)
#define write_port(port,val) cpu_writeport(port,val)

/* no need to go through cpu_readmem for these ones... */
/* ASG 971222 -- PUSH/POP now use the standard mechanisms; opcode reading is the same */
#define FETCH ((BYTE)cpu_readop_arg((I.base[CS]+I.ip++)))
#define FETCHOP ((BYTE)cpu_readop((I.base[CS]+I.ip++)))
#define FETCHWORD(var) { var=cpu_readop_arg(((I.base[CS]+I.ip)))+(cpu_readop_arg(((I.base[CS]+I.ip+1)))<<8); I.ip+=2; }
#define PUSH(val) { I.regs.w[SP]-=2; WriteWord(((I.base[SS]+I.regs.w[SP])),val); }
#define POP(var) { var = ReadWord(((I.base[SS]+I.regs.w[SP]))); I.regs.w[SP]+=2; }
/************************************************************************/
#define CompressFlags() (WORD)(CF | (PF << 2) | (AF << 4) | (ZF << 6) \
				| (SF << 7) | (I.TF << 8) | (I.IF << 9) \
				| (I.DF << 10) | (OF << 11)| (MD << 15))

#define ExpandFlags(f) \
{ \
	  I.CarryVal = (f) & 1; \
	  I.ParityVal = !((f) & 4); \
	  I.AuxVal = (f) & 16; \
	  I.ZeroVal = !((f) & 64); \
	  I.SignVal = (f) & 128 ? -1 : 0; \
	  I.TF = ((f) & 256) == 256; \
	  I.IF = ((f) & 512) == 512; \
	  I.DF = ((f) & 1024) == 1024; \
	  I.OverVal = (f) & 2048; \
	  I.MF = ((f) & 0x8000) == 0x8000; \
}
