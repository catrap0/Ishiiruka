// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#ifndef VIDEO_BACKEND_H_
#define VIDEO_BACKEND_H_

#include <string>
#include <vector>

#include "Common/ChunkFile.h"
#include "VideoCommon/PerfQueryBase.h"

namespace MMIO { class Mapping; }

typedef struct _EFBPeekCacheElement
{
	u32 ColorValue;
	u32 DepthValue;
	u32 ColorFrame;
	u32 DepthFrame;	
}EFBPeekCacheElement;

enum FieldType
{
	FIELD_PROGRESSIVE = 0,
	FIELD_UPPER,
	FIELD_LOWER
};

enum EFBAccessType
{
	PEEK_Z = 0,
	POKE_Z,
	PEEK_COLOR,
	POKE_COLOR
};

struct SCPFifoStruct
{
	// fifo registers
	volatile u32 CPBase;
	volatile u32 CPEnd;
	u32 CPHiWatermark;
	u32 CPLoWatermark;
	volatile u32 CPReadWriteDistance;
	volatile u32 CPWritePointer;
	volatile u32 CPReadPointer;
	volatile u32 CPBreakpoint;
	volatile u32 SafeCPReadPointer;
	// Super Monkey Ball Adventure require this.
	// Because the read&check-PEToken-loop stays in its JITed block I suppose.
	// So no possiblity to ack the Token irq by the scheduler until some sort of PPC watchdog do its mess.
	volatile u16 PEToken;

	volatile u32 bFF_GPLinkEnable;
	volatile u32 bFF_GPReadEnable;
	volatile u32 bFF_BPEnable;
	volatile u32 bFF_BPInt;
	volatile u32 bFF_Breakpoint;

	volatile u32 CPCmdIdle;
	volatile u32 CPReadIdle;        

	volatile u32 bFF_LoWatermarkInt;
	volatile u32 bFF_HiWatermarkInt;

	volatile u32 bFF_LoWatermark;
	volatile u32 bFF_HiWatermark;

	// for GP watchdog hack
	volatile u32 Fake_GPWDToken; // cicular incrementer
	volatile u32 isGpuReadingData;
};

class VideoBackend
{
public:
	virtual ~VideoBackend() {}

	virtual void EmuStateChange(EMUSTATE_CHANGE) = 0;

	virtual void UpdateFPSDisplay(const std::string&) = 0;

	virtual unsigned int PeekMessages() = 0;

	virtual bool Initialize(void *&) = 0;
	virtual void Shutdown() = 0;
	virtual void RunLoop(bool enable) = 0;

	virtual std::string GetName() = 0;
	virtual std::string GetDisplayName() { return GetName(); }

	virtual void ShowConfig(void*) {}

	virtual void Video_Prepare() = 0;
	virtual void Video_EnterLoop() = 0;
	virtual void Video_ExitLoop() = 0;
	virtual void Video_Cleanup() = 0; // called from gl/d3d thread

	virtual void Video_BeginField(u32, u32, u32) = 0;
	virtual void Video_EndField() = 0;

	virtual u32 Video_AccessEFB(EFBAccessType, u32, u32, u32) = 0;
	virtual u32 Video_GetQueryResult(PerfQueryType type) = 0;

	virtual void Video_AddMessage(const std::string& pstr, unsigned int milliseconds) = 0;
	virtual void Video_ClearMessages() = 0;
	virtual bool Video_Screenshot(const std::string& filename) = 0;

	virtual void Video_SetRendering(bool bEnabled) = 0;

	virtual void Video_GatherPipeBursted() = 0;

	virtual bool Video_IsPossibleWaitingSetDrawDone() = 0;
	virtual bool Video_IsHiWatermarkActive() = 0;
	virtual void Video_AbortFrame() = 0;

	// Registers MMIO handlers for the CommandProcessor registers.
	virtual void RegisterCPMMIO(MMIO::Mapping* mmio, u32 base) = 0;

	static void PopulateList();
	static void ClearList();
	static void ActivateBackend(const std::string& name);

	// waits until is paused and fully idle, and acquires a lock on that state.
	// or, if doLock is false, releases a lock on that state and optionally unpauses.
	// calls must be balanced and non-recursive (once with doLock true, then once with doLock false).
	virtual void PauseAndLock(bool doLock, bool unpauseOnUnlock=true) = 0;

	// the implementation needs not do synchronization logic, because calls to it are surrounded by PauseAndLock now
	virtual void DoState(PointerWrap &p) = 0;
	
	virtual void CheckInvalidState() = 0;
};

extern std::vector<VideoBackend*> g_available_video_backends;
extern VideoBackend* g_video_backend;

// inherited by dx9/dx11/ogl backends
class VideoBackendHardware : public VideoBackend
{
	void RunLoop(bool enable);
	bool Initialize(void *&) { InitializeShared(); return true; }

	void EmuStateChange(EMUSTATE_CHANGE);

	void Video_EnterLoop();
	void Video_ExitLoop();
	void Video_BeginField(u32, u32, u32);
	void Video_EndField();

	u32 Video_AccessEFB(EFBAccessType, u32, u32, u32);
	u32 Video_GetQueryResult(PerfQueryType type);
	
	void Video_AddMessage(const std::string& str, unsigned int milliseconds);
	void Video_ClearMessages();
	bool Video_Screenshot(const std::string& filename);

	void Video_SetRendering(bool bEnabled);

	void Video_GatherPipeBursted();

	bool Video_IsPossibleWaitingSetDrawDone();
	bool Video_IsHiWatermarkActive();
	void Video_AbortFrame();

	// Registers MMIO handlers for the CommandProcessor registers.
	void RegisterCPMMIO(MMIO::Mapping* mmio, u32 base) override;

	void PauseAndLock(bool doLock, bool unpauseOnUnlock=true);
	void DoState(PointerWrap &p);
	
	bool m_invalid;
	u32 m_EFB_PCache_Width;
	u32 m_EFB_PCache_Height;
	u32 m_EFB_PCache_Size;	
	u32 m_EFB_PCache_Divisor;
	u32 m_EFB_PCache_Life;
	EFBPeekCacheElement* m_EFB_PCache;
public:	
	 void CheckInvalidState();
	 VideoBackendHardware();
	 ~VideoBackendHardware();
protected:
	void InitializeShared();
	void InvalidState();
};

#endif