#include "driver.h"
#include "mamedbg.h"

const char *cc[] = {
	"EQ", "NE",
	"CS", "CC",
	"MI", "PL",
	"VS", "VC",
	"HI", "LS",
	"GE", "LT",
	"GT", "LE",
	"  ", "NV"  /* AL for always isn't printed */
};

/* destination register */
static char *RD(UINT32 op)
{
	static char buff[8];
	sprintf(buff, "R%d", (op >> 12) & 15);
	return buff;
}

/* first operand register */
static char *RN(UINT32 op)
{
	static char buff[8];
	sprintf(buff, "R%d", (op >> 16) & 15);
	return buff;
}

/* second operand is a (shifted) register */
static char *RS(UINT32 op)
{
	static char buff[32];
	UINT32 m = op & 0x70;
    UINT32 s = (op >> 7) & 31;
	UINT32 r = s >> 1;
	UINT32 rs = op & 15;

	buff[0] = '\0';

    switch( m )
	{
	case 0x00:
		/* LSL (aka ASL) #0 .. 31 */
		if (s > 0)
			sprintf(buff, "R%d LSL #%d", rs, s);
		else
			sprintf(buff, "R%d", rs);
		break;
	case 0x10:
		/* LSL (aka ASL) R0 .. R15 */
		sprintf(buff, "R%d LSL R%d", rs, r);
		break;
	case 0x20:
        /* LSR #1 .. 32 */
		if (s == 0) s = 32;
		sprintf(buff, "R%d LSR #%d", rs, s);
        break;
	case 0x30:
		/* LSR R0 .. R15 */
		sprintf(buff, "R%d LSR R%d", rs, r);
		break;
	case 0x40:
        /* ASR #1 .. 32 */
		if (s == 0) s = 32;
		sprintf(buff, "R%d ASR #%d", rs, s);
        break;
	case 0x50:
		/* ASR R0 .. R15 */
		sprintf(buff, "R%d ASR R%d", rs, r);
		break;
	case 0x60:
		/* ASR #1 .. 32 */
        if (s == 0)
			sprintf(buff, "R%d RRX", rs);
        else
			sprintf(buff, "R%d ROR #%d", rs, s);
        break;
	case 0x70:
	default:
		/* ROR R0 .. R15  */
		sprintf(buff, "R%d ROR R%d", rs, r);
        break;
    }
	return buff;
}

static char *IM(UINT32 op)
{
    static char buff[32];
	UINT32 val = op & 0xff, shift = (op >> 8) & 31;
	sprintf(buff, "$%x", (val << (31-shift*2)) | (val >> (shift*2)));
	return buff;
}

static char *RL(UINT32 op)
{
	static char buff[64];
	char *dst;
	int f, t, n;

	dst = buff;
	for(f = 0, n = 0; f < 16; f = t)
	{
		for (/* */; f < 16; f++)
			if( (op & (1L << f)) != 0 )
				break;
		for (t = f; t < 16; t++)
			if( (op & (1L << t)) == 0 )
				break;
		t--;

        if (f == 16 && n == 0)
			return "---";

		if (n)
			dst += sprintf(dst, ",");
		if (f == t)
			dst += sprintf(dst, "R%d", f);
		else
			dst += sprintf(dst, "R%d-R%d", f, t);
		n++;
	}
	return buff;
}

/*
 * The ARM SWI numbers
 * for taken from the ARM2 emulator (C) Edwin Dorre 1990
 */
#define XOS_Bit                        0x020000

#define OS_WriteI                      0x000100

