/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "libemu.h"
#include "global.h"
#include "os/endian.h"
#include "os/math.h"
#include "os/strings.h"
#include "driver.h"
#include "file.h"
#include "interrupt.h"
#include "mt32mpu.h"
#include "tools.h"

extern void f__01F7_27FD_0037_E2C0();
extern void emu_Tools_Malloc();
extern void emu_Tools_Free();
extern void emu_Tools_GetFreeMemory();
extern void emu_Highmem_GetSize();
extern void emu_Highmem_IsInHighmem();
extern void Game_Timer_Interrupt();
extern void emu_Tools_PrintDebug();
extern void emu_MPU_TestPort();
extern void emu_DSP_GetInfo();
extern void emu_DSP_TestPort();
extern void emu_MPU_GetInfo();
extern void f__AB00_0B91_0014_89BD();
extern void f__AB00_0C08_0013_3E08();
extern void emu_DSP_Init();
extern void f__AB00_1068_0020_E6F1();
extern void f__AB00_1122_001C_9408();
extern void f__AB00_118F_0029_4B06();
extern void f__AB00_1235_0013_28BA();
extern void f__AB01_0F24_0044_3584();
extern void emu_MPU_Init();
extern void f__AB01_2103_0040_93D2();
extern void emu_MPU_ClearData();
extern void f__AB01_26EB_0047_41F4();

static uint16 _stat04;
static uint16 _stat06;
static csip32 _stat08[17];
static uint16 _stat4C[17];
static uint16 _stat6E[17];
static uint32 _stat90[17];
static uint32 _statD4[17];
static uint32 _stat118;
static csip32 _stat11C;
static uint16 _stat120;
static uint32 _stat122;
static uint16 _stat126;
static csip32 _stat128[16];
static uint16 _stat168[16];
static uint16 _stat188[16];
static uint16 _stat1AC;
static uint16 _stat1B2;
static uint32 *_stat1B4 = (uint32 *)&emu_get_memory8(0x2756, 0x00, 0x1B4);
static csip32 _stat3B8;
static uint16 _stat460;
static uint16 _stat462;
static uint16 _stat464;
static csip32 _stat466;

static void Drivers_CustomTimer_Clear()
{
	_stat118 = 0xFFFFFFFF;
	memset(_stat6E, 0, 34);
	memset(_stat90, 0, 68);
	memset(_statD4, 0, 68);
}

static void Drivers_CustomTimer_InstallInterrupt()
{
	_stat11C = emu_get_csip32(0x00, 0x00, 0x20);
	_stat08[16].csip = 0x27560622;
	emu_get_csip32(0x00, 0x00, 0x20).csip = 0x2756050F;
}

static void Drivers_CustomTimer_EnableHandler(uint16 index)
{
	if (_stat6E[index] != 1) return;

	_stat6E[index] = 2;
}

static void Drivers_CustomTimer_DisableHandler(uint16 index) {
	if (_stat6E[index] == 2) _stat6E[index] = 1;
}

static void Drivers_CustomTimer_06A9(uint16 value)
{
	emu_outb(0x43, 0x36);
	_stat126 = value;
	emu_outb(0x40, value & 0xFF);
	emu_outb(0x40, value >> 8);
}

static void Drivers_CustomTimer_06D2(uint16 value)
{
	if (value >= 54925) {
		value = 0;
	} else {
		value = value * 10000 / 8380;
	}

	Drivers_CustomTimer_06A9(value);
}

static void Drivers_CustomTimer_0746()
{
	uint8 i;

	_stat122 = 0xFFFFFFFF;

	for (i = 0; i <= 16; i++) {
		if (_stat6E[i] == 0) continue;
		if (_statD4[i] < _stat122) _stat122 = _statD4[i];
	}

	if (_stat122 == _stat118) return;

	_stat120 = 0xFFFF;
	_stat118 = _stat122;

	Drivers_CustomTimer_06D2(_stat118 & 0xFFFF);

	memset(_stat90, 0, 68);
}

static void Drivers_CustomTimer_SetDelay(uint16 index, uint32 delay)
{
	uint16 old6E;

	old6E = _stat6E[index];
	_stat6E[index] = 1;

	_statD4[index] = delay;
	_stat90[index] = 0;

	Drivers_CustomTimer_0746();

	_stat6E[index] = old6E;
}

static void Drivers_CustomTimer_SetFrequency(uint16 index, uint32 freq)
{
	assert(freq != 0);
	Drivers_CustomTimer_SetDelay(index, 1000000 / freq);
}

static uint16 Drivers_CustomTimer_AddHandler(csip32 csip)
{
	uint8 i;

	for (i = 0; i < 16; i++) if (_stat6E[i] == 0) break;
	if (i == 16) return 0xFFFF;

	_stat6E[i] = 1;
	_stat4C[i] = 0x353F;
	_stat08[i] = csip;

	_stat04++;
	if (_stat04 != 1) return i;

	Drivers_CustomTimer_Clear();

	_stat6E[16] = 1;

	Drivers_CustomTimer_InstallInterrupt();

	Drivers_CustomTimer_SetDelay(16, 54925);

	Drivers_CustomTimer_EnableHandler(16);

	_stat6E[i] = 1;

	return i;
}

static void Drivers_CustomTimer_UninstallInterrupt()
{
	emu_get_csip32(0x00, 0x00, 0x20) = _stat11C;
	_stat120 = 0xFFFF;
}

static void Drivers_CustomTimer_RemoveHandler(uint16 index)
{
	if (index == 0xFFFF || _stat6E[index] == 0) return;

	_stat6E[index] = 0;

	_stat04--;
	if (_stat04 != 0) return;

	Drivers_CustomTimer_06A9(0);

	Drivers_CustomTimer_UninstallInterrupt();
}

void Drivers_CustomTimer_Interrupt()
{
	if (_stat06 == 0) {
		_stat06 = 1;

		emu_push(emu_ax);
		emu_push(emu_bx);
		emu_push(emu_cx);
		emu_push(emu_dx);
		emu_push(emu_si);
		emu_push(emu_di);
		emu_push(emu_bp);
		emu_push(emu_es);
		emu_push(emu_ds);

		_stat3B8.s.cs = emu_ss;
		_stat3B8.s.ip = emu_sp;
		emu_ss = 0x2756;
		emu_sp = 0x03B8;

		for (_stat120 = 0; _stat120 <= 16; _stat120++) {
			if (_stat6E[_stat120] != 2) continue;

			emu_ds = _stat4C[_stat120];

			_stat90[_stat120] += _stat118;
			if (_stat90[_stat120] < _statD4[_stat120]) continue;
			_stat90[_stat120] -= _statD4[_stat120];

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x05A2); emu_cs = _stat08[_stat120].s.cs;
			switch (_stat08[_stat120].csip) {
				case 0x27560622: emu_Drivers_CustomTimer_OriginalInterrupt(); break;
				case 0x2BD10006: Game_Timer_Interrupt(); break;
				case 0x44AF1CEE: emu_MPU_Interrupt(); break;
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x2756; emu_last_ip = 0x059D; emu_last_length = 0x0019; emu_last_crc = 0x7966;
					emu_call();
					return;
			}
		}

		emu_ss = _stat3B8.s.cs;
		emu_sp = _stat3B8.s.ip;

		emu_pop(&emu_ds);
		emu_pop(&emu_es);
		emu_pop(&emu_bp);
		emu_pop(&emu_di);
		emu_pop(&emu_si);
		emu_pop(&emu_dx);
		emu_pop(&emu_cx);
		emu_pop(&emu_bx);
		emu_pop(&emu_ax);

		_stat06 = 0;
	}

	emu_outb(0x20, 0x20);

	assert(*_stat1B4 == HTOBE32('Test'));
}

