// ----------------------------------------------------------------------------
// fix_fps.cpp
//
// Copyright (c) 2021-2024 Vaana
// ----------------------------------------------------------------------------

#include "fix_fps.h"

#include "patching.h"
#include "shared.h"
#include <cassert>

DWORD sigFramerateDivisorConstructor[] =
{
		0x89, 0x5C, 0x24, 0x18,
		0x64, 0x8B, 0x15, 0x2C, 0x00, 0x00, 0x00,
		0xF3, 0x0F, 0x10, 0x05, MASK, MASK, MASK, MASK,
HERE,	0xC7, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x8B, 0x02,
		0xC7, 0x06, MASK, MASK, MASK, MASK
};

DWORD sigAltFramerateDivisorConstructor[] =
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

DWORD sigFramerateDivisorGameplay[] =
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

DWORD sigPencil[] =
{
		0x8B, 0x42, 0x10,
		0x57,
		0x51,
		0xD9, 0x1C, 0x24,
		0xFF, 0xD0,
HERE,	0x8B, 0x4D, 0x04,
		0x8B, 0x41, 0x10,
		0x2B, 0xC6
};

DWORD sigNotebookClueConstructor[] =
{
		0x56,
		0xE8, MASK, MASK, MASK, MASK,
		0xB8, 0xFF, 0xFF, 0x00, 0x00,
		0xC7, 0x06, HERE, MASK, MASK, MASK, MASK,
		0xC7, 0x46, 0x24, MASK, MASK, MASK, MASK,
		0x66, 0x89, 0x86, 0xB0, 0x00, 0x00, 0x00,
		0x64, 0x8B, 0x0D, 0x2C, 0x00, 0x00, 0x00
};

DWORD sigPairedAnimStageConstructor[] =
{
		0x89, 0x48, 0x08,
		0xC7, 0x40, 0x0C, MASK, MASK, MASK, MASK,
		0x89, 0x48, 0x10,
		0xC7, 0x00, HERE, MASK, MASK, MASK, MASK,
		0xC7, 0x40, 0x0C, MASK, MASK, MASK, MASK,
		0x89, 0x48, 0x14,
		0x89, 0x48, 0x18
};

DWORD sigBirds[] =
{
		0xF3, 0x0F, 0x11, 0x83, MASK, MASK, MASK, MASK,
		0x77, MASK,
		0x0F, 0x28, 0xC1,
HERE,	0xF3, 0x0F, 0x10, 0x0D, MASK, MASK, MASK, MASK,
		0x0F, 0x2F, 0xC8,
		0x77, MASK,
		0x0F, 0x28, 0xC1,
		0xF3, 0x0F, 0x10, 0x8B, MASK, MASK, MASK, MASK,
		0xF3, 0x0F, 0x10, 0x93, MASK, MASK, MASK, MASK
};

static int engineDestructorIndex = -1;
static int framerateDivisorConstructorIndex = -1;
static int framerateDivisorGameplayIndex = -1;
static int waitAndHookIndex = -1;
static int brakingIndex = -1;
static int pencilIndex = -1;
static int notebookClueConstructorIndex = -1;
static int pairedAnimStageConstructorIndex = -1;
static int birdsIndex = -1;

void RegisterPatch_Framerate()
{
	Patch patch;

	engineDestructorIndex				= patch.AddSignature(SIGARG(sigEngineDestructor));
	framerateDivisorConstructorIndex	= patch.AddSignatureWithAlt(SIGARG(sigFramerateDivisorConstructor), SIGARG(sigAltFramerateDivisorConstructor));
	framerateDivisorGameplayIndex		= patch.AddSignature(SIGARG(sigFramerateDivisorGameplay));
	waitAndHookIndex					= patch.AddSignature(SIGARG(sigWaitAndHook));
	brakingIndex						= patch.AddSignatureWithAlt(SIGARG(sigBraking), SIGARG(sigAltBraking));
	pencilIndex							= patch.AddSignature(SIGARG(sigPencil));
	notebookClueConstructorIndex		= patch.AddSignature(SIGARG(sigNotebookClueConstructor));
	pairedAnimStageConstructorIndex		= patch.AddSignature(SIGARG(sigPairedAnimStageConstructor));
	birdsIndex							= patch.AddSignature(SIGARG(sigBirds));

	patch.SetName(TEXT("Framerate Unlock"));
	patch.func = ApplyPatch_Framerate;

	RegisterPatch(patch);
}

static I3DEngine** ppEngine;					// Pointer to engine object
static LARGE_INTEGER lastTime, timeFrequency;	// Time measurement variables

static DWORD fixedFrametime = 0x3D088889;		// Default frametime => 0.03333333507

static void* pairedAnimStageVft;				// PairedAnimStage::__vftptr
static void* notebookClueVft;					// NotebookClue::__vftptr

static float birdMaxSpeed;
static float* defaultBirdMaxSpeed;