#define OS_WriteC                      0x000000
#define OS_WriteS                      0x000001
#define OS_Write0                      0x000002
#define OS_NewLine                     0x000003
#define OS_ReadC                       0x000004
#define OS_CLI                         0x000005
#define OS_Byte                        0x000006
#define OS_Word                        0x000007
#define OS_File                        0x000008
#define OS_Args                        0x000009
#define OS_BGet                        0x00000a
#define OS_BPut                        0x00000b
#define OS_GBPB                        0x00000c
#define OS_Find                        0x00000d
#define OS_ReadLine                    0x00000e
#define OS_Control                     0x00000f
#define OS_GetEnv                      0x000010
#define OS_Exit                        0x000011
#define OS_SetEnv                      0x000012
#define OS_IntOn                       0x000013
#define OS_IntOff                      0x000014
#define OS_CallBack                    0x000015
#define OS_EnterOS                     0x000016
#define OS_BreakPt                     0x000017
#define OS_BreakCtrl                   0x000018
#define OS_UnusedSWI                   0x000019
#define OS_UpdateMEMC                  0x00001a
#define OS_SetCallBack                 0x00001b
#define OS_Mouse                       0x00001c
#define OS_Heap                        0x00001d
#define OS_Module                      0x00001e
#define OS_Claim                       0x00001f
#define OS_Release                     0x000020
#define OS_ReadUnsigned                0x000021
#define OS_GenerateEvent               0x000022
#define OS_ReadVarVal                  0x000023
#define OS_SetVarVal                   0x000024
#define OS_GSInit                      0x000025
#define OS_GSRead                      0x000026
#define OS_GSTrans                     0x000027
#define OS_BinaryToDecimal             0x000028
#define OS_FSControl                   0x000029
#define OS_ChangeDynamicArea           0x00002a
#define OS_GenerateError               0x00002b
#define OS_ReadEscapeState             0x00002c
#define OS_EvaluateExpression          0x00002d
#define OS_SpriteOp                    0x00002e
#define OS_ReadPalette                 0x00002f
#define OS_ServiceCall                 0x000030
#define OS_ReadVduVariables            0x000031
#define OS_ReadPoint                   0x000032
#define OS_UpCall                      0x000033
#define OS_CallAVector                 0x000034
#define OS_ReadModeVariable            0x000035
#define OS_RemoveCursors               0x000036
#define OS_RestoreCursors              0x000037
#define OS_SWINumberToString           0x000038
#define OS_SWINumberFromString         0x000039
#define OS_ValidateAddress             0x00003a
#define OS_CallAfter                   0x00003b
#define OS_CallEvery                   0x00003c
#define OS_RemoveTickerEvent           0x00003d
#define OS_InstallKeyHandler           0x00003e
#define OS_CheckModeValid              0x00003f
#define OS_ChangeEnvironment           0x000040
#define OS_ClaimScreenMemory           0x000041
#define OS_ReadMonotonicTime           0x000042
#define OS_SubstituteArgs              0x000043
#define OS_PrettyPrint                 0x000044
#define OS_Plot                        0x000045
#define OS_WriteN                      0x000046
#define OS_AddToVector                 0x000047
#define OS_WriteEnv                    0x000048
#define OS_ReadArgs                    0x000049
#define OS_ReadRAMFsLimits             0x00004a
#define OS_ClaimDeviceVector           0x00004b
#define OS_ReleaseDeviceVector         0x00004c
#define OS_DelinkApplication           0x00004d
#define OS_RelinkApplication           0x00004e
#define OS_HeapSort                    0x00004f
#define OS_ExitAndDie                  0x000050
#define OS_ReadMemMapInfo              0x000051
#define OS_ReadMemMapEntries           0x000052
#define OS_SetMemMapEntries            0x000053
#define OS_AddCallBack                 0x000054
#define OS_ReadDefaultHandler          0x000055
#define OS_SetECFOrigin                0x000056
#define OS_SerialOp                    0x000057
#define OS_ReadSysInfo                 0x000058
#define OS_Confirm                     0x000059
#define OS_ChangedBox                  0x00005a
#define OS_CRC                         0x00005b
#define OS_ReadDynamicArea             0x00005c
#define OS_PrintChar                   0x00005d
#define OS_ConvertStandardDateAndTime  0x0000c0
#define OS_ConvertDateAndTime          0x0000c1
#define OS_ConvertHex1                 0x0000d0
#define OS_ConvertHex2                 0x0000d1
#define OS_ConvertHex4                 0x0000d2
#define OS_ConvertHex6                 0x0000d3
#define OS_ConvertHex8                 0x0000d4
#define OS_ConvertCardinal1            0x0000d5
#define OS_ConvertCardinal2            0x0000d6
#define OS_ConvertCardinal3            0x0000d7
#define OS_ConvertCardinal4            0x0000d8
#define OS_ConvertInteger1             0x0000d9
#define OS_ConvertInteger2             0x0000da
#define OS_ConvertInteger3             0x0000db
#define OS_ConvertInteger4             0x0000dc
#define OS_ConvertBinary1              0x0000dd
#define OS_ConvertBinary2              0x0000de
#define OS_ConvertBinary3              0x0000df
#define OS_ConvertBinary4              0x0000e0
#define OS_ConvertSpacedCardinal1      0x0000e1
#define OS_ConvertSpacedCardinal2      0x0000e2
#define OS_ConvertSpacedCardinal3      0x0000e3
#define OS_ConvertSpacedCardinal4      0x0000e4
#define OS_ConvertSpacedInteger1       0x0000e5
#define OS_ConvertSpacedInteger2       0x0000e6
#define OS_ConvertSpacedInteger3       0x0000e7
#define OS_ConvertSpacedInteger4       0x0000e8
#define OS_ConvertFixedNetStation      0x0000e9
#define OS_ConvertNetStation           0x0000ea
#define OS_ConvertFixedFileSize        0x0000eb
#define OS_ConvertFileSize             0x0000ec
#define IIC_Control                    0x000240
#define Econet_CreateReceive           0x040000
#define Econet_ExamineReceive          0x040001
#define Econet_ReadReceive             0x040002
#define Econet_AbandonReceive          0x040003
#define Econet_WaitForReception        0x040004
#define Econet_EnumerateReceive        0x040005
#define Econet_StartTransmit           0x040006
#define Econet_PollTransmit            0x040007
#define Econet_AbandonTransmit         0x040008
#define Econet_DoTransmit              0x040009
#define Econet_ReadLocalStationAndNet  0x04000a
#define Econet_ConvertStatusToString   0x04000b
#define Econet_ConvertStatusToError    0x04000c
#define Econet_ReadProtection          0x04000d
#define Econet_SetProtection           0x04000e
#define Econet_ReadStationNumber       0x04000f
#define Econet_PrintBanner             0x040010
#define Econet_ReleasePort             0x040012
#define Econet_AllocatePort            0x040013
#define Econet_DeAllocatePort          0x040014
#define Econet_ClaimPort               0x040015
#define Econet_StartImmediate          0x040016
#define Econet_DoImmediate             0x040017
#define NetFS_ReadFSNumber             0x040040
#define NetFS_SetFSNumber              0x040041
#define NetFS_ReadFSName               0x040042
#define NetFS_SetFSName                0x040043
#define NetFS_ReadCurrentContext       0x040044
#define NetFS_SetCurrentContext        0x040045
#define NetFS_ReadFSTimeouts           0x040046
#define NetFS_SetFSTimeouts            0x040047
#define NetFS_DoFSOp                   0x040048
#define NetFS_EnumerateFSList          0x040049
#define NetFS_EnumerateFS              0x04004a
#define NetFS_ConvertDate              0x04004b
#define NetFS_DoFSOpToGivenFS          0x04004c
#define Font_CacheAddr                 0x040080
#define Font_FindFont                  0x040081
#define Font_LoseFont                  0x040082
#define Font_ReadDefn                  0x040083
#define Font_ReadInfo                  0x040084
#define Font_StringWidth               0x040085
#define Font_Paint                     0x040086
#define Font_Caret                     0x040087
#define Font_ConverttoOS               0x040088
#define Font_Converttopoints           0x040089
#define Font_SetFont                   0x04008a
#define Font_CurrentFont               0x04008b
#define Font_FutureFont                0x04008c
#define Font_FindCaret                 0x04008d
#define Font_CharBBox                  0x04008e
#define Font_ReadScaleFactor           0x04008f
#define Font_SetScaleFactor            0x040090
#define Font_ListFonts                 0x040091
#define Font_SetFontColours            0x040092
#define Font_SetPalette                0x040093
#define Font_ReadThresholds            0x040094
#define Font_SetThresholds             0x040095
#define Font_FindCaretJ                0x040096
#define Font_StringBBox                0x040097
#define Font_ReadColourTable           0x040098
#define Wimp_Initialise                0x0400c0
#define Wimp_CreateWindow              0x0400c1
#define Wimp_CreateIcon                0x0400c2
#define Wimp_DeleteWindow              0x0400c3
#define Wimp_DeleteIcon                0x0400c4
#define Wimp_OpenWindow                0x0400c5
#define Wimp_CloseWindow               0x0400c6
#define Wimp_Poll                      0x0400c7
#define Wimp_RedrawWindow              0x0400c8
#define Wimp_UpdateWindow              0x0400c9
#define Wimp_GetRectangle              0x0400ca
#define Wimp_GetWindowState            0x0400cb
#define Wimp_GetWindowInfo             0x0400cc
#define Wimp_SetIconState              0x0400cd
#define Wimp_GetIconState              0x0400ce
#define Wimp_GetPointerInfo            0x0400cf
#define Wimp_DragBox                   0x0400d0
#define Wimp_ForceRedraw               0x0400d1
#define Wimp_SetCaretPosition          0x0400d2
#define Wimp_GetCaretPosition          0x0400d3
#define Wimp_CreateMenu                0x0400d4
#define Wimp_DecodeMenu                0x0400d5
#define Wimp_WhichIcon                 0x0400d6
#define Wimp_SetExtent                 0x0400d7
#define Wimp_SetPointerShape           0x0400d8
#define Wimp_OpenTemplate              0x0400d9
#define Wimp_CloseTemplate             0x0400da
#define Wimp_LoadTemplate              0x0400db
#define Wimp_ProcessKey                0x0400dc
#define Wimp_CloseDown                 0x0400dd
#define Wimp_StartTask                 0x0400de
#define Wimp_ReportError               0x0400df
#define Wimp_GetWindowOutline          0x0400e0
#define Wimp_PollIdle                  0x0400e1
#define Wimp_PlotIcon                  0x0400e2
#define Wimp_SetMode                   0x0400e3
#define Wimp_SetPalette                0x0400e4
#define Wimp_ReadPalette               0x0400e5
#define Wimp_SetColour                 0x0400e6
#define Wimp_SendMessage               0x0400e7
#define Wimp_CreateSubMenu             0x0400e8
#define Wimp_SpriteOp                  0x0400e9
#define Wimp_BaseOfSprites             0x0400ea
#define Wimp_BlockCopy                 0x0400eb
#define Wimp_SlotSize                  0x0400ec
#define Wimp_ReadPixTrans              0x0400ed
#define Wimp_ClaimFreeMemory           0x0400ee
#define Wimp_CommandWindow             0x0400ef
#define Wimp_TextColour                0x0400f0
#define Wimp_TransferBlock             0x0400f1
#define Wimp_ReadSysInfo               0x0400f2
#define Wimp_SetFontColours            0x0400f3
#define Sound_Configure                0x040140
#define Sound_Enable                   0x040141
#define Sound_Stereo                   0x040142
#define Sound_Speaker                  0x040143
#define Sound_Volume                   0x040180
#define Sound_SoundLog                 0x040181
#define Sound_LogScale                 0x040182
#define Sound_InstallVoice             0x040183
#define Sound_RemoveVoice              0x040184
#define Sound_AttachVoice              0x040185
#define Sound_ControlPacked            0x040186
#define Sound_Tuning                   0x040187
#define Sound_Pitch                    0x040188
#define Sound_Control                  0x040189
#define Sound_AttachNamedVoice         0x04018a
#define Sound_ReadControlBlock         0x04018b
#define Sound_WriteControlBlock        0x04018c
#define Sound_QInit                    0x0401c0
#define Sound_QSchedule                0x0401c1
#define Sound_QRemove                  0x0401c2
#define Sound_QFree                    0x0401c3
#define Sound_QSDispatch               0x0401c4
#define Sound_QTempo                   0x0401c5
#define Sound_QBeat                    0x0401c6
#define Sound_QInterface               0x0401c7
#define NetPrint_ReadPSNumber          0x040200
#define NetPrint_SetPSNumber           0x040201
#define NetPrint_ReadPSName            0x040202
#define NetPrint_SetPSName             0x040203
#define NetPrint_ReadPSTimeouts        0x040204
#define NetPrint_SetPSTimeouts         0x040205
#define ADFS_DiscOp                    0x040240
#define ADFS_HDC                       0x040241
#define ADFS_Drives                    0x040242
#define ADFS_FreeSpace                 0x040243
#define ADFS_Retries                   0x040244
#define ADFS_DescribeDisc              0x040245
#define Podule_ReadID                  0x040280
#define Podule_ReadHeader              0x040281
#define Podule_EnumerateChunks         0x040282
#define Podule_ReadChunk               0x040283
#define Podule_ReadBytes               0x040284
#define Podule_WriteBytes              0x040285
#define Podule_CallLoader              0x040286
#define Podule_RawRead                 0x040287
#define Podule_RawWrite                0x040288
#define Podule_HardwareAddress         0x040289
#define WaveSynth_Load                 0x040300
#define Debugger_Disassemble           0x040380
#define FPEmulator_Version             0x040480
#define FileCore_DiscOp                0x040540
#define FileCore_Create                0x040541
#define FileCore_Drives                0x040542
#define FileCore_FreeSpace             0x040543
#define FileCore_FloppyStructure       0x040544
#define FileCore_DescribeDisc          0x040545
#define Shell_Create                   0x0405c0
#define Shell_Destroy                  0x0405c1
#define Hourglass_On                   0x0406c0
#define Hourglass_Off                  0x0406c1
#define Hourglass_Smash                0x0406c2
#define Hourglass_Start                0x0406c3
#define Hourglass_Percentage           0x0406c4
#define Hourglass_LEDs                 0x0406c5
#define Draw_ProcessPath               0x040700
#define Draw_ProcessPathFP             0x040701
#define Draw_Fill                      0x040702
#define Draw_FillFP                    0x040703
#define Draw_Stroke                    0x040704
#define Draw_StrokeFP                  0x040705
#define Draw_StrokePath                0x040706
#define Draw_StrokePathFP              0x040707
#define Draw_FlattenPath               0x040708
#define Draw_FlattenPathFP             0x040709
#define Draw_TransformPath             0x04070a
#define Draw_TransformPathFP           0x04070b
#define RamFS_DiscOp                   0x040780
#define RamFS_Drives                   0x040782
#define RamFS_FreeSpace                0x040783
#define RamFS_DescribeDisc             0x040785

