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

#include "ELuaMonitor.h"

#include "lstate.h"
#include "UnLuaBase.h"
#include "UnLuaDelegates.h"

#define LOCTEXT_NAMESPACE "FELuaMonitorModule"

void FELuaMonitorModule::StartupModule()
{
}

void FELuaMonitorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FELuaMonitorModule, ELuaMonitor)

void* FELuaMonitor::RunningCoroutine = nullptr;

FELuaMonitor::FELuaMonitor()
{
    CurTraceTree = MakeShared<FELuaTraceInfoTree>();
    TickDelegate = FTickerDelegate::CreateRaw(this, &FELuaMonitor::Tick);
    TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	FUnLuaDelegates::OnLuaStateCreated.AddRaw(this, &FELuaMonitor::OnLuaStateCreate);
}

FELuaMonitor::~FELuaMonitor()
{
    CurTraceTree = nullptr;
    LuaFuncPtrMap.Empty();
    LuaTwigsFuncPtrList.Empty();
}

void FELuaMonitor::OnLuaStateCreate(lua_State* L) const
{
	if(State & STARTED)
    {
        // waiting game start
        // re-register lua hook when lua_State changed
        FELuaMonitor::RegisterLuaHook(L);
	    ELuaProfiler::GCSize = 0;
	    ELuaProfiler::AllocSize = 0;
    }
}

void FELuaMonitor::RegisterLuaHook(lua_State* L)
{
    lua_sethook(L, OnHook, ELuaProfiler::HookMask, 0);
    lua_setallocf(L, FELuaMonitor::LuaAllocator, nullptr);
}

void FELuaMonitor::Init()
{
    if (UnLua::GetState())
    {
        State |= INITED;
    }
    else
    {
        State &= (~INITED);
    }
    CurTraceTree->Init();
}

void FELuaMonitor::OnForward()
{
    if (State == RUNING)
    {
        Pause();
    }
    else if ((State & PAUSE) > 0)
    {
        Resume();
    }
    else
    {
        Start();
    }
}

void* FELuaMonitor::LuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
        ELuaProfiler::GCSize += osize;
        FMemory::Free(ptr);
        return nullptr;
    }

	if (!ptr)
    {
        ELuaProfiler::AllocSize += nsize;
        return FMemory::Malloc(nsize);
    }
    else
    {
        ELuaProfiler::GCSize += osize;
        ELuaProfiler::AllocSize += nsize;
        return FMemory::Realloc(ptr, nsize);
    }
}

void FELuaMonitor::Start()
{
    Init();
    if ((State & INITED) > 0)
    {
        if (lua_State* L = UnLua::GetState())
        {
	        FELuaMonitor::RegisterLuaHook(L);
        }
    }
    State |= STARTED;
}

void FELuaMonitor::Stop()
{
    if (State == RUNING)
    {
        if (lua_State* L = UnLua::GetState())
        {
            lua_sethook(L, nullptr, 0, 0);
        }
    }
    State &= (~STARTED);
}

void FELuaMonitor::Pause()
{
    if (State == RUNING)
    {
        if (lua_State* L = UnLua::GetState())
        {
            lua_sethook(L, nullptr, 0, 0);
        }
    }
    State |= PAUSE;
}

void FELuaMonitor::Resume()
{
    if ((State & PAUSE) > 0)
    {
        State &= (~PAUSE);
        if (lua_State* L = UnLua::GetState())
        {
            lua_sethook(L, OnHook, ELuaProfiler::HookMask, 0);
        }
    }
}

void FELuaMonitor::OnClear()
{
    Stop();

    State = CREATED;
    
    LuaFuncPtrMap.Empty();
    LuaTwigsFuncPtrList.Empty();
    FramesTraceTreeList.Empty();

    CurDepth = 0;
    CurFrameIndex = 0;

    CurTraceTree = TSharedPtr<FELuaTraceInfoTree>(new FELuaTraceInfoTree());
}

