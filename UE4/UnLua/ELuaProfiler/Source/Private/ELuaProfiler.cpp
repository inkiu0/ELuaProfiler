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

#include "ELuaProfiler.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "FileManager.h"
#include "Engine/GameEngine.h"
#include "Containers/Ticker.h"
#include "UnLuaBase.h"
#if WITH_EDITOR
#include "LevelEditor.h"
#include "EditorStyle.h"
#endif

DEFINE_LOG_CATEGORY(LogELuaProfiler)
#define LOCTEXT_NAMESPACE "FELuaProfilerModule"

namespace {
	static const FName ELuaProfilerTabName(TEXT("ELuaProfiler"));

	uint32_t currentLayer = 0;
}

void FELuaProfilerModule::StartupModule()
{
#if WITH_EDITOR

	PluginCommands = MakeShareable(new FUICommandList);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FELuaProfilerModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}


	if (GIsEditor && !IsRunningCommandlet())
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ELuaProfilerTabName,
			FOnSpawnTab::CreateRaw(this, &FELuaProfilerModule::OnSpawnPluginTab))
			.SetDisplayName(LOCTEXT("Flua_wrapperTabTitle", "Easy Lua Profiler"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);
		TickDelegate = FTickerDelegate::CreateRaw(this, &FELuaProfilerModule::Tick);
		TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	}
#endif
}

void FELuaProfilerModule::ShutdownModule()
{
#if WITH_EDITOR
	ClearCurProfiler();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ELuaProfilerTabName);
#endif
}

void FELuaProfilerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(ELuaProfilerTabName);
}

bool FELuaProfilerModule::Tick(float DeltaTime)
{
	if (!tabOpened)
	{
		return true;
	}

	return true;
}

TSharedRef<class SDockTab> FELuaProfilerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab);
}

void FELuaProfilerModule::ClearCurProfiler()
{
	currentLayer = 0;
}

void FELuaProfilerModule::AddMenuExtension(FMenuBuilder & Builder)
{
}

void FELuaProfilerModule::OnTabClosed(TSharedRef<SDockTab>)
{
	tabOpened = false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FELuaProfilerModule, ELuaProfiler)