static struct {
	UINT32 swi;
	char *name;
} SWI[] = {
	{XOS_Bit,						 "XOS_Bit"},
	{OS_WriteI, 					 "OS_WriteI"},
	{OS_WriteC, 					 "OS_WriteC"},
	{OS_WriteS, 					 "OS_WriteS"},
	{OS_Write0, 					 "OS_Write0"},
	{OS_NewLine,					 "OS_NewLine"},
	{OS_ReadC,						 "OS_ReadC"},
	{OS_CLI,						 "OS_CLI"},
	{OS_Byte,						 "OS_Byte"},
	{OS_Word,						 "OS_Word"},
	{OS_File,						 "OS_File"},
	{OS_Args,						 "OS_Args"},
	{OS_BGet,						 "OS_BGet"},
	{OS_BPut,						 "OS_BPut"},
	{OS_GBPB,						 "OS_GBPB"},
	{OS_Find,						 "OS_Find"},
	{OS_ReadLine,					 "OS_ReadLine"},
	{OS_Control,					 "OS_Control"},
	{OS_GetEnv, 					 "OS_GetEnv"},
	{OS_Exit,						 "OS_Exit"},
	{OS_SetEnv, 					 "OS_SetEnv"},
	{OS_IntOn,						 "OS_IntOn"},
	{OS_IntOff, 					 "OS_IntOff"},
	{OS_CallBack,					 "OS_CallBack"},
	{OS_EnterOS,					 "OS_EnterOS"},
	{OS_BreakPt,					 "OS_BreakPt"},
	{OS_BreakCtrl,					 "OS_BreakCtrl"},
	{OS_UnusedSWI,					 "OS_UnusedSWI"},
	{OS_UpdateMEMC, 				 "OS_UpdateMEMC"},
	{OS_SetCallBack,				 "OS_SetCallBack"},
	{OS_Mouse,						 "OS_Mouse"},
	{OS_Heap,						 "OS_Heap"},
	{OS_Module, 					 "OS_Module"},
	{OS_Claim,						 "OS_Claim"},
	{OS_Release,					 "OS_Release"},
	{OS_ReadUnsigned,				 "OS_ReadUnsigned"},
	{OS_GenerateEvent,				 "OS_GenerateEvent"},
	{OS_ReadVarVal, 				 "OS_ReadVarVal"},
	{OS_SetVarVal,					 "OS_SetVarVal"},
	{OS_GSInit, 					 "OS_GSInit"},
	{OS_GSRead, 					 "OS_GSRead"},
	{OS_GSTrans,					 "OS_GSTrans"},
	{OS_BinaryToDecimal,			 "OS_BinaryToDecimal"},
	{OS_FSControl,					 "OS_FSControl"},
	{OS_ChangeDynamicArea,			 "OS_ChangeDynamicArea"},
	{OS_GenerateError,				 "OS_GenerateError"},
	{OS_ReadEscapeState,			 "OS_ReadEscapeState"},
	{OS_EvaluateExpression, 		 "OS_EvaluateExpression"},
	{OS_SpriteOp,					 "OS_SpriteOp"},
	{OS_ReadPalette,				 "OS_ReadPalette"},
	{OS_ServiceCall,				 "OS_ServiceCall"},
	{OS_ReadVduVariables,			 "OS_ReadVduVariables"},
	{OS_ReadPoint,					 "OS_ReadPoint"},
	{OS_UpCall, 					 "OS_UpCall"},
	{OS_CallAVector,				 "OS_CallAVector"},
	{OS_ReadModeVariable,			 "OS_ReadModeVariable"},
	{OS_RemoveCursors,				 "OS_RemoveCursors"},
	{OS_RestoreCursors, 			 "OS_RestoreCursors"},
	{OS_SWINumberToString,			 "OS_SWINumberToString"},
	{OS_SWINumberFromString,		 "OS_SWINumberFromString"},
	{OS_ValidateAddress,			 "OS_ValidateAddress"},
	{OS_CallAfter,					 "OS_CallAfter"},
	{OS_CallEvery,					 "OS_CallEvery"},
	{OS_RemoveTickerEvent,			 "OS_RemoveTickerEvent"},
	{OS_InstallKeyHandler,			 "OS_InstallKeyHandler"},
	{OS_CheckModeValid, 			 "OS_CheckModeValid"},
	{OS_ChangeEnvironment,			 "OS_ChangeEnvironment"},
	{OS_ClaimScreenMemory,			 "OS_ClaimScreenMemory"},
	{OS_ReadMonotonicTime,			 "OS_ReadMonotonicTime"},
	{OS_SubstituteArgs, 			 "OS_SubstituteArgs"},
	{OS_PrettyPrint,				 "OS_PrettyPrint"},
	{OS_Plot,						 "OS_Plot"},
	{OS_WriteN, 					 "OS_WriteN"},
	{OS_AddToVector,				 "OS_AddToVector"},
	{OS_WriteEnv,					 "OS_WriteEnv"},
	{OS_ReadArgs,					 "OS_ReadArgs"},
	{OS_ReadRAMFsLimits,			 "OS_ReadRAMFsLimits"},
	{OS_ClaimDeviceVector,			 "OS_ClaimDeviceVector"},
	{OS_ReleaseDeviceVector,		 "OS_ReleaseDeviceVector"},
	{OS_DelinkApplication,			 "OS_DelinkApplication"},
	{OS_RelinkApplication,			 "OS_RelinkApplication"},
	{OS_HeapSort,					 "OS_HeapSort"},
	{OS_ExitAndDie, 				 "OS_ExitAndDie"},
	{OS_ReadMemMapInfo, 			 "OS_ReadMemMapInfo"},
	{OS_ReadMemMapEntries,			 "OS_ReadMemMapEntries"},
	{OS_SetMemMapEntries,			 "OS_SetMemMapEntries"},
	{OS_AddCallBack,				 "OS_AddCallBack"},
	{OS_ReadDefaultHandler, 		 "OS_ReadDefaultHandler"},
	{OS_SetECFOrigin,				 "OS_SetECFOrigin"},
	{OS_SerialOp,					 "OS_SerialOp"},
	{OS_ReadSysInfo,				 "OS_ReadSysInfo"},
	{OS_Confirm,					 "OS_Confirm"},
	{OS_ChangedBox, 				 "OS_ChangedBox"},
	{OS_CRC,						 "OS_CRC"},
	{OS_ReadDynamicArea,			 "OS_ReadDynamicArea"},
	{OS_PrintChar,					 "OS_PrintChar"},
	{OS_ConvertStandardDateAndTime,  "OS_ConvertStandardDateAndTime"},
	{OS_ConvertDateAndTime, 		 "OS_ConvertDateAndTime"},
	{OS_ConvertHex1,				 "OS_ConvertHex1"},
	{OS_ConvertHex2,				 "OS_ConvertHex2"},
	{OS_ConvertHex4,				 "OS_ConvertHex4"},
	{OS_ConvertHex6,				 "OS_ConvertHex6"},
	{OS_ConvertHex8,				 "OS_ConvertHex8"},
	{OS_ConvertCardinal1,			 "OS_ConvertCardinal1"},
	{OS_ConvertCardinal2,			 "OS_ConvertCardinal2"},
	{OS_ConvertCardinal3,			 "OS_ConvertCardinal3"},
	{OS_ConvertCardinal4,			 "OS_ConvertCardinal4"},
	{OS_ConvertInteger1,			 "OS_ConvertInteger1"},
	{OS_ConvertInteger2,			 "OS_ConvertInteger2"},
	{OS_ConvertInteger3,			 "OS_ConvertInteger3"},
	{OS_ConvertInteger4,			 "OS_ConvertInteger4"},
	{OS_ConvertBinary1, 			 "OS_ConvertBinary1"},
	{OS_ConvertBinary2, 			 "OS_ConvertBinary2"},
	{OS_ConvertBinary3, 			 "OS_ConvertBinary3"},
	{OS_ConvertBinary4, 			 "OS_ConvertBinary4"},
	{OS_ConvertSpacedCardinal1, 	 "OS_ConvertSpacedCardinal1"},
	{OS_ConvertSpacedCardinal2, 	 "OS_ConvertSpacedCardinal2"},
	{OS_ConvertSpacedCardinal3, 	 "OS_ConvertSpacedCardinal3"},
	{OS_ConvertSpacedCardinal4, 	 "OS_ConvertSpacedCardinal4"},
	{OS_ConvertSpacedInteger1,		 "OS_ConvertSpacedInteger1"},
	{OS_ConvertSpacedInteger2,		 "OS_ConvertSpacedInteger2"},
	{OS_ConvertSpacedInteger3,		 "OS_ConvertSpacedInteger3"},
	{OS_ConvertSpacedInteger4,		 "OS_ConvertSpacedInteger4"},
	{OS_ConvertFixedNetStation, 	 "OS_ConvertFixedNetStation"},
	{OS_ConvertNetStation,			 "OS_ConvertNetStation"},
	{OS_ConvertFixedFileSize,		 "OS_ConvertFixedFileSize"},
	{OS_ConvertFileSize,			 "OS_ConvertFileSize"},
	{IIC_Control,					 "IIC_Control"},
	{Econet_CreateReceive,			 "Econet_CreateReceive"},
	{Econet_ExamineReceive, 		 "Econet_ExamineReceive"},
	{Econet_ReadReceive,			 "Econet_ReadReceive"},
	{Econet_AbandonReceive, 		 "Econet_AbandonReceive"},
	{Econet_WaitForReception,		 "Econet_WaitForReception"},
	{Econet_EnumerateReceive,		 "Econet_EnumerateReceive"},
	{Econet_StartTransmit,			 "Econet_StartTransmit"},
	{Econet_PollTransmit,			 "Econet_PollTransmit"},
	{Econet_AbandonTransmit,		 "Econet_AbandonTransmit"},
	{Econet_DoTransmit, 			 "Econet_DoTransmit"},
	{Econet_ReadLocalStationAndNet,  "Econet_ReadLocalStationAndNet"},
	{Econet_ConvertStatusToString,	 "Econet_ConvertStatusToString"},
	{Econet_ConvertStatusToError,	 "Econet_ConvertStatusToError"},
	{Econet_ReadProtection, 		 "Econet_ReadProtection"},
	{Econet_SetProtection,			 "Econet_SetProtection"},
	{Econet_ReadStationNumber,		 "Econet_ReadStationNumber"},
	{Econet_PrintBanner,			 "Econet_PrintBanner"},
	{Econet_ReleasePort,			 "Econet_ReleasePort"},
	{Econet_AllocatePort,			 "Econet_AllocatePort"},
	{Econet_DeAllocatePort, 		 "Econet_DeAllocatePort"},
	{Econet_ClaimPort,				 "Econet_ClaimPort"},
	{Econet_StartImmediate, 		 "Econet_StartImmediate"},
	{Econet_DoImmediate,			 "Econet_DoImmediate"},
	{NetFS_ReadFSNumber,			 "NetFS_ReadFSNumber"},
	{NetFS_SetFSNumber, 			 "NetFS_SetFSNumber"},
	{NetFS_ReadFSName,				 "NetFS_ReadFSName"},
	{NetFS_SetFSName,				 "NetFS_SetFSName"},
	{NetFS_ReadCurrentContext,		 "NetFS_ReadCurrentContext"},
	{NetFS_SetCurrentContext,		 "NetFS_SetCurrentContext"},
	{NetFS_ReadFSTimeouts,			 "NetFS_ReadFSTimeouts"},
	{NetFS_SetFSTimeouts,			 "NetFS_SetFSTimeouts"},
	{NetFS_DoFSOp,					 "NetFS_DoFSOp"},
	{NetFS_EnumerateFSList, 		 "NetFS_EnumerateFSList"},
	{NetFS_EnumerateFS, 			 "NetFS_EnumerateFS"},
	{NetFS_ConvertDate, 			 "NetFS_ConvertDate"},
	{NetFS_DoFSOpToGivenFS, 		 "NetFS_DoFSOpToGivenFS"},
	{Font_CacheAddr,				 "Font_CacheAddr"},
	{Font_FindFont, 				 "Font_FindFont"},
	{Font_LoseFont, 				 "Font_LoseFont"},
	{Font_ReadDefn, 				 "Font_ReadDefn"},
	{Font_ReadInfo, 				 "Font_ReadInfo"},
	{Font_StringWidth,				 "Font_StringWidth"},
	{Font_Paint,					 "Font_Paint"},
	{Font_Caret,					 "Font_Caret"},
	{Font_ConverttoOS,				 "Font_ConverttoOS"},
	{Font_Converttopoints,			 "Font_Converttopoints"},
	{Font_SetFont,					 "Font_SetFont"},
	{Font_CurrentFont,				 "Font_CurrentFont"},
	{Font_FutureFont,				 "Font_FutureFont"},
	{Font_FindCaret,				 "Font_FindCaret"},
	{Font_CharBBox, 				 "Font_CharBBox"},
	{Font_ReadScaleFactor,			 "Font_ReadScaleFactor"},
	{Font_SetScaleFactor,			 "Font_SetScaleFactor"},
	{Font_ListFonts,				 "Font_ListFonts"},
	{Font_SetFontColours,			 "Font_SetFontColours"},
	{Font_SetPalette,				 "Font_SetPalette"},
	{Font_ReadThresholds,			 "Font_ReadThresholds"},
	{Font_SetThresholds,			 "Font_SetThresholds"},
	{Font_FindCaretJ,				 "Font_FindCaretJ"},
	{Font_StringBBox,				 "Font_StringBBox"},
	{Font_ReadColourTable,			 "Font_ReadColourTable"},
	{Wimp_Initialise,				 "Wimp_Initialise"},
	{Wimp_CreateWindow, 			 "Wimp_CreateWindow"},
	{Wimp_CreateIcon,				 "Wimp_CreateIcon"},
	{Wimp_DeleteWindow, 			 "Wimp_DeleteWindow"},
	{Wimp_DeleteIcon,				 "Wimp_DeleteIcon"},
	{Wimp_OpenWindow,				 "Wimp_OpenWindow"},
	{Wimp_CloseWindow,				 "Wimp_CloseWindow"},
	{Wimp_Poll, 					 "Wimp_Poll"},
	{Wimp_RedrawWindow, 			 "Wimp_RedrawWindow"},
	{Wimp_UpdateWindow, 			 "Wimp_UpdateWindow"},
	{Wimp_GetRectangle, 			 "Wimp_GetRectangle"},
	{Wimp_GetWindowState,			 "Wimp_GetWindowState"},
	{Wimp_GetWindowInfo,			 "Wimp_GetWindowInfo"},
	{Wimp_SetIconState, 			 "Wimp_SetIconState"},
	{Wimp_GetIconState, 			 "Wimp_GetIconState"},
	{Wimp_GetPointerInfo,			 "Wimp_GetPointerInfo"},
	{Wimp_DragBox,					 "Wimp_DragBox"},
	{Wimp_ForceRedraw,				 "Wimp_ForceRedraw"},
	{Wimp_SetCaretPosition, 		 "Wimp_SetCaretPosition"},
	{Wimp_GetCaretPosition, 		 "Wimp_GetCaretPosition"},
	{Wimp_CreateMenu,				 "Wimp_CreateMenu"},
	{Wimp_DecodeMenu,				 "Wimp_DecodeMenu"},
	{Wimp_WhichIcon,				 "Wimp_WhichIcon"},
	{Wimp_SetExtent,				 "Wimp_SetExtent"},
	{Wimp_SetPointerShape,			 "Wimp_SetPointerShape"},
	{Wimp_OpenTemplate, 			 "Wimp_OpenTemplate"},
	{Wimp_CloseTemplate,			 "Wimp_CloseTemplate"},
	{Wimp_LoadTemplate, 			 "Wimp_LoadTemplate"},
	{Wimp_ProcessKey,				 "Wimp_ProcessKey"},
	{Wimp_CloseDown,				 "Wimp_CloseDown"},
	{Wimp_StartTask,				 "Wimp_StartTask"},
	{Wimp_ReportError,				 "Wimp_ReportError"},
	{Wimp_GetWindowOutline, 		 "Wimp_GetWindowOutline"},
	{Wimp_PollIdle, 				 "Wimp_PollIdle"},
	{Wimp_PlotIcon, 				 "Wimp_PlotIcon"},
	{Wimp_SetMode,					 "Wimp_SetMode"},
	{Wimp_SetPalette,				 "Wimp_SetPalette"},
	{Wimp_ReadPalette,				 "Wimp_ReadPalette"},
	{Wimp_SetColour,				 "Wimp_SetColour"},
	{Wimp_SendMessage,				 "Wimp_SendMessage"},
	{Wimp_CreateSubMenu,			 "Wimp_CreateSubMenu"},
	{Wimp_SpriteOp, 				 "Wimp_SpriteOp"},
	{Wimp_BaseOfSprites,			 "Wimp_BaseOfSprites"},
	{Wimp_BlockCopy,				 "Wimp_BlockCopy"},
	{Wimp_SlotSize, 				 "Wimp_SlotSize"},
	{Wimp_ReadPixTrans, 			 "Wimp_ReadPixTrans"},
	{Wimp_ClaimFreeMemory,			 "Wimp_ClaimFreeMemory"},
	{Wimp_CommandWindow,			 "Wimp_CommandWindow"},
	{Wimp_TextColour,				 "Wimp_TextColour"},
	{Wimp_TransferBlock,			 "Wimp_TransferBlock"},
	{Wimp_ReadSysInfo,				 "Wimp_ReadSysInfo"},
	{Wimp_SetFontColours,			 "Wimp_SetFontColours"},
	{Sound_Configure,				 "Sound_Configure"},
	{Sound_Enable,					 "Sound_Enable"},
	{Sound_Stereo,					 "Sound_Stereo"},
	{Sound_Speaker, 				 "Sound_Speaker"},
	{Sound_Volume,					 "Sound_Volume"},
	{Sound_SoundLog,				 "Sound_SoundLog"},
	{Sound_LogScale,				 "Sound_LogScale"},
	{Sound_InstallVoice,			 "Sound_InstallVoice"},
	{Sound_RemoveVoice, 			 "Sound_RemoveVoice"},
	{Sound_AttachVoice, 			 "Sound_AttachVoice"},
	{Sound_ControlPacked,			 "Sound_ControlPacked"},
	{Sound_Tuning,					 "Sound_Tuning"},
	{Sound_Pitch,					 "Sound_Pitch"},
	{Sound_Control, 				 "Sound_Control"},
	{Sound_AttachNamedVoice,		 "Sound_AttachNamedVoice"},
	{Sound_ReadControlBlock,		 "Sound_ReadControlBlock"},
	{Sound_WriteControlBlock,		 "Sound_WriteControlBlock"},
	{Sound_QInit,					 "Sound_QInit"},
	{Sound_QSchedule,				 "Sound_QSchedule"},
	{Sound_QRemove, 				 "Sound_QRemove"},
	{Sound_QFree,					 "Sound_QFree"},
	{Sound_QSDispatch,				 "Sound_QSDispatch"},
	{Sound_QTempo,					 "Sound_QTempo"},
	{Sound_QBeat,					 "Sound_QBeat"},
	{Sound_QInterface,				 "Sound_QInterface"},
	{NetPrint_ReadPSNumber, 		 "NetPrint_ReadPSNumber"},
	{NetPrint_SetPSNumber,			 "NetPrint_SetPSNumber"},
	{NetPrint_ReadPSName,			 "NetPrint_ReadPSName"},
	{NetPrint_SetPSName,			 "NetPrint_SetPSName"},
	{NetPrint_ReadPSTimeouts,		 "NetPrint_ReadPSTimeouts"},
	{NetPrint_SetPSTimeouts,		 "NetPrint_SetPSTimeouts"},
	{ADFS_DiscOp,					 "ADFS_DiscOp"},
	{ADFS_HDC,						 "ADFS_HDC"},
	{ADFS_Drives,					 "ADFS_Drives"},
	{ADFS_FreeSpace,				 "ADFS_FreeSpace"},
	{ADFS_Retries,					 "ADFS_Retries"},
	{ADFS_DescribeDisc, 			 "ADFS_DescribeDisc"},
	{Podule_ReadID, 				 "Podule_ReadID"},
	{Podule_ReadHeader, 			 "Podule_ReadHeader"},
	{Podule_EnumerateChunks,		 "Podule_EnumerateChunks"},
	{Podule_ReadChunk,				 "Podule_ReadChunk"},
	{Podule_ReadBytes,				 "Podule_ReadBytes"},
	{Podule_WriteBytes, 			 "Podule_WriteBytes"},
	{Podule_CallLoader, 			 "Podule_CallLoader"},
	{Podule_RawRead,				 "Podule_RawRead"},
	{Podule_RawWrite,				 "Podule_RawWrite"},
	{Podule_HardwareAddress,		 "Podule_HardwareAddress"},
	{WaveSynth_Load,				 "WaveSynth_Load"},
	{Debugger_Disassemble,			 "Debugger_Disassemble"},
	{FPEmulator_Version,			 "FPEmulator_Version"},
	{FileCore_DiscOp,				 "FileCore_DiscOp"},
	{FileCore_Create,				 "FileCore_Create"},
	{FileCore_Drives,				 "FileCore_Drives"},
	{FileCore_FreeSpace,			 "FileCore_FreeSpace"},
	{FileCore_FloppyStructure,		 "FileCore_FloppyStructure"},
	{FileCore_DescribeDisc, 		 "FileCore_DescribeDisc"},
	{Shell_Create,					 "Shell_Create"},
	{Shell_Destroy, 				 "Shell_Destroy"},
	{Hourglass_On,					 "Hourglass_On"},
	{Hourglass_Off, 				 "Hourglass_Off"},
	{Hourglass_Smash,				 "Hourglass_Smash"},
	{Hourglass_Start,				 "Hourglass_Start"},
	{Hourglass_Percentage,			 "Hourglass_Percentage"},
	{Hourglass_LEDs,				 "Hourglass_LEDs"},
	{Draw_ProcessPath,				 "Draw_ProcessPath"},
	{Draw_ProcessPathFP,			 "Draw_ProcessPathFP"},
	{Draw_Fill, 					 "Draw_Fill"},
	{Draw_FillFP,					 "Draw_FillFP"},
	{Draw_Stroke,					 "Draw_Stroke"},
	{Draw_StrokeFP, 				 "Draw_StrokeFP"},
	{Draw_StrokePath,				 "Draw_StrokePath"},
	{Draw_StrokePathFP, 			 "Draw_StrokePathFP"},
	{Draw_FlattenPath,				 "Draw_FlattenPath"},
	{Draw_FlattenPathFP,			 "Draw_FlattenPathFP"},
	{Draw_TransformPath,			 "Draw_TransformPath"},
	{Draw_TransformPathFP,			 "Draw_TransformPathFP"},
	{RamFS_DiscOp,					 "RamFS_DiscOp"},
	{RamFS_Drives,					 "RamFS_Drives"},
	{RamFS_FreeSpace,				 "RamFS_FreeSpace"},
	{RamFS_DescribeDisc,			 "RamFS_DescribeDisc"}
};