void Drivers_CustomTimer_OriginalInterrupt()
{
	assert(_stat11C.csip == 0x00700040);

	emu_pushf(); emu_push(emu_cs); emu_push(0x0628); emu_cs = 0x0070; Interrupt_Timer();
}

static csip32 Drivers_Load(const char *filename, csip32 fcsip)
{
	csip32 ret;

	ret.csip = 0x0;

	if (!File_Exists(filename)) return ret;

	if (strstr(filename, ".COM") != NULL) {
		emu_push(fcsip.s.cs); emu_push(fcsip.s.ip);
		/* Unresolved call */ emu_push(emu_cs); emu_push(0x045D); emu_cs = 0x2431; emu_ip = 0x03DF; emu_last_cs = 0x1DD7; emu_last_ip = 0x0458; emu_last_length = 0x0012; emu_last_crc = 0xAB15; emu_call();
		emu_sp += 4;
		return ret; /* return value of the above unresolved call */
	}

	return File_ReadWholeFile(filename, 0x20);
}

MSVC_PACKED_BEGIN
typedef struct DriverInfo {
	/* 0000(2)   */ PACK uint16 variable_0000;              /*!< ?? */
	/* 0002()    */ PACK uint8  unknown_0002[2];
	/* 0004(4)   */ PACK char extension[4];                 /*!< ?? */
	/* 0008()    */ PACK uint8   unknown_0008[4];
	/* 000C(2)   */ PACK uint16 port;                       /*!< ?? */
	/* 000E(2)   */ PACK uint16 irq1;                       /*!< ?? */
	/* 0010(2)   */ PACK uint16 dma;                        /*!< ?? */
	/* 0012(2)   */ PACK uint16 irq2;                       /*!< ?? */
	/* 0014(2)   */ PACK uint16 variable_0014;              /*!< ?? */
} GCC_PACKED DriverInfo;
MSVC_PACKED_END
assert_compile(sizeof(DriverInfo) == 0x16);

static DriverInfo *Driver_GetInfo(uint16 driver)
{
	csip32 ret;

	emu_push(0x2756); emu_push(0x888);
	emu_push(driver); /* unused, but needed for correct param accesses. */
	ret = Drivers_CallFunction(driver, 0x64);
	emu_sp += 6;
	return (DriverInfo *)emu_get_memorycsip(ret);
}

static void Driver_Init(uint16 driver, uint16 port, uint16 irq1, uint16 dma, uint16 irq2)
{
	csip32 csip;
	uint16 locsi;

	if (driver >= 16) return;

	_stat1B2 = 0xFFFF;

	locsi = Driver_GetInfo(driver)->variable_0014;
	csip = Drivers_GetFunctionCSIP(driver, 0x67);
	if (locsi != 0xFFFF && csip.csip != 0) {
		uint16 handlerId = Drivers_CustomTimer_AddHandler(csip);
		_stat168[driver] = handlerId;
		_stat1B2 = handlerId;

		Drivers_CustomTimer_SetFrequency(handlerId, locsi);
	}

	emu_push(irq2);
	emu_push(dma);
	emu_push(irq1);
	emu_push(port);
	emu_push(driver); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(driver, 0x66);
	emu_sp += 10;

	_stat188[driver] = 1;

	if (_stat1B2 != 0xFFFF) Drivers_CustomTimer_EnableHandler(_stat1B2);
}

uint16 Drivers_EnableSounds(uint16 sounds)
{
	uint16 ret;

	ret = g_global->soundsEnabled;
	g_global->soundsEnabled = sounds;

	if (sounds == 0) Driver_Sound_Stop();

	if (g_global->soundDriver.index != 0xFFFF || g_global->soundDriver.dcontent.csip == 0x0) return ret;

	emu_ax = 0x1;
	emu_bx = g_global->soundsEnabled == 0 ? 0x10 : 0x11;
	emu_pushf();

	/* Call based on memory/register values */
	emu_ip = g_global->soundDriver.dcontent.s.ip;
	emu_push(emu_cs);
	emu_cs = g_global->soundDriver.dcontent.s.cs;
	emu_push(0x0059);
	switch ((emu_cs << 16) + emu_ip) {
		default:
			/* In case we don't know the call point yet, call the dynamic call */
			emu_last_cs = 0x1DD7; emu_last_ip = 0x0056; emu_last_length = 0x002E; emu_last_crc = 0x813E;
			emu_call();
			return ret;
	}

	return ret;
}

uint16 Drivers_EnableMusic(uint16 music)
{
	uint16 ret;

	ret = g_global->soundsEnabled; /* Looks like a bug in original code */
	g_global->musicEnabled = music;

	if (music == 0) Driver_Music_Stop();

	if (g_global->musicDriver.index != 0xFFFF || g_global->musicDriver.dcontent.csip == 0x0) return ret;

	emu_ax = 0x2;
	emu_bx = g_global->musicEnabled == 0 ? 0x10 : 0x11;
	emu_pushf();

	/* Call based on memory/register values */
	emu_ip = g_global->musicDriver.dcontent.s.ip;
	emu_push(emu_cs);
	emu_cs = g_global->musicDriver.dcontent.s.cs;
	emu_push(0x00BA);
	switch ((emu_cs << 16) + emu_ip) {
		default:
			/* In case we don't know the call point yet, call the dynamic call */
			emu_last_cs = 0x1DD7; emu_last_ip = 0x00B7; emu_last_length = 0x002E; emu_last_crc = 0xB16C;
			emu_call();
			return ret;
	}

	return ret;
}

static void Driver_Uninstall(uint16 driver)
{
	if (driver >= 16) return;

	_stat128[driver].csip = 0x0;

	if (_stat462 != 0 && _stat460 == driver) _stat462 = 0;
}

static void Driver_Uninit(uint16 driver, csip32 arg08)
{
	if (driver >= 16 || _stat188[driver] == 0) return;
	_stat188[driver] = 0;

	if (_stat168[driver] != 0xFFFF) Drivers_CustomTimer_RemoveHandler(_stat168[driver]);

	emu_push(arg08.s.cs); emu_push(arg08.s.ip);
	emu_push(driver); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(driver, 0x68);
	emu_sp += 6;
}

static void Drivers_Uninit(Driver *driver)
{
	if (driver == NULL) return;

	if (driver->customTimer != 0xFFFF) {
		Drivers_CustomTimer_DisableHandler(driver->customTimer);

		Drivers_CustomTimer_RemoveHandler(driver->customTimer);

		driver->customTimer = 0xFFFF;
	}

	if (driver->index == 0xFFFF) {
		if (driver->dcontent.csip != 0) {
			emu_bx = 0x3;
			emu_pushf();

			/* Call based on memory/register values */
			emu_ip = driver->dcontent.s.ip;
			emu_push(emu_cs);
			emu_cs = driver->dcontent.s.cs;
			emu_push(0x16FD);
			switch ((emu_cs << 16) + emu_ip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x16FA; emu_last_length = 0x0029; emu_last_crc = 0x9C96;
					emu_call();
					return;
			}
		}
	} else {
		csip32 nullcsip;
		nullcsip.csip = 0x0;

		Driver_Uninit(driver->index, nullcsip);

		Driver_Uninstall(driver->index);

		driver->index = 0xFFFF;
	}

	emu_push(driver->dcontent.s.cs); emu_push(driver->dcontent.s.ip);
	emu_push(emu_cs); emu_push(0x1737); emu_cs = 0x23E1; emu_Tools_Free();
	emu_sp += 4;

	emu_push(driver->variable_12.s.cs); emu_push(driver->variable_12.s.ip);
	emu_push(emu_cs); emu_push(0x1749); emu_cs = 0x23E1; emu_Tools_Free();
	emu_sp += 4;

	driver->variable_12.csip = 0x0;
	driver->dcontent.csip    = 0x0;
	driver->dfilename.csip   = 0x0;
}

