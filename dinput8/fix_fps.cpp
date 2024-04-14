// ----------------------------------------------------------------------------
// fix_fps.cpp
//
// Copyright (c) 2021-2024 Vaana
// ----------------------------------------------------------------------------

#include "fix_fps.h"

#include "patching.h"
#include "shared.h"
#include <cassert>

DWORD sigFramerateDividerConstructor[] =
{
		0x89, 0x5C, 0x24, 0x18,
		0x64, 0x8B, 0x15, 0x2C, 0x00, 0x00, 0x00,
		0xF3, 0x0F, 0x10, 0x05, MASK, MASK, MASK, MASK,
HERE,	0xC7, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x8B, 0x02,
		0xC7, 0x06, MASK, MASK, MASK, MASK
};

DWORD sigAltFramerateDividerConstructor[] =
{
		0x89, 0x5C, 0x24, 0x18,
		0xD9, 0xE8,
		0x64, 0x8B, 0x15, 0x2C, 0x00, 0x00, 0x00,
		0xD9, 0x56, 0x0C,
HERE,	0xC7, 0x00, 0x02, 0x00, 0x00, 0x00,
		0xD9, 0x5E, 0x10,
		0x8B, 0x02,
		0xC7, 0x06, MASK, MASK, MASK, MASK
};

DWORD sigFramerateDividerGameplay[] =
{
		0xFE, 0x0D, MASK, MASK, MASK, MASK,
		0xFF, 0xD2,
		0x8B, 0x0D, MASK, MASK, MASK, MASK,
HERE,	0xC7, 0x46, 0x04, 0x02, 0x00, 0x00, 0x00,
		0x8B, 0x01,
		0x8B, 0x50, 0x40
};

DWORD sigWaitAndHook[] =
{
		0x0F, 0x57, 0xC0,
		0x0F, 0x2F, 0x44, 0x24, 0x0C,
HERE,	0x76, MASK,
		0xD9, 0x44, 0x24, 0x0C,
		0xDC, 0x0D, MASK, MASK, MASK, MASK,
		0xD9, 0x7C, 0x24, 0x0C
};

DWORD sigBraking[] =
{
		0x0F, 0xC6, 0xC0, 0x00,
		0x0F, 0x59, 0xC1,
		0xE8, MASK, MASK, MASK, MASK,
HERE,	0xF3, 0x0F, 0x10, 0x45, 0x08,
		0x0F, 0x28, 0x4C, 0x24, 0x40,
		0x0F, 0x5A, 0xC0
};

DWORD sigAltBraking[] =
{
		0x0F, 0xC6, 0xC0, 0x00,
		0x0F, 0x59, 0xC1,
		0xE8, MASK, MASK, MASK, MASK,
HERE,	0xD9, 0x45, 0x08,
		0xDC, 0x0D, MASK, MASK, MASK, MASK,
		0x8D, 0x94, 0x24, 0xA0, 0x00, 0x00, 0x00
};

void RegisterPatch_Framerate()
{
	Patch patch;

	REGISTER_ENGINE_MASK(patch);
	REGISTER_MASK_ALTERNATE(patch, sigFramerateDividerConstructor, sigAltFramerateDividerConstructor);
	REGISTER_MASK(patch, sigFramerateDividerGameplay);
	REGISTER_MASK(patch, sigWaitAndHook);
	REGISTER_MASK_ALTERNATE(patch, sigBraking, sigAltBraking);

	ua_tcscpy_s(patch.name, 50, TEXT("Framerate Unlock"));
	patch.func = ApplyPatch_Framerate;

	RegisterPatch(patch);
}

static I3DEngine** ppEngine;
static LARGE_INTEGER lastTime, timeFrequency;

static float frm = 0.033333f;

static DWORD fixedFrametime = 0x3D088889; // 0.03333333507

bool ApplyPatch_Framerate(Patch* patch)
{
	assert(patch->numSignatures == 5);
	void* enginePtr						= patch->signatures[0].foundPtr;
	void* framerateDividerConstructor	= (BYTE*)patch->signatures[1].foundPtr + 2;
	void* framerateDividerGameplay		= (BYTE*)patch->signatures[2].foundPtr + 3;
	void* waitAndHook					= patch->signatures[3].foundPtr;
	void* braking						= patch->signatures[4].foundPtr;
	bool isBrakingAlt					= patch->signatures[4].isAlternate;

	// Find the engine pointer
	if (!MemRead(enginePtr, &ppEngine, sizeof(ppEngine)))	return false;

	// Remove framerate divider
	static unsigned int newFramerateDivider = 1;
	if (!MemWrite(framerateDividerConstructor, &newFramerateDivider, sizeof(newFramerateDivider)))	return false;
	if (!MemWrite(framerateDividerGameplay, &newFramerateDivider, sizeof(newFramerateDivider)))		return false;

	// Remove waiting logic and add hook
	jmp jmp;
	if (!MemRead(waitAndHook, &jmp, sizeof(jmp)))		return false;
	if (!MemWriteHookCall(waitAndHook, &Hook_Frame))	return false;
	jmp.opcode = 0xEB;
	jmp.offset -= 5;
	if (!MemWrite((BYTE*)waitAndHook + 5, &jmp, sizeof(jmp)))	return false;

	// Fix braking force
	if (!isBrakingAlt)
	{
		BYTE brakeHook[] =
		{
			0xF3, 0x0F, 0x10, 0x05, MASK, MASK, MASK, MASK,	// movss xmm0, dword ptr [$fixedFrametime]
			0xE9, MASK, MASK, MASK, MASK					// jmp $hook
		};

		byte* pBrakeHook = (byte*)ExecCopy(brakeHook, sizeof(brakeHook));

		DWORD* i1 = (DWORD*)&pBrakeHook[4];
		DWORD* i2 = (DWORD*)&pBrakeHook[9];

		*i1 = (DWORD)&fixedFrametime;
		*i2 = (DWORD)braking - (DWORD)i2 + 1;

		if (!MemWriteHookJmp(braking, pBrakeHook))	return false;
	}
	else
	{
		BYTE brakeHook[] =
		{
			0xD9, 0x05, MASK, MASK, MASK, MASK,	// fld dword ptr [$fixedFrametime]
			0xE9, MASK, MASK, MASK, MASK		// jmp $hook
		};


		assert(false);
	}

	// Prepare required variables
	if (!QueryPerformanceCounter(&lastTime))		{ HandleError(TEXT("Patching failed!"), TEXT("Could not query performance counter.")); return false; }
	if (!QueryPerformanceFrequency(&timeFrequency)) { HandleError(TEXT("Patching failed!"), TEXT("Could not query performance frequency.")); return false; }

	return true;
}

void Hook_Frame()
{
	I3DEngine* engine = *ppEngine;
	assert(engine);

	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);

	LONGLONG quadDiff = currTime.QuadPart - lastTime.QuadPart;

	if (quadDiff > 0)
	{
		double fps = timeFrequency.QuadPart / (double)quadDiff;

		if (engine)
		{
			engine->framerate = max(fps, 25);

			frm = 1 / engine->framerate;
			//MemWrite((void*)0x10D7230, &frm, sizeof(frm));
		}

	}

	lastTime = currTime;
}