bool ApplyPatch_Framerate(Patch* patch)
{
	assert(patch->numSignatureIndices == 9);
	void* enginePtr						= patch->GetSignature(engineDestructorIndex);
	void* framerateDivisorConstructor	= (BYTE*)patch->GetSignature(framerateDivisorConstructorIndex) + 2;
	void* framerateDivisorGameplay		= (BYTE*)patch->GetSignature(framerateDivisorGameplayIndex) + 3;
	void* waitAndHook					= patch->GetSignature(waitAndHookIndex);
	bool isBrakingAlt					= false;
	void* braking						= patch->GetSignature(brakingIndex, &isBrakingAlt);
	void* pencil						= (BYTE*)patch->GetSignature(pencilIndex);
	void* notebookClueConstructor		= (BYTE*)patch->GetSignature(notebookClueConstructorIndex);
	void* pairedAnimStageConstructor	= (BYTE*)patch->GetSignature(pairedAnimStageConstructorIndex);
	void* birds							= (BYTE*)patch->GetSignature(birdsIndex) + 4;

	// ========================================================================
	// 
	// Framerate cap removal
	// 
	// ========================================================================

	// Find the engine object pointer
	if (!MemRead(enginePtr, &ppEngine, sizeof(ppEngine)))	return false;

	//
	// By default, the game has a target framerate of 59.97. However there is
	// a divisor value, which is used to determine the final framerate.
	// In menus, this value is set to 1 (60/1=60), but during gameplay, this
	// is set to 2 (60/2=30).
	// 
	// Here we patch out this divisor in gameplay, since it makes the rest of
	// the patch a lot simpler, and prevents potential bugs.
	//

	static DWORD newFramerateDivisor = 1;
	if (!MemWrite(framerateDivisorConstructor, &newFramerateDivisor, sizeof(newFramerateDivisor)))	return false;
	if (!MemWrite(framerateDivisorGameplay, &newFramerateDivisor, sizeof(newFramerateDivisor)))		return false;

	//
	// In order to fully "uncap" the framerate, we must first allow the game
	// to run at full speed. Here we patch out the waiting logic and hook our
	// own function, which then increases or decreases the speed of the
	// simulation based on how quickly the game runs.
	//

	jmp jmp;
	if (!MemRead(waitAndHook, &jmp, sizeof(jmp)))				return false;

	if (!MemWriteHookCall(waitAndHook, &Hook_Frame))			return false; // replace conditional jump with hook
	jmp.opcode = 0xEB;
	jmp.offset -= 5;
	if (!MemWrite((BYTE*)waitAndHook + 5, &jmp, sizeof(jmp)))	return false; // append hook with jump to end of branch

	// ========================================================================
	// 
	// FPS-related fixes
	// 
	// ========================================================================

	//
	// CAR BRAKING
	// 
	// When driving cars, the game creates forces and impulses based on the
	// user's inputs, which are then passed down to the physics system (Havok)
	// that actually performs the physics simulation. Havok will then correctly
	// scale these forces according to the FPS.
	// 
	// Unfortunately, the force which is used for braking is already scaled
	// by the engine once before it is passed to Havok, meaning it gets scaled twice.
	// This causes the braking to become weaker and weaker, the smaller
	// the frametime (high FPS)
	// 
	// forcePerFrameVector = forceVector * frametime * frametime;
	// 
	// The fix is to simply hardcode one of these multiplications to a fixed 1/30 value,
	// so that the final scaling is correct on FPS above 30.
	// 
	// forcePerFrameVector = forceVector * 0.0333333 * frametime;
	//

	if (!isBrakingAlt)
	{
		BYTE brakeHook[] =
		{
			0xF3, 0x0F, 0x10, 0x05, MASK, MASK, MASK, MASK,	// movss xmm0, dword ptr [$fixedFrametime]
			0xE9, MASK, MASK, MASK, MASK					// jmp $hook
		};

		BYTE* pBrakeHook = (BYTE*)ExecCopy(brakeHook, sizeof(brakeHook));
		assert(pBrakeHook);

		DWORD* a1 = (DWORD*)&pBrakeHook[4];
		DWORD* a2 = (DWORD*)&pBrakeHook[9];

		*a1 = (DWORD)&fixedFrametime;
		*a2 = (DWORD)braking - (DWORD)a2 + 1;

		if (!MemWriteHookJmp(braking, pBrakeHook))	return false;
	}
	else
	{
		BYTE brakeHook[] =
		{
			0xD9, 0x05, MASK, MASK, MASK, MASK,	// fld dword ptr [$fixedFrametime]
			0xDC, 0x0D, MASK, MASK, MASK, MASK,	// fmul dword ptr [$30]
			0xE9, MASK, MASK, MASK, MASK		// jmp $hook
		};

		BYTE* pBrakeHook = (BYTE*)ExecCopy(brakeHook, sizeof(brakeHook));
		assert(pBrakeHook);

		BYTE fmul[6];
		if (!MemRead((BYTE*)braking + 6, &fmul, sizeof(fmul)))	return false;
		memcpy(&brakeHook[6], fmul, sizeof(fmul));

		DWORD* a1 = (DWORD*)&pBrakeHook[2];
		DWORD* a2 = (DWORD*)&pBrakeHook[13];

		*a1 = (DWORD)&fixedFrametime;
		*a2 = (DWORD)braking - (DWORD)a2 + 5;

		if (!MemWriteHookJmp(braking, pBrakeHook))	return false;
		if (!MemWriteNop((BYTE*)braking + 5, 4))	return false;
	}

	//
	// PENCIL CLUE
	//
	// When the player picks up the pencil in the case "The Set Up", there
	// are two things which have to happen:
	//   Firstly, a PickupEventHandler should be called.
	//   Secondly, the InspectionSystem should begin the interaction with
	//   the object.
	// 
	// For some odd reason, above 50 FPS these things happen in the
	// incorrect order and the InspectionSystem panics and cancels,
	// since it does not yet have the correct values set by the pickup event.
	// 
	// The best way I have figured out to fix this is to manually delay
	// the InspectionSystem entering the next stage via a hook and
	// cock-blocking the progression until the pickup event happens.
	// 
	// It's probably related to some weird animation event floating point
	// timing fuckery, and I haven't got the slightest fucking clue how to fix it!
	//

	pairedAnimStageVft	= (void*)(*(DWORD*)pairedAnimStageConstructor);
	notebookClueVft		= (void*)(*(DWORD*)notebookClueConstructor);

	BYTE pencilHook[] =
	{
		0x89, 0xE9,							// mov ecx, ebp
		0x51,								// push ecx
		0xE8, MASK, MASK, MASK, MASK,		// call $Hook_Pencil
		0x8B, 0x4D, 0x04,					// mov ecx,[ebp + 04]
		0x8B, 0x41, 0x10,					// mov eax,[ecx + 10]
		0xE9, MASK, MASK, MASK, MASK		// jmp $hook
	};

	BYTE* pPencilHook = (BYTE*)ExecCopy(pencilHook, sizeof(pencilHook));

	DWORD* a1 = (DWORD*)&pPencilHook[4];
	DWORD* a2 = (DWORD*)&pPencilHook[15];

	*a1 = (DWORD)&Hook_Pencil - (DWORD)a1 - 4;
	*a2 = (DWORD)pencil - (DWORD)a2 + 1; // FIXME: is this correct?

	if (!MemWriteHookJmp(pencil, pPencilHook))	return false;
	if (!MemWriteNop((BYTE*)pencil + 5, 1))		return false;

	//
	// PIGEON TAKEOFF
	//
	// Birds have a maximum top speed when taking off. However, this speed
	// is specified per frame (0.01 units/frame) and thus it becomes too
	// large on high FPS.
	// 
	// 0.01 * 30  = 0.3 u/s
	// 0.01 * 160 = 1.6 u/s
	//
	// The fix adjusts the maximum top speed according to the framerate.
	//

	if (!MemRead(birds, &defaultBirdMaxSpeed, sizeof(defaultBirdMaxSpeed)))	return false;
	birdMaxSpeed = *defaultBirdMaxSpeed;

	float* pBirdMaxSpeed = &birdMaxSpeed;
	if (!MemWrite(birds, &pBirdMaxSpeed, sizeof(pBirdMaxSpeed)))			return false;

	// Prepare required variables
	if (!QueryPerformanceCounter(&lastTime))		{ HandleError(TEXT("Patching failed!"), TEXT("Could not query performance counter.")); return false; }
	if (!QueryPerformanceFrequency(&timeFrequency)) { HandleError(TEXT("Patching failed!"), TEXT("Could not query performance frequency.")); return false; }

	return true;
}