/*static*/ void FELuaMonitor::OnHook(lua_State* L, lua_Debug* ar)
{
    int32 Event = ar->event;
    if (L == G(L)->mainthread)
    {
        if (nullptr != RunningCoroutine)
        {
            if (lua_State* CoroState = static_cast<lua_State*>(RunningCoroutine))
            {
                if (lua_status(CoroState) >= LUA_ERRRUN)
                {
                    // Error on coroutine
                    FELuaMonitor::GetInstance()->OnHookReturn();
                }
            }
            RunningCoroutine = nullptr;
        }
    }
    else
    {
        // may be running a coroutine
        RunningCoroutine = L;

        lua_getinfo(L, "n", ar);
        if (nullptr != ar->name && strcmp(ar->name, "yield") == 0)
        {
            if (LUA_HOOKCALL == ar->event)
            {
                // Call corotinue yield regarded as a return
                Event = LUA_HOOKRET;
            }
            else if (LUA_HOOKRET == ar->event)
            {
                // Continue corotinue yield regarded as a function call
                Event = LUA_HOOKCALL;
            }
        }
    }
    
    switch (Event)
    {
    case LUA_HOOKCALL:
        FELuaMonitor::GetInstance()->OnHookCall(L, ar);
        break;
    case LUA_HOOKRET:
        FELuaMonitor::GetInstance()->OnHookReturn();
        break;
    default:
        break;
    }
}

void const* FELuaMonitor::GetLuaFunc(lua_State* L, lua_Debug* ar)
{
	lua_getinfo(L, "f", ar);
    const void* luaPtr = lua_topointer(L, -1);
    lua_pop(L, 1);

    if (!LuaFuncPtrMap.Contains(luaPtr))
    {
		lua_getinfo(L, "nS", ar);
	    TCHAR* Name = UTF8_TO_TCHAR(ar->name);
	    FString ID = FString::Printf(TEXT("%s:%d~%d %s"), UTF8_TO_TCHAR(ar->short_src), ar->linedefined, ar->lastlinedefined, Name).Replace(*SandBoxPath, TEXT(""));
        LuaFuncPtrMap.Add(luaPtr, ID);
    }
    return luaPtr;
}

void FELuaMonitor::OnHookCall(lua_State* L, lua_Debug* ar)
{
    if (CurTraceTree)
    {
        if (CurDepth < MaxDepth)
        {
	        void const* luaptr = GetLuaFunc(L, ar);
            if (PurningDepth == 0 && LuaTwigsFuncPtrList.Contains(luaptr))
            {
	            PurningDepth = CurDepth + 1;
            }

            if (PurningDepth == 0)
            {
	            CurTraceTree->OnHookCall(L, luaptr, LuaFuncPtrMap[luaptr], MonitorMode == Statistics);
            }
        }
        CurDepth++;
    }
}

void FELuaMonitor::OnHookReturn()
{
    if (CurTraceTree)
    {
        if (CurDepth <= MaxDepth)
        {
            if (PurningDepth == 0)
            {
				CurTraceTree->OnHookReturn();
            }
        }
        CurDepth--;

        if (CurDepth < PurningDepth/* && PurningDepth > 0*/)
        {
	        PurningDepth = 0;
        }
    }
}

TSharedPtr<FELuaTraceInfoNode> FELuaMonitor::GetRoot(uint32 FrameIndex /* = 0 */)
{
    if ((State & INITED) > 0)
    {
        if (MonitorMode == PerFrame)
        {
            if (GetTotalFrames() > 0)
            {
	            const int32 Index = (0 < GetCurFrameIndex() && GetCurFrameIndex() < GetTotalFrames()) ? CurFrameIndex - 1 : GetTotalFrames() - 1;
                return FramesTraceTreeList[Index]->GetRoot();
            }
            else
            {
                return nullptr;
            }
        }
        else if (MonitorMode == Statistics)
        {
            CurTraceTree->CountSelfTime(MonitorSortMode);
            return CurTraceTree->Statisticize();
        }
        else
        {
            CurTraceTree->CountSelfTime(MonitorSortMode);
            return CurTraceTree->GetRoot();
        }
    }
    return nullptr;
}

void FELuaMonitor::SetCurFrameIndex(int32 Index)
{
    if (Index > 0 && Index < GetTotalFrames())
    {
        CurFrameIndex = Index;
    }
    else
    {
        CurFrameIndex = GetTotalFrames();
    }
}

void FELuaMonitor::PerFrameModeUpdate(bool Manual /* = false */)
{
    if (FramesTraceTreeList.Num() > 0)
    {
        TSharedPtr<FELuaTraceInfoTree> PreFrameTree = FramesTraceTreeList[FramesTraceTreeList.Num() - 1];
        if (PreFrameTree->GetRoot() && PreFrameTree->GetRoot()->Children.Num() == 0)
        {
            FramesTraceTreeList.Pop();
            if (CurFrameIndex >= (uint32)FramesTraceTreeList.Num())
            {
                CurFrameIndex = FramesTraceTreeList.Num() - 1;
            }
        }
    }
    CurTraceTree->CountSelfTime(MonitorSortMode);
    FramesTraceTreeList.Add(CurTraceTree);
    CurTraceTree = MakeShared<FELuaTraceInfoTree>();
    CurTraceTree->Init();

    if (CurFrameIndex == FramesTraceTreeList.Num() - 1)
    {
        CurFrameIndex = FramesTraceTreeList.Num();
    }
}