static uint16 Driver_Install(csip32 dcontent)
{
	uint8  *content = emu_get_memorycsip(dcontent);
	DriverInfo *info;

	for (_stat1AC = 0; _stat1AC < 16; _stat1AC++) {
		if (_stat128[_stat1AC].csip == 0) break;
	}
	if (_stat1AC == 16) return 0xFFFF;

	if (strncmp((char *)(content + 3), "DIGPAK", 6) == 0) {
		if (_stat462 != 0) return 0xFFFF;
		_stat462 = 1;
		_stat460 = 0xFFFF;
		_stat466 = dcontent;
		_stat466.s.cs -= 0x10;
		_stat466.s.ip += 0x100;
		_stat128[_stat1AC].csip = 0x275603BE;
		_stat464 = 0;
		return _stat1AC;
	}

	if (strncmp((char *)(content + 2), "Copy", 4) != 0) return 0xFFFF;

	_stat128[_stat1AC] = dcontent;
	_stat128[_stat1AC].s.ip += ((uint16 *)content)[0];

	info = Driver_GetInfo(_stat1AC);
	if (info == NULL || info->variable_0000 > 211) return 0xFFFF;

	return _stat1AC;
}

static bool Drivers_Init(const char *filename, csip32 fcsip, Driver *driver, const char *extension, uint16 variable_0008)
{
	if (filename == NULL || !File_Exists(filename)) return false;

	if (driver->dcontent.csip != 0) {
		if (strcasecmp((char *)emu_get_memorycsip(driver->dfilename), filename) == 0) return true;
		Drivers_Uninit(driver);
	}

	driver->dcontent = Drivers_Load(filename, fcsip);

	if (driver->dcontent.csip == 0) return false;

	if (variable_0008 != 0) {
		emu_bx = 0x2;
		emu_pushf();

		/* Call based on memory/register values */
		emu_ip = driver->dcontent.s.ip;
		emu_push(emu_cs);
		emu_cs = driver->dcontent.s.cs;
		emu_push(0x136D);
		switch ((emu_cs << 16) + emu_ip) {
			default:
				/* In case we don't know the call point yet, call the dynamic call */
				emu_last_cs = 0x1DD7; emu_last_ip = 0x136A; emu_last_length = 0x0007; emu_last_crc = 0x2888;
				emu_call();
				return false;
		}

		if ((variable_0008 & 0x8000) != 0) {
			emu_ax = 0x4;
			emu_bx = 0x10;
			emu_pushf();

			/* Call based on memory/register values */
			emu_ip = driver->dcontent.s.ip;
			emu_push(emu_cs);
			emu_cs = driver->dcontent.s.cs;
			emu_push(0x137B);
			switch ((emu_cs << 16) + emu_ip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1378; emu_last_length = 0x000E; emu_last_crc = 0xB970;
					emu_call();
					return false;
			}
		}

		{
			csip32 csip = driver->dcontent;
			csip.s.ip += 3;
			driver->customTimer = Drivers_CustomTimer_AddHandler(csip);
		}
		if (driver->customTimer == 0xFFFF) {
			emu_push(driver->dcontent.s.cs); emu_push(driver->dcontent.s.ip);
			emu_push(emu_cs); emu_push(0x13B2); emu_cs = 0x23E1; emu_Tools_Free();
			emu_sp += 4;

			driver->dcontent.csip = 0;
			return false;
		}

		Drivers_CustomTimer_SetFrequency(driver->customTimer, abs(variable_0008));

		Drivers_CustomTimer_EnableHandler(driver->customTimer);

		strcpy(driver->extension, (strcasecmp(filename, "alfx.drv") == 0) ? "adl" : "snd");
	} else {
		DriverInfo *info;

		driver->index = Driver_Install(driver->dcontent);

		if (driver->index == 0xFFFF) {
			emu_push(driver->dcontent.s.cs); emu_push(driver->dcontent.s.ip);
			emu_push(emu_cs); emu_push(0x13B2); emu_cs = 0x23E1; emu_Tools_Free();
			emu_sp += 4;

			driver->dcontent.csip = 0;
			return false;
		}

		info = Driver_GetInfo(driver->index);

		memcpy(driver->extension2, info->extension, 4);
		strcpy(driver->extension, "xmi");

		if (strcasecmp(filename, "sbdig.adv") == 0 || strcasecmp(filename, "sbpdig.adv") == 0) {
			char *blaster;

			emu_push(emu_ds); emu_push(0x65FE); /* "BLASTER" */
			emu_push(emu_cs); emu_push(0x14CF); emu_cs = 0x01F7; f__01F7_27FD_0037_E2C0();
			emu_sp += 4;
			blaster = (char *)&emu_get_memory8(emu_dx, emu_ax, 0x0);

			if (blaster != NULL) {
				char *val;

				val = strchr(blaster, 'A');
				if (val != NULL) {
					val++;
					info->port = (uint16)strtoul(val, NULL, 16);
				}

				val = strchr(blaster, 'I');
				if (val != NULL) {
					val++;
					info->irq1 = info->irq2 = (uint16)strtoul(val, NULL, 10);
				}

				val = strchr(blaster, 'D');
				if (val != NULL) {
					val++;
					info->dma = (uint16)strtoul(val, NULL, 10);
				}
			}
		}

		emu_push(info->irq2);
		emu_push(info->dma);
		emu_push(info->irq1);
		emu_push(info->port);
		emu_push(driver->index); /* unused, but needed for correct param accesses. */
		emu_ax = Drivers_CallFunction(driver->index, 0x65).s.ip;
		emu_sp += 8;

		if (emu_ax == 0) {
			Driver_Uninstall(driver->index);

			emu_push(driver->dcontent.s.cs); emu_push(driver->dcontent.s.ip);
			emu_push(emu_cs); emu_push(0x13B2); emu_cs = 0x23E1; emu_Tools_Free();
			emu_sp += 4;

			driver->dcontent.csip = 0;
			return false;
		}

		Driver_Init(driver->index, info->port, info->irq1, info->dma, info->irq2);

		{
			int32 value = (int32)Drivers_CallFunction(driver->index, 0x99).s.ip;
			if (value != 0) {
				emu_push(0);
				emu_push(value >> 16); emu_push(value & 0xFFFF);
				emu_push(emu_cs); emu_push(0x1625); emu_cs = 0x23E1; emu_Tools_Malloc();
				emu_sp += 6;

				driver->variable_12.s.cs = emu_dx;
				driver->variable_12.s.ip = emu_ax;

				emu_push(value & 0xFFFF);
				emu_push(driver->variable_12.s.cs); emu_push(driver->variable_12.s.ip);
				emu_push(driver->index); /* unused, but needed for correct param accesses. */
				Drivers_CallFunction(driver->index, 0x9A);
				emu_sp += 8;
			}
		}
	}

	driver->dfilename = fcsip;
	driver->extension[0] = 0;
	if (driver->dcontent.csip != 0) memcpy(driver->extension, extension, 4);

	return true;
}