double minFrameTime = 0.0;

void __stdcall Hook_Frame()
{
	I3DEngine* engine = *ppEngine;
	assert(engine);

	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);

	LONGLONG timeDiff = currTime.QuadPart - lastTime.QuadPart;

	// Frame limiter
	LONGLONG minCounter = timeFrequency.QuadPart * minFrameTime;

	while (timeDiff < minCounter)
	{
		QueryPerformanceCounter(&currTime);
		timeDiff = currTime.QuadPart - lastTime.QuadPart;
	}

	// Adjust game speed
	LONGLONG maxCounter = timeFrequency.QuadPart * 0.04; // 25 FPS (safe limit)
	timeDiff = min(maxCounter, timeDiff);

	double fps = timeFrequency.QuadPart / (double)timeDiff;
	double frameTime = timeDiff / (double)timeFrequency.QuadPart;

	float multiplier = frameTime / *(float*)&fixedFrametime;

	if (engine)
	{
		engine->framerate = fps;
	}

	// Change bird speed limit
	birdMaxSpeed = multiplier * *defaultBirdMaxSpeed;

	lastTime = currTime;
}

void __stdcall Hook_Pencil(InspectionSystem* inspection)
{	
	if (!inspection || !inspection->tb.object || !inspection->tb.object->clue || !inspection->stage) // Safe-guard
	{
		return;
	}

	if (inspection->tb.object->clue->__vftptr == notebookClueVft) // must be of type NotebookClue
	{
		if (inspection->stage->__vftptr != pairedAnimStageVft) // must be of type PairedAnimStage
		{
			return; 
		}

		if (inspection->stage->state == 2 &&
			inspection->tb.object1 == inspection->tb.object2)
		{
			inspection->stage->state = 1; // Delay next stage
		}
	}
}
