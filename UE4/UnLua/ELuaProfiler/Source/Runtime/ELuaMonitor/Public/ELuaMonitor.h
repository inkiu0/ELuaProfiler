// The MIT License (MIT)

// Copyright 2020 HankShu inkiu0@gmail.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "ELuaBase.h"
#include "ELuaTraceInfoTree.h"

#include "Runtime/Launch/Resources/Version.h"
#if (ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20)
#include "Modules/ModuleManager.h"
#else
#include "ModuleManager.h"
#endif


class FELuaMonitorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

enum EMonitorState : uint32
{
    CREATED = 0,
    STARTED = 1 << 0,
    INITED = 1 << 1,
    RUNING = STARTED | INITED,
    PAUSE = 1 << 2,
    //PAUSE = 1 << 3,
};

class ELUAMONITOR_API FELuaMonitor
{
public:
    static FELuaMonitor* GetInstance()
    {
        static FELuaMonitor Instance;
        return &Instance;
    }

    void OnForward();

    void OnClear();

    bool Tick(float DeltaTime);

    int32 GetMaxDepth() const { return MaxDepth; }

    void SetMaxDepth(int32 Depth) { MaxDepth = Depth; }

    TSharedPtr<FELuaTraceInfoNode> GetRoot(uint32 FrameIndex = 0);

    bool IsRuning() const { return State == RUNING; }

    void Deserialize(const FString& Path, ELuaMonitorMode& EMode);

    void Serialize(const FString& Path);

    void SetMonitorMode(ELuaMonitorMode Mode) { MonitorMode = Mode; }

    int32 GetTotalFrames() const { return FramesTraceTreeList.Num(); }

    TArray<TSharedPtr<FELuaTraceInfoTree>> GetFrameList() const { return FramesTraceTreeList; }

    int32 GetCurFrameIndex() const { return CurFrameIndex; }

    void SetCurFrameIndex(int32 Index);

    void SetSortMode(EMonitorSortMode Mode) { MonitorSortMode = Mode; }

    EMonitorSortMode GetSortMode() const { return MonitorSortMode; }

    void OnPurning(void const* luaptr);
    
    static void OnCommandStart(const TArray<FString>& Args);
private:
    FELuaMonitor();
    ~FELuaMonitor();

    static void RegisterLuaHook(lua_State* L);

    static void OnHook(lua_State* L, lua_Debug* ar);
    
    static void* LuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize);

    void OnLuaStateCreate(lua_State* L) const;

    void OnHookCall(lua_State* L, lua_Debug* ar);

    void OnHookReturn();

    void OnHookError();

    void Init();

    void Start();

    void Stop();

    void Pause();

    void Resume();

    void PerFrameModeUpdate(bool Manual = false);
    
	void const* GetLuaFunc(lua_State* L, lua_Debug* ar);

private:
    /* max depth of hook  tracking */
    uint32 MaxDepth = 100;

    /* current depth of hook tracking */
    uint32 CurDepth = 0;

    /* Purning Lua Twigs */
    uint32 PurningDepth = 0;

    TSharedPtr<FELuaTraceInfoTree> CurTraceTree;

    TArray<TSharedPtr<FELuaTraceInfoTree>> FramesTraceTreeList;

    TMap<void const*, FString> LuaFuncPtrMap;

    TArray<void const*> LuaTwigsFuncPtrList;

    uint32 CurFrameIndex = 0;

    bool Started = false;

    bool Inited = false;

    bool ManualUpdated = false;

    uint32 State = CREATED;

    ELuaMonitorMode MonitorMode = Total;

    EMonitorSortMode MonitorSortMode = EMonitorSortMode::TotalTime;

    static void* RunningCoroutine;

    FTickerDelegate TickDelegate;
    FDelegateHandle TickDelegateHandle;
};