uint16 Drivers_Sound_Init(uint16 index)
{
	char *filename;
	MSDriver *driver;
	Driver *sound;
	Driver *music;

	driver = &g_global->soundDrv[index];
	sound  = &g_global->soundDriver;
	music  = &g_global->musicDriver;

	if (driver->filename.csip == 0x0) return index;

	filename = (char *)emu_get_memorycsip(driver->filename);

	if (music->dfilename.csip != 0x0 && !strcasecmp((char *)emu_get_memorycsip(music->dfilename), filename)) {
		memcpy(sound, music, sizeof(Driver));
	} else {
		if (!Drivers_Init(filename, driver->filename, sound, (char *)emu_get_memorycsip(driver->extension), driver->variable_0008)) return 0;
	}

	if (driver->variable_0008 == 0) {
		uint8 i;
		int32 value;

		value = (int32)Drivers_CallFunction(sound->index, 0x96).s.ip;

		for (i = 0; i < 4; i++) {
			MSBuffer *buf = &g_global->soundBuffer[i];

			emu_push(0x10);
			emu_push(value >> 16); emu_push(value & 0xFFFF);
			emu_push(emu_cs); emu_push(0x1014); emu_cs = 0x23E1; emu_Tools_Malloc();
			emu_sp += 6;

			buf->buffer.s.cs = emu_dx;
			buf->buffer.s.ip = emu_ax;
			buf->index       = 0xFFFF;
		}
		g_global->soundBufferIndex = 0;
	}

	if (driver->variable_000A != 0) {
		g_global->variable_6328 = 1;
	}

	return index;
}

uint16 Drivers_Music_Init(uint16 index)
{
	char *filename;
	MSDriver *driver;
	Driver *music;
	Driver *sound;
	int32 value;

	driver = &g_global->musicDrv[index];
	music  = &g_global->musicDriver;
	sound  = &g_global->soundDriver;

	if (driver->filename.csip == 0x0) return index;

	filename = (char *)emu_get_memorycsip(driver->filename);

	if (sound->dfilename.csip != 0x0 && !strcasecmp((char *)emu_get_memorycsip(sound->dfilename), filename)) {
		memcpy(music, sound, sizeof(Driver));
	} else {
		if (!Drivers_Init(filename, driver->filename, music, (char *)emu_get_memorycsip(driver->extension), driver->variable_0008)) return 0;
	}

	g_global->variable_636A = driver->variable_000A;
	if (driver->variable_0008 != 0) return index;

	value = (int32)Drivers_CallFunction(music->index, 0x96).s.ip;

	emu_push(0x10);
	emu_push(value >> 16); emu_push(value & 0xFFFF);
	emu_push(emu_cs); emu_push(0x1014); emu_cs = 0x23E1; emu_Tools_Malloc();
	emu_sp += 6;

	g_global->musicBuffer.buffer.s.cs = emu_dx;
	g_global->musicBuffer.buffer.s.ip = emu_ax;
	g_global->musicBuffer.index       = 0xFFFF;

	return index;
}

uint16 Drivers_Voice_Init(uint16 index)
{
	char *filename;
	DSDriver *driver;
	Driver *voice;

	driver = &g_global->voiceDrv[index];
	voice  = &g_global->voiceDriver;

	if (driver->filename.csip == 0x0) return index;

	filename = (char *)emu_get_memorycsip(driver->filename);

	if (!Drivers_Init(filename, driver->filename, voice, "VOC", 0)) return 0;

	return index;
}

static void Drivers_Reset()
{
	_stat04 = 0;
	_stat06 = 0;
	_stat462 = 0;
	memset(_stat128, 0, 64);
	memset(_stat168, 0xFF, 32);
	memset(_stat188, 0, 32);
}

void Drivers_All_Init(uint16 sound, uint16 music, uint16 voice)
{
	Drivers_Reset();

	{
		csip32 csip;
		csip.csip = 0x2BD10006;
		g_global->variable_639C = Drivers_CustomTimer_AddHandler(csip);
	}

	Drivers_CustomTimer_SetFrequency(g_global->variable_639C, 60);

	Drivers_CustomTimer_EnableHandler(g_global->variable_639C);

	g_global->variable_6D8D = Drivers_Music_Init(music);
	g_global->variable_6D8B = Drivers_Sound_Init(sound);
	g_global->variable_6D8F = Drivers_Voice_Init(voice);
}

csip32 Drivers_GetFunctionCSIP(uint16 driver, uint16 function)
{
	csip32 csip;

	if (driver >= 16) {
		csip.csip = 0;
		return csip;
	}

	csip = _stat128[driver];
	if (csip.csip == 0) return csip;

	for (;; csip.s.ip += 4) {
		uint16 f = emu_get_memory16(csip.s.cs, csip.s.ip, 0);
		if (f == function) break;
		if (f != 0xFFFF) continue;
		csip.csip = 0;
		return csip;
	}

	csip.s.ip = emu_get_memory16(csip.s.cs, csip.s.ip, 2);
	return csip;
}

csip32 Drivers_CallFunction(uint16 driver, uint16 function)
{
	csip32 csip;

	csip = Drivers_GetFunctionCSIP(driver, function);
	if (csip.csip == 0) return csip;

	/* Call/jump based on memory/register values */
	emu_push(emu_cs); emu_push(emu_ip);
	emu_ip = csip.s.ip;
	emu_cs = csip.s.cs;
	switch ((emu_cs << 16) + emu_ip) {
		case 0x44AF045A: emu_MPU_TestPort(); break; /* 0x65 */
		case 0x44AF0B73: case 0x47EE0B73: emu_DSP_GetInfo(); break; /* 0x64 */
		case 0x44AF0B91: case 0x47EE0B91: f__AB00_0B91_0014_89BD(); break; /* 0x68 */
		case 0x44AF0C08: case 0x47EE0C08: f__AB00_0C08_0013_3E08(); break; /* 0x81 */
		case 0x44AF0C3F: case 0x47EE0C3F: emu_DSP_TestPort(); break; /* 0x65 */
		case 0x44AF0C96: emu_MPU_GetInfo(); break; /* 0x64 */
		case 0x44AF0DA4: case 0x47EE0DA4: emu_DSP_Init(); break; /* 0x66 */
		case 0x44AF0F02: emu_MPU_GetUnknownSize(); break; /* 0x99 */
		case 0x44AF0F19: break; /* 0x9A */
		case 0x44AF0F24: f__AB01_0F24_0044_3584(); break; /* 0x9B */
		case 0x44AF1068: case 0x47EE1068: f__AB00_1068_0020_E6F1(); break; /* 0x7B */
		case 0x44AF1122: case 0x47EE1122: f__AB00_1122_001C_9408(); break; /* 0x7D */
		case 0x44AF118F: case 0x47EE118F: f__AB00_118F_0029_4B06(); break; /* 0x7E */
		case 0x44AF1235: case 0x47EE1235: f__AB00_1235_0013_28BA(); break; /* 0x7C */
		case 0x44AF1FA8: emu_MPU_Init(); break; /* 0x66 */
		case 0x44AF2103: f__AB01_2103_0040_93D2(); break; /* 0x68 */
		case 0x44AF2191: emu_MPU_GetDataSize(); break; /* 0x96 */
		case 0x44AF21F0: emu_MPU_SetData(); break; /* 0x97 */
		case 0x44AF2336: emu_MPU_ClearData(); break; /* 0x98 */
		case 0x44AF237A: emu_MPU_Play(); break; /* 0xAA */
		case 0x44AF240F: emu_MPU_Stop(); break; /* 0xAB */
		case 0x44AF26EB: f__AB01_26EB_0047_41F4(); break; /* 0xB1 */
		default:
			/* In case we don't know the call point yet, call the dynamic call */
			emu_last_cs = 0x2756; emu_last_ip = 0x050B; emu_last_length = 0x0003; emu_last_crc = 0x6FD4;
			emu_call();
			return csip; /* not reached */
	}

	csip.s.cs = emu_dx;
	csip.s.ip = emu_ax;
	return csip;
}