bool FELuaMonitor::Tick(float DeltaTime)
{
    if (State == RUNING)
    {
        // game stop
        if (!UnLua::GetState())
        {
            Stop();
        }

        if (MonitorMode == PerFrame && !ManualUpdated)
        {
            PerFrameModeUpdate();
        }
    }
    else if(State == STARTED)
    {
        // waiting game start
        if (UnLua::GetState())
        {
            OnForward();
        }
    }
    return true;
}

void FELuaMonitor::Deserialize(const FString& Path, ELuaMonitorMode& EMode)
{
    if (Path.IsEmpty())
        return;

    TArray<uint8> Buffer;
    FFileHelper::LoadFileToArray(Buffer, *Path);
    FMemoryReader ReaderAr(Buffer);

    int32 Mode;
    ReaderAr << Mode;
    EMode = static_cast<ELuaMonitorMode>(Mode);
    if (EMode == PerFrame)
    {
        int32 TreeNum;
		ReaderAr << TreeNum;
        for (int32 i = 0; i < TreeNum; i++)
        {
			TSharedPtr<FELuaTraceInfoTree> TraceTree = MakeShared<FELuaTraceInfoTree>();
	        ReaderAr << TraceTree;
            FramesTraceTreeList.Add(TraceTree);
        }
    }
    else
    {
		CurTraceTree = MakeShared<FELuaTraceInfoTree>();
		ReaderAr << CurTraceTree;
    }
    State |= INITED;
}

void FELuaMonitor::Serialize(const FString& Path)
{
    FString FilePath = Path;
    if (FilePath.IsEmpty())
    {
        const FString FileName = FApp::GetProjectName() + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + ELuaProfiler::ELUA_PROF_FILE_SUFFIX;
        FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ELuaProfiler"), FileName);
    }
    
    TArray<uint8> Buffer;
    FMemoryWriter MemoryAr(Buffer);
    int32 Mode = MonitorMode;
    MemoryAr << Mode;
    if (MonitorMode == PerFrame)
    {
        int32 TreeNum = FramesTraceTreeList.Num();
		MemoryAr << TreeNum;
        for (int32 i = 0; i < TreeNum; i++)
        {
	        MemoryAr << FramesTraceTreeList[i];
        }
        SetCurFrameIndex(TreeNum - 1);
    }
    else
    {
		MemoryAr << CurTraceTree;
    }

    FFileHelper::SaveArrayToFile(Buffer, *FilePath);
}

void FELuaMonitor::OnCommandStart(const TArray<FString>& Args)
{
    if (Args.Num() < 1)
        return;
    
    const FString ExeCommand = Args[0].ToLower();

    int32 Mode = -1;
    if (Args.Num() >= 2)
        Mode = FCString::Atoi(*Args[1]);

    int32 Depth = 10;
    if (Args.Num() >= 3)
		Depth = FCString::Atoi(*Args[2]);

    if (ExeCommand.Equals(TEXT("start")))
    {
        if (Mode >= 0)
        {
            FELuaMonitor::GetInstance()->SetMonitorMode(static_cast<ELuaMonitorMode>(Mode));
        }
        FELuaMonitor::GetInstance()->SetMaxDepth(Depth);
        FELuaMonitor::GetInstance()->Start();
    }
    else if (ExeCommand.Equals(TEXT("stop")))
    {
        FELuaMonitor::GetInstance()->Stop();
        FELuaMonitor::GetInstance()->Serialize("");
    }
    else
    {
        if (Mode == ELuaMonitorMode::PerFrame)
        {
            if (ExeCommand.Equals(TEXT("pause")))
            {
                FELuaMonitor::GetInstance()->Pause();
            }
            else if (ExeCommand.Equals(TEXT("resume")))
            {
                FELuaMonitor::GetInstance()->Resume();
            }
        }
    }
}

void FELuaMonitor::OnPurning(void const* luaptr)
{
	if (luaptr)
	{
		LuaTwigsFuncPtrList.Add(luaptr);
        PurningDepth = CurDepth + 1;
	}
}



static FAutoConsoleCommand ExecGMCommand(
    TEXT("ELuaProfiler"),
    TEXT("ELuaProfiler, with 3 arg : start/stop/pause/resume/...; Mode 0:PerFrame/1:Total/2:Statistics; Depth [10]"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FELuaMonitor::OnCommandStart)
);