unsigned DasmARM(char *buffer, unsigned pc)
{
    UINT32 op = cpu_readmem26lew_dword(pc);
	int i;

	buffer[0] = '\0';
    switch( (op>>20) & 0xff )
	{
	case 0x00: sprintf(buffer, "AND%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x01: sprintf(buffer, "ANDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x02: sprintf(buffer, "EOR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x03: sprintf(buffer, "EORS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x04: sprintf(buffer, "SUB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x05: sprintf(buffer, "SUBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x06: sprintf(buffer, "RSB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x07: sprintf(buffer, "RSBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x08: sprintf(buffer, "ADD%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x09: sprintf(buffer, "ADDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0a: sprintf(buffer, "ADC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0b: sprintf(buffer, "ADCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0c: sprintf(buffer, "SBC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0d: sprintf(buffer, "SBCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0e: sprintf(buffer, "RSC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0f: sprintf(buffer, "RSCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x10: sprintf(buffer, "TST%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x11: sprintf(buffer, "TSTS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x12: sprintf(buffer, "TEQ%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x13: sprintf(buffer, "TEQS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x14: sprintf(buffer, "CMP%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x15: sprintf(buffer, "CMPS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x16: sprintf(buffer, "CMN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x17: sprintf(buffer, "CMNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x18: sprintf(buffer, "ORR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x19: sprintf(buffer, "ORRS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1a: sprintf(buffer, "MOV%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1b: sprintf(buffer, "MOVS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1c: sprintf(buffer, "BIC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1d: sprintf(buffer, "BICS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1e: sprintf(buffer, "MVN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1f: sprintf(buffer, "MVNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x20: sprintf(buffer, "AND%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x21: sprintf(buffer, "ANDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x22: sprintf(buffer, "EOR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x23: sprintf(buffer, "EORS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x24: sprintf(buffer, "SUB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x25: sprintf(buffer, "SUBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x26: sprintf(buffer, "RSB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x27: sprintf(buffer, "RSBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x28: sprintf(buffer, "ADD%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x29: sprintf(buffer, "ADDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2a: sprintf(buffer, "ADC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2b: sprintf(buffer, "ADCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2c: sprintf(buffer, "SBC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2d: sprintf(buffer, "SBCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2e: sprintf(buffer, "RSC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2f: sprintf(buffer, "RSCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x30: sprintf(buffer, "TST%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x31: sprintf(buffer, "TSTS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x32: sprintf(buffer, "TEQ%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x33: sprintf(buffer, "TEQS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x34: sprintf(buffer, "CMP%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x35: sprintf(buffer, "CMPS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x36: sprintf(buffer, "CMN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x37: sprintf(buffer, "CMNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x38: sprintf(buffer, "ORR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x39: sprintf(buffer, "ORRS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3a: sprintf(buffer, "MOV%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3b: sprintf(buffer, "MOVS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3c: sprintf(buffer, "BIC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3d: sprintf(buffer, "BICS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3e: sprintf(buffer, "MVN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3f: sprintf(buffer, "MVNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x40: sprintf(buffer, "STR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x41: sprintf(buffer, "LDR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x42: sprintf(buffer, "STR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x43: sprintf(buffer, "LDR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x44: sprintf(buffer, "STRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x45: sprintf(buffer, "LDRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x46: sprintf(buffer, "STRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x47: sprintf(buffer, "LDRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x48: sprintf(buffer, "STR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x49: sprintf(buffer, "LDR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4a: sprintf(buffer, "STR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4b: sprintf(buffer, "LDR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4c: sprintf(buffer, "STRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4d: sprintf(buffer, "LDRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4e: sprintf(buffer, "STRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4f: sprintf(buffer, "LDRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x50: sprintf(buffer, "STR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x51: sprintf(buffer, "LDR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x52: sprintf(buffer, "STR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x53: sprintf(buffer, "LDR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x54: sprintf(buffer, "STRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x55: sprintf(buffer, "LDRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x56: sprintf(buffer, "STRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x57: sprintf(buffer, "LDRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x58: sprintf(buffer, "STR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x59: sprintf(buffer, "LDR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5a: sprintf(buffer, "STR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5b: sprintf(buffer, "LDR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5c: sprintf(buffer, "STRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5d: sprintf(buffer, "LDRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5e: sprintf(buffer, "STRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5f: sprintf(buffer, "LDRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x60: sprintf(buffer, "STR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x61: sprintf(buffer, "LDR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x62: sprintf(buffer, "STR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x63: sprintf(buffer, "LDR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x64: sprintf(buffer, "STRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x65: sprintf(buffer, "LDRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x66: sprintf(buffer, "STRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x67: sprintf(buffer, "LDRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x68: sprintf(buffer, "STR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x69: sprintf(buffer, "LDR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6a: sprintf(buffer, "STR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6b: sprintf(buffer, "LDR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6c: sprintf(buffer, "STRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6d: sprintf(buffer, "LDRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6e: sprintf(buffer, "STRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6f: sprintf(buffer, "LDRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x70: sprintf(buffer, "STR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x71: sprintf(buffer, "LDR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x72: sprintf(buffer, "STR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x73: sprintf(buffer, "LDR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x74: sprintf(buffer, "STRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x75: sprintf(buffer, "LDRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x76: sprintf(buffer, "STRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x77: sprintf(buffer, "LDRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x78: sprintf(buffer, "STR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x79: sprintf(buffer, "LDR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7a: sprintf(buffer, "STR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7b: sprintf(buffer, "LDR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7c: sprintf(buffer, "STRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7d: sprintf(buffer, "LDRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7e: sprintf(buffer, "STRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7f: sprintf(buffer, "LDRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x80: sprintf(buffer, "STMDA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x81: sprintf(buffer, "LDMDA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x82: sprintf(buffer, "STMDA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x83: sprintf(buffer, "LDMDA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x84: sprintf(buffer, "STMDAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x85: sprintf(buffer, "LDMDAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x86: sprintf(buffer, "STMDAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x87: sprintf(buffer, "LDMDAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x88: sprintf(buffer, "STMIA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x89: sprintf(buffer, "LDMIA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8a: sprintf(buffer, "STMIA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8b: sprintf(buffer, "LDMIA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8c: sprintf(buffer, "STMIAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8d: sprintf(buffer, "LDMIAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8e: sprintf(buffer, "STMIAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8f: sprintf(buffer, "LDMIAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;

	case 0x90: sprintf(buffer, "STMDB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x91: sprintf(buffer, "LDMDB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x92: sprintf(buffer, "STMDB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x93: sprintf(buffer, "LDMDB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x94: sprintf(buffer, "STMDBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x95: sprintf(buffer, "LDMDBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x96: sprintf(buffer, "STMDBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x97: sprintf(buffer, "LDMDBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x98: sprintf(buffer, "STMIB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x99: sprintf(buffer, "LDMIB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9a: sprintf(buffer, "STMIB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9b: sprintf(buffer, "LDMIB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9c: sprintf(buffer, "STMIBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9d: sprintf(buffer, "LDMIBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9e: sprintf(buffer, "STMIBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9f: sprintf(buffer, "LDMIBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;

	case 0xa0: case 0xa1: case 0xa2: case 0xa3:
	case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xa8: case 0xa9: case 0xaa: case 0xaf:
		sprintf(buffer, "B%s      $%x", cc[(op>>28)&15], (pc + 4*(op&0xffffff)) & 0x3ffffffc);
		break;
	case 0xb0: case 0xb1: case 0xb2: case 0xb3:
	case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbf:
		sprintf(buffer, "BL%s     $%x", cc[(op>>28)&15], (pc + 4*(op&0xffffff)) & 0x3ffffffc);
        break;
	case 0xc0: case 0xc1: case 0xc2: case 0xc3:
	case 0xc4: case 0xc5: case 0xc6: case 0xc7:
	case 0xc8: case 0xc9: case 0xca: case 0xcf:
		sprintf(buffer, "INVC%s   $%08x", cc[(op>>28)&15], op);
        break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
	case 0xd4: case 0xd5: case 0xd6: case 0xd7:
	case 0xd8: case 0xd9: case 0xda: case 0xdf:
		sprintf(buffer, "INVD%s   $%08x", cc[(op>>28)&15], op);
		break;
	case 0xe0: case 0xe1: case 0xe2: case 0xe3:
	case 0xe4: case 0xe5: case 0xe6: case 0xe7:
	case 0xe8: case 0xe9: case 0xea: case 0xef:
		sprintf(buffer, "INVE%s   $%08x", cc[(op>>28)&15], op);
        break;
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:
	case 0xf4: case 0xf5: case 0xf6: case 0xf7:
	case 0xf8: case 0xf9: case 0xfa: case 0xff:
		for (i = 0; i < sizeof(SWI)/sizeof(SWI[0]); i++)
		{
			if (SWI[i].swi == (op & 0xffffff))
			{
				sprintf(buffer, "SWI%s    %s", cc[(op>>28)&15], SWI[i].name);
				return 4;
			}
        }
		sprintf(buffer, "SWI%s    $%08x", cc[(op>>28)&15], op);
		break;
	default:
		sprintf(buffer, "???%s    $%08x", cc[(op>>28)&15], op);
    }

    return 4;
}