bool Driver_Music_IsPlaying()
{
	Driver *driver = &g_global->musicDriver;
	MSBuffer *buffer = &g_global->musicBuffer;

	if (driver->index == 0xFFFF) {
		if (driver->dcontent.csip == 0x0) return false;

		emu_ax = 0x0;
		emu_bx = 0x7;
		emu_pushf();

		/* Call based on memory/register values */
		emu_ip = driver->dcontent.s.ip;
		emu_push(emu_cs);
		emu_cs = driver->dcontent.s.cs;
		emu_push(0x08E2);
		switch ((emu_cs << 16) + emu_ip) {
			default:
				/* In case we don't know the call point yet, call the dynamic call */
				emu_last_cs = 0x1DD7; emu_last_ip = 0x08DF; emu_last_length = 0x0020; emu_last_crc = 0xBD30;
				emu_call();
				return false;
		}

		return emu_ax == 1;
	}

	if (buffer->index == 0xFFFF) return false;

	return MPU_IsPlaying(buffer->index) == 1;
}

bool Driver_Voice_IsPlaying()
{
	if (g_global->voiceDriver.index == 0xFFFF) return false;
	return Drivers_CallFunction(g_global->voiceDriver.index, 0x7C).s.ip == 2;
}

void Driver_Sound_Play(int16 index, int16 volume)
{
	Driver *sound = &g_global->soundDriver;
	MSBuffer *soundBuffer = &g_global->soundBuffer[g_global->soundBufferIndex];

	if (index < 0 || index >= 120) return;

	if (g_global->soundsEnabled == 0 && index > 1) return;

	if (sound->index == 0xFFFF) {
		Drivers_1DD7_1C3C(sound, index, volume);
		return;
	}

	if (soundBuffer->index != 0xFFFF) {
		emu_push(soundBuffer->index);
		emu_push(sound->index); /* unused, but needed for correct param accesses. */
		Drivers_CallFunction(sound->index, 0xAB);
		emu_sp += 4;

		emu_push(soundBuffer->index);
		emu_push(sound->index); /* unused, but needed for correct param accesses. */
		Drivers_CallFunction(sound->index, 0x98);
		emu_sp += 4;

		soundBuffer->index = 0xFFFF;
	}

	emu_push(0); emu_push(0);
	emu_push(soundBuffer->buffer.s.cs); emu_push(soundBuffer->buffer.s.ip);
	emu_push(index);
	emu_push(sound->content.s.cs); emu_push(sound->content.s.ip);
	emu_push(sound->index); /* unused, but needed for correct param accesses. */
	soundBuffer->index = Drivers_CallFunction(sound->index, 0x97).s.ip;
	emu_sp += 16;

	Drivers_1DD7_0B9C(sound, soundBuffer->index);

	emu_push(soundBuffer->index);
	emu_push(sound->index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(sound->index, 0xAA);
	emu_sp += 4;

	emu_push(0);
	emu_push(((volume & 0xFF) * 90) / 256);
	emu_push(soundBuffer->index);
	emu_push(sound->index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(sound->index, 0xB1);
	emu_sp += 8;

	g_global->soundBufferIndex = (g_global->soundBufferIndex + 1) % 4;
}

void Driver_Music_Stop()
{
	Driver *music = &g_global->musicDriver;
	MSBuffer *musicBuffer = &g_global->musicBuffer;

	if (music->index == 0xFFFF) {
		if (music->dcontent.csip == 0x0) return;
		Drivers_1DD7_1C3C(music, 0, 0);
	}

	if (musicBuffer->index == 0xFFFF) return;

	emu_push(musicBuffer->index);
	emu_push(music->index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(music->index, 0xAB);
	emu_sp += 4;

	emu_push(musicBuffer->index);
	emu_push(music->index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(music->index, 0x98);
	emu_sp += 4;

	musicBuffer->index = 0xFFFF;
}

void Driver_Sound_Stop()
{
	Driver *sound = &g_global->soundDriver;
	uint8 i;

	if (sound->index == 0xFFFF) {
		if (sound->dcontent.csip == 0x0) return;

		if (g_global->variable_6372 != 0xFFFF) {
			emu_push(g_global->variable_6372);
			emu_push(g_global->musicDriver.index); /* unused, but needed for correct param accesses. */
			Drivers_CallFunction(g_global->musicDriver.index, 0xC1);
			emu_sp += 4;

			g_global->variable_6372 = 0xFFFF;
		}

		Drivers_1DD7_1C3C(sound, 0, 0);

		return;
	}

	for (i = 0; i < 4; i++) {
		MSBuffer *soundBuffer = &g_global->soundBuffer[i];
		if (soundBuffer->index == 0xFFFF) continue;

		emu_push(soundBuffer->index);
		emu_push(sound->index); /* unused, but needed for correct param accesses. */
		Drivers_CallFunction(sound->index, 0xAB);
		emu_sp += 4;

		emu_push(soundBuffer->index);
		emu_push(sound->index); /* unused, but needed for correct param accesses. */
		Drivers_CallFunction(sound->index, 0x98);
		emu_sp += 4;

		soundBuffer->index = 0xFFFF;
	}
}

void Driver_Voice_LoadFile(char *filename, void *buffer, csip32 buffer_csip, uint32 length)
{
	assert(buffer == (void *)emu_get_memorycsip(buffer_csip));

	if (filename == NULL) return;
	if (g_global->voiceDriver.index == 0xFFFF) return;
	if (!File_Exists(filename)) return;

	if (buffer == NULL) {
		buffer_csip = File_ReadWholeFile(filename, 0x40);

		emu_push(buffer_csip.s.cs); emu_push(buffer_csip.s.ip);
		emu_push(emu_cs); emu_push(0x015F); emu_cs = 0x2649; emu_Highmem_GetSize();
		emu_sp += 4;
		length = (emu_dx << 16) + emu_ax;
	} else {
		length = File_ReadBlockFile(filename, buffer, length);
	}

	emu_push(0xFFFF);
	emu_push(buffer_csip.s.cs); emu_push(buffer_csip.s.ip);
	emu_push(g_global->voiceDriver.index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(g_global->voiceDriver.index, 0x85);
	emu_sp += 8;
}

void Driver_Voice_Play(uint8 *arg06, csip32 arg06_csip, int16 arg0A, int16 arg0C)
{
	Driver *voice = &g_global->voiceDriver;

	assert(arg06 == emu_get_memorycsip(arg06_csip));

	if (!g_global->soundsEnabled || voice->index == 0xFFFF) return;

	if (arg06 == NULL) {
		arg0A = 0x100;
	} else {
		arg0A = min(arg0A, 0xFF);
	}

	if (!Driver_Voice_IsPlaying()) g_global->variable_639A = 0xFFFF;

	if (arg0A < (int16)g_global->variable_639A) return;

	Driver_Voice_Stop();

	if (arg06 == NULL) return;

	g_global->variable_639A = arg0A;

	emu_push(arg0C / 2);
	emu_push(voice->index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(voice->index, 0x81);
	emu_sp += 4;

	emu_push(arg06_csip.s.cs); emu_push(arg06_csip.s.ip);
	emu_push(emu_cs); emu_push(0x02C8); emu_cs = 0x2649; emu_Highmem_IsInHighmem();
	emu_sp += 4;

	if (emu_ax != 0) {
		int32 loc04;

		emu_push(arg06_csip.s.cs); emu_push(arg06_csip.s.ip);
		emu_push(emu_cs); emu_push(0x030C); emu_cs = 0x2649; emu_Highmem_GetSize();
		emu_sp += 4;
		loc04 = (emu_dx << 16) + emu_ax;

		emu_push(emu_cs); emu_push(0x0319); emu_cs = 0x23E1; emu_Tools_GetFreeMemory();

		if (((emu_dx << 16) + emu_ax) < loc04) return;

		emu_push(0);
		emu_push(loc04 >> 16); emu_push(loc04 & 0xFFFF);
		emu_push(emu_cs); emu_push(0x0335); emu_cs = 0x23E1; emu_Tools_Malloc();
		emu_sp += 6;
		voice->content.s.cs = emu_dx;
		voice->content.s.ip = emu_ax;

		voice->contentMalloced = 1;

		memmove(emu_get_memorycsip(voice->content), arg06, loc04);

		arg06_csip = voice->content;
		arg06 = emu_get_memorycsip(arg06_csip);
	}

	if (arg06 == NULL) return;

	emu_push(0xFFFF);
	emu_push(arg06_csip.s.cs); emu_push(arg06_csip.s.ip);
	emu_push(voice->index); /* unused, but needed for correct param accesses. */
	Drivers_CallFunction(voice->index, 0x7B);
	emu_sp += 8;

	Drivers_CallFunction(voice->index, 0x7D);
}

void Driver_Voice_Stop()
{
	Driver *voice = &g_global->voiceDriver;

	if (Driver_Voice_IsPlaying()) Drivers_CallFunction(voice->index, 0x7E);

	if (voice->contentMalloced != 0) {
		emu_push(voice->content.s.cs); emu_push(voice->content.s.ip);
		emu_push(emu_cs); emu_push(0x01D5); emu_cs = 0x23E1; emu_Tools_Free();
		emu_sp += 4;

		voice->contentMalloced = 0;
	}

	voice->content.csip = 0x0;
}

void Driver_Sound_LoadFile(char *musicName)
{
	Driver *sound = &g_global->soundDriver;
	Driver *music = &g_global->musicDriver;

	Driver_Sound_Stop();

	if (sound->index == 0xFFFF && sound->dcontent.csip == 0x0) return;

	if (sound->content.csip == music->content.csip) {
		sound->content.csip = 0x0;
		sound->variable_1E.csip = 0x0;
		sound->filename.csip = 0x0;
		sound->contentMalloced = 0;
	} else {
		Driver_UnloadFile(sound);
	}

	if (music->filename.csip != 0x0) {
		char *filename;

		filename = Drivers_GenerateFilename(musicName, sound);

		if (strcasecmp(filename, (char *)emu_get_memorycsip(music->filename)) == 0) {
			sound->content = music->content;
			sound->variable_1E = music->variable_1E;
			sound->filename = music->filename;
			sound->contentMalloced = music->contentMalloced;

			if (sound->index == 0xFFFF) {
				emu_dx = music->dcontent.s.cs;
				emu_ax = music->dcontent.s.ip;
				emu_bx = 0x4;
				emu_pushf();
				emu_push(emu_cs); emu_push(0x06DA); emu_cs = sound->dcontent.s.cs;
				switch (sound->dcontent.csip) {
					default:
						/* In case we don't know the call point yet, call the dynamic call */
						emu_last_cs = 0x1DD7; emu_last_ip = 0x06D7; emu_last_length = 0x005F; emu_last_crc = 0x3AAB;
						emu_call();
						return;
				}
			}
			return;
		}
	}

	Driver_LoadFile(musicName, sound);
}

char *Drivers_GenerateFilename(char *name, Driver *driver)
{
	if (name == NULL || driver == NULL || driver->index == 0xFFFF || driver->dcontent.csip == 0x0) return NULL;

	strcpy(g_global->variable_984A, name);
	if (strrchr(g_global->variable_984A, '.') != NULL) *strrchr(g_global->variable_984A, '.') = '\0';
	strcat(g_global->variable_984A, ".");
	strcat(g_global->variable_984A, driver->extension);

	if (File_Exists(g_global->variable_984A)) return g_global->variable_984A;

	if (driver->index == 0xFFFF) return NULL;

	strcpy(g_global->variable_984A, name);
	if (strrchr(g_global->variable_984A, '.') != NULL) *strrchr(g_global->variable_984A, '.') = '\0';
	strcat(g_global->variable_984A, ".XMI");

	if (File_Exists(g_global->variable_984A)) return g_global->variable_984A;

	return NULL;
}

char *Drivers_GenerateFilename2(char *name, Driver *driver)
{
	if (name == NULL || driver == NULL || driver->index == 0xFFFF || driver->dcontent.csip == 0x0) return NULL;

	strcpy(g_global->variable_9858, name);
	if (strrchr(g_global->variable_9858, '.') != NULL) *strrchr(g_global->variable_9858, '.') = '\0';
	strcat(g_global->variable_9858, ".");
	strcat(g_global->variable_9858, driver->extension2);

	if (File_Exists(g_global->variable_9858)) return g_global->variable_9858;

	strcpy(g_global->variable_9858, "DEFAULT.");
	strcat(g_global->variable_9858, driver->extension2);

	if (File_Exists(g_global->variable_9858)) return g_global->variable_9858;

	return NULL;
}

void Drivers_1DD7_0B9C(Driver *driver, uint16 bufferIndex)
{
	MSVC_PACKED_BEGIN
	struct {
		/* 0000(2)   */ PACK uint16 variable_00;                /*!< ?? */
		/* 0002(4)   */ PACK uint32 position;                   /*!< ?? */
	} GCC_PACKED data;
	MSVC_PACKED_END
	assert_compile(sizeof(data) == 0x6);

	uint8 file_index = 0xFF;
	char *filename;

	if (driver == NULL || driver->index == 0xFFFF || bufferIndex == 0xFFFF) return;

	memcpy(&data, g_global->variable_639E, 6);

	while (true) {
		uint32 position = 0;
		uint16 locdi;

		emu_push(bufferIndex);
		emu_push(driver->index); /* unused, but needed for correct param accesses. */
		locdi = Drivers_CallFunction(driver->index, 0x9B).s.ip;
		emu_sp += 4;

		if (locdi == 0xFFFF) break;

		if (file_index == 0xFF) {
			filename = Drivers_GenerateFilename2((char *)emu_get_memorycsip(driver->filename), driver);

			if (filename == NULL || !File_Exists(filename)) return;

			file_index = File_Open(filename, 1);
		}

		while ((data.variable_00 >> 8) != 0xFF) {
			uint16 size;
			csip32 buffer_csip;
			uint16 *buffer;

			File_Seek(file_index, position, 0);

			File_Read(file_index, &data, 6);
			position += 6;

			if ((data.variable_00 >> 8) == 0xFF) continue;
			if (data.variable_00 != locdi) continue;

			File_Seek(file_index, data.position, 0);

			File_Read(file_index, &size, 2);

			emu_push(0);
			emu_push(0); emu_push(size);
			emu_push(emu_cs); emu_push(0xCC9); emu_cs = 0x23E1; emu_Tools_Malloc();
			emu_sp += 6;
			buffer_csip.s.cs = emu_dx;
			buffer_csip.s.ip = emu_ax;
			buffer = (uint16 *)emu_get_memorycsip(buffer_csip);

			buffer[0] = size;
			size -= 2;

			if (File_Read(file_index, buffer + 2, size) == size) {
				emu_push(buffer_csip.s.cs); emu_push(buffer_csip.s.ip);
				emu_push(data.variable_00 & 0xFF); emu_push(data.variable_00 >> 8);
				emu_push(driver->index); /* unused, but needed for correct param accesses. */
				Drivers_CallFunction(driver->index, 0x9C);
				emu_sp += 10;
			}

			emu_push(buffer_csip.s.cs); emu_push(buffer_csip.s.ip);
			emu_push(emu_cs); emu_push(0x0D36); emu_cs = 0x23E1; emu_Tools_Free();
			emu_sp += 4;
			break;
		}

		if ((data.variable_00 >> 8) == 0xFF) break;
	}

	if (file_index != 0xFF) File_Close(file_index);
}

static void Drivers_Music_Uninit()
{
	Driver *music = &g_global->musicDriver;

	if (music->index != 0xFFFF) {
		MSBuffer *buffer = &g_global->musicBuffer;

		if (buffer->index != 0xFFFF) {
			emu_push(buffer->index);
			emu_push(music->index); /* unused, but needed for correct param accesses. */
			Drivers_CallFunction(music->index, 0xAB);
			emu_sp += 4;

			emu_push(buffer->index);
			emu_push(music->index); /* unused, but needed for correct param accesses. */
			Drivers_CallFunction(music->index, 0x98);
			emu_sp += 4;

			buffer->index = 0xFFFF;
		}

		emu_push(buffer->buffer.s.cs); emu_push(buffer->buffer.s.ip);
		emu_push(emu_cs); emu_push(0x106E); emu_cs = 0x23E1; emu_Tools_Free();
		emu_sp += 4;
		buffer->buffer.csip = 0x0;
	}

	if (music->dcontent.csip == g_global->soundDriver.dcontent.csip) {
		music->dcontent.csip    = 0x0;
		music->variable_12.csip = 0x0;
		music->dfilename.csip   = 0x0;
		music->customTimer      = 0xFFFF;
	} else {
		Drivers_Uninit(music);
	}
}

static void Drivers_Sound_Uninit()
{
	Driver *sound = &g_global->soundDriver;

	if (sound->index != 0xFFFF) {
		uint8 i;

		for (i = 0; i < 4; i++) {
			MSBuffer *buffer = &g_global->soundBuffer[i];

			if (buffer->index != 0xFFFF) {
				emu_push(buffer->index);
				emu_push(sound->index); /* unused, but needed for correct param accesses. */
				Drivers_CallFunction(sound->index, 0xAB);
				emu_sp += 4;

				emu_push(buffer->index);
				emu_push(sound->index); /* unused, but needed for correct param accesses. */
				Drivers_CallFunction(sound->index, 0x98);
				emu_sp += 4;

				buffer->index = 0xFFFF;
			}

			emu_push(buffer->buffer.s.cs); emu_push(buffer->buffer.s.ip);
			emu_push(emu_cs); emu_push(0x1260); emu_cs = 0x23E1; emu_Tools_Free();
			emu_sp += 4;
			buffer->buffer.csip = 0x0;
		}
	}

	if (sound->dcontent.csip == g_global->musicDriver.dcontent.csip) {
		sound->dcontent.csip    = 0x0;
		sound->variable_12.csip = 0x0;
		sound->dfilename.csip   = 0x0;
		sound->customTimer      = 0xFFFF;
	} else {
		Drivers_Uninit(sound);
	}
}

static void Drivers_Voice_Uninit()
{
	Drivers_Uninit(&g_global->voiceDriver);
}

void Drivers_All_Uninit()
{
	Drivers_CustomTimer_RemoveHandler(g_global->variable_639C);

	g_global->variable_639C = 0xFFFF;

	Drivers_Music_Uninit();
	Drivers_Sound_Uninit();
	Drivers_Voice_Uninit();
}

static void Drivers_1DD7_0D77(char *musicName, Driver *driver)
{
	MSVC_PACKED_BEGIN
	struct {
		/* 0000(1)   */ PACK uint8 variable_00;                 /*!< ?? */
		/* 0001(1)   */ PACK uint8 variable_01;                 /*!< ?? */
		/* 0002(4)   */ PACK uint32 position;                   /*!< ?? */
	} GCC_PACKED data;
	MSVC_PACKED_END
	assert_compile(sizeof(data) == 0x6);

	char *filename;
	uint8 fileIndex;
	uint32 position = 0;

	if (musicName == NULL || driver->index == 0xFFFF) return;

	memcpy(&data, g_global->variable_63A4, 6);

	filename = Drivers_GenerateFilename2(musicName, driver);

	if (filename == NULL) return;

	fileIndex = File_Open(filename, 1);

	while (data.variable_01 != 0xFF) {
		uint16 size;
		csip32 buffer_csip;
		uint16 *buffer;

		File_Seek(fileIndex, position, 0);

		File_Read(fileIndex, &data, 6);
		position += 6;

		if (data.variable_01 == 0xFF) continue;

		File_Seek(fileIndex, data.position, 0);

		File_Read(fileIndex, &size, 2);

		emu_push(0);
		emu_push(0); emu_push(size);
		emu_push(emu_cs); emu_push(0x0E5C); emu_cs = 0x23E1; emu_Tools_Malloc();
		emu_sp += 6;
		buffer_csip.s.cs = emu_dx;
		buffer_csip.s.ip = emu_ax;
		buffer = (uint16 *)emu_get_memorycsip(buffer_csip);

		buffer[0] = size;
		size -= 2;

		if (File_Read(fileIndex, buffer + 2, size) == size) {
				emu_push(buffer_csip.s.cs); emu_push(buffer_csip.s.ip);
				emu_push(data.variable_00);
				emu_push(data.variable_01);
				emu_push(driver->index); /* unused, but needed for correct param accesses. */
				Drivers_CallFunction(driver->index, 0x9C);
				emu_sp += 10;
		} else {
			data.variable_01 = 0xFF;
		}

		emu_push(buffer_csip.s.cs); emu_push(buffer_csip.s.ip);
		emu_push(emu_cs); emu_push(0x0EC5); emu_cs = 0x23E1; emu_Tools_Free();
		emu_sp += 4;
	}

	File_Close(fileIndex);
}

void Driver_LoadFile(char *musicName, Driver *driver)
{
	char *filename;
	uint8 fileIndex;
	int32 size;

	filename = Drivers_GenerateFilename(musicName, driver);

	if (filename == NULL) return;

	Driver_UnloadFile(driver);

	emu_push(0);
	emu_push(0); emu_push(strlen(filename) + 1);
	emu_push(emu_cs); emu_push(0x19A0); emu_cs = 0x23E1; emu_Tools_Malloc();
	emu_sp += 6;
	driver->filename.s.cs = emu_dx;
	driver->filename.s.ip = emu_ax;

	strcpy((char *)emu_get_memorycsip(driver->filename), filename);

	fileIndex = File_Open(filename, 1);

	size = File_GetSize(fileIndex);

	emu_push(emu_cs); emu_push(0x1A34); emu_cs = 0x23E1; emu_Tools_GetFreeMemory();

	if ((int32)((emu_dx << 16) + emu_ax) < (size + 16)) {
		File_Close(fileIndex);

		emu_push(driver->filename.s.cs); emu_push(driver->filename.s.ip);
		emu_push(emu_cs); emu_push(0x1A19); emu_cs = 0x23E1; emu_Tools_Free();
		emu_sp += 4;

		driver->filename.csip = 0x0;
		return;
	}

	if (driver->index == 0xFFFF) {
		emu_push(0);
		emu_push(0); emu_push(120);
		emu_push(emu_cs); emu_push(0x1A94); emu_cs = 0x23E1; emu_Tools_Malloc();
		emu_sp += 6;
		driver->variable_1E.s.cs = emu_dx;
		driver->variable_1E.s.ip = emu_ax;

		driver->contentMalloced = 1;

		size -= 120;

		File_Read(fileIndex, (void *)emu_get_memorycsip(driver->variable_1E), 120);
	}

	emu_push(0x20);
	emu_push(size >> 16); emu_push(size & 0xFFFF);
	emu_push(emu_cs); emu_push(0x1B14); emu_cs = 0x23E1; emu_Tools_Malloc();
	emu_sp += 6;
	driver->content.s.cs = emu_dx;
	driver->content.s.ip = emu_ax;

	driver->contentMalloced = 1;

	File_Read(fileIndex, (void *)emu_get_memorycsip(driver->content), size);

	File_Close(fileIndex);

	if (driver->index == 0xFFFF) {
		emu_dx = driver->content.s.cs;
		emu_ax = driver->content.s.ip;
		emu_bx = 0x4;
		emu_pushf();

		/* Call based on memory/register values */
		emu_ip = driver->dcontent.s.ip;
		emu_push(emu_cs);
		emu_cs = driver->dcontent.s.cs;
		emu_push(0x1B91);
		switch ((emu_cs << 16) + emu_ip) {
			default:
				/* In case we don't know the call point yet, call the dynamic call */
				emu_last_cs = 0x1DD7; emu_last_ip = 0x1B8E; emu_last_length = 0x003A; emu_last_crc = 0x432B;
				emu_call();
				return;
		}
	} else {
		Drivers_1DD7_0D77(musicName, driver);
	}
}

void Driver_UnloadFile(Driver *driver)
{
	if (driver->content.csip != 0x0 && driver->contentMalloced != 0) {
		emu_push(driver->content.s.cs); emu_push(driver->content.s.ip);
		emu_push(emu_cs); emu_push(0x1BDE); emu_cs = 0x23E1; emu_Tools_Free();
		emu_sp += 4;

		emu_push(driver->variable_1E.s.cs); emu_push(driver->variable_1E.s.ip);
		emu_push(emu_cs); emu_push(0x1BF0); emu_cs = 0x23E1; emu_Tools_Free();
		emu_sp += 4;
	}

	emu_push(driver->filename.s.cs); emu_push(driver->filename.s.ip);
	emu_push(emu_cs); emu_push(0x1C02); emu_cs = 0x23E1; emu_Tools_Free();
	emu_sp += 4;

	driver->contentMalloced  = 0;
	driver->filename.csip    = 0x0;
	driver->content.csip     = 0x0;
	driver->variable_1E.csip = 0x0;
}

void Driver_Music_FadeOut()
{
	Driver *music = &g_global->musicDriver;
	MSBuffer *musicBuffer = &g_global->musicBuffer;

	if (music->index != 0xFFFF) {
		if (musicBuffer->index != 0xFFFF) {
			emu_push(0x7D0);
			emu_push(0);
			emu_push(musicBuffer->index);
			emu_push(music->index); /* unused, but needed for correct param accesses. */
			Drivers_CallFunction(music->index, 0xB1);
			emu_sp += 8;
		}
		return;
	}

	if (music->dcontent.csip != 0x0) Drivers_1DD7_1C3C(music, 1, 0);
}

void Drivers_1DD7_1C3C(Driver *driver, int16 index, uint16 volume)
{
	if (index == -1 || driver->dcontent.csip == 0x0 || driver->content.csip == 0x0) return;

	index = (int8)emu_get_memorycsip(driver->variable_1E)[index];
	if (index == -1) return;

	if (strncmp((char *)emu_get_memorycsip(driver->dfilename), "ALFX", 4) == 0) {
		index &= 0xFF;

		while (true) {
			emu_ax = 0;
			emu_bx = 0x10;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1CC2); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1CBF; emu_last_length = 0x0009; emu_last_crc = 0x2391;
					emu_call();
					return;
			}
			if ((emu_ax & 0x8) == 0) break;
		}

		if (g_global->variable_6516 != 0xFFFF) {
			emu_cx = g_global->variable_9868;
			emu_dx = 1;
			emu_ax = g_global->variable_6516;
			emu_bx = 0xA;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1CE4); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1CE1; emu_last_length = 0x0022; emu_last_crc = 0xF69B;
					emu_call();
					return;
			}

			emu_cx = g_global->variable_9866;
			emu_dx = 3;
			emu_ax = g_global->variable_6516;
			emu_bx = 0xA;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1CF5); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1CF2; emu_last_length = 0x0022; emu_last_crc = 0xF69B;
					emu_call();
					return;
			}
			g_global->variable_6516 = 0xFFFF;
		}

		emu_dx = 0;
		emu_ax = index;
		emu_bx = 0x9;
		emu_pushf();

		/* Call based on memory/register values */
		emu_push(emu_cs); emu_push(0x1D08); emu_cs = driver->dcontent.s.cs;
		switch (driver->dcontent.csip) {
			default:
				/* In case we don't know the call point yet, call the dynamic call */
				emu_last_cs = 0x1DD7; emu_last_ip = 0x1D05; emu_last_length = 0x000D; emu_last_crc = 0xFF6E;
				emu_call();
				return;
		}

		g_global->variable_6516 = 0xFFFF;

		if (volume != 0xFFFF && emu_ax != 9) {
			g_global->variable_6516 = index;

			emu_dx = 1;
			emu_ax = index;
			emu_bx = 0x9;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1D36); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1D33; emu_last_length = 0x000D; emu_last_crc = 0xFF6E;
					emu_call();
					return;
			}
			g_global->variable_9866 = emu_ax;

			emu_cx = 63 - ((63 - g_global->variable_9866) * volume >> 8);
			emu_dx = 3;
			emu_ax = index;
			emu_bx = 0xA;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1D6C); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1D69; emu_last_length = 0x000D; emu_last_crc = 0xFF6E;
					emu_call();
					return;
			}

			emu_cx = (g_global->variable_9868 * volume) >> 8;
			emu_dx = 1;
			emu_ax = 0xA;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1D85); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1D82; emu_last_length = 0x000D; emu_last_crc = 0xFF6E;
					emu_call();
					return;
			}
		}

		emu_ax = index;
		emu_bx = 0x6;
		emu_pushf();

		/* Call based on memory/register values */
		emu_push(emu_cs); emu_push(0x1D8F); emu_cs = driver->dcontent.s.cs;
		switch (driver->dcontent.csip) {
			default:
				/* In case we don't know the call point yet, call the dynamic call */
				emu_last_cs = 0x1DD7; emu_last_ip = 0x1D8C; emu_last_length = 0x000A; emu_last_crc = 0x136B;
				emu_call();
				return;
		}

		if (index == 0) {
			Tools_Sleep(5);
		}

		return;
	}

	if (g_global->variable_6328 == 1) {
		if (index < 0) {
			emu_ax = 4;
			emu_bx = 0x11;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1DB8); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1DB5; emu_last_length = 0x000A; emu_last_crc = 0x136B;
					emu_call();
					return;
			}
		} else {
			emu_ax = 4;
			emu_bx = 0x10;
			emu_pushf();

			/* Call based on memory/register values */
			emu_push(emu_cs); emu_push(0x1DC4); emu_cs = driver->dcontent.s.cs;
			switch (driver->dcontent.csip) {
				default:
					/* In case we don't know the call point yet, call the dynamic call */
					emu_last_cs = 0x1DD7; emu_last_ip = 0x1DC1; emu_last_length = 0x000A; emu_last_crc = 0x136B;
					emu_call();
					return;
			}
		}
	}

	emu_ax = abs(index);
	emu_bx = 0x10;
	emu_pushf();

	/* Call based on memory/register values */
	emu_push(emu_cs); emu_push(0x1DE1); emu_cs = driver->dcontent.s.cs;
	switch (driver->dcontent.csip) {
		default:
			/* In case we don't know the call point yet, call the dynamic call */
			emu_last_cs = 0x1DD7; emu_last_ip = 0x1DDE; emu_last_length = 0x000A; emu_last_crc = 0x136B;
			emu_call();
			return;
	}
}
