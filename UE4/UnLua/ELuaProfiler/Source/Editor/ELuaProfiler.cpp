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
#if WITH_EDITOR
#include "Editor/Styles/ELuaStyle.h"
#include "LevelEditor.h"
#include "Styles/ELuaProfilerCommands.h"
#endif

DEFINE_LOG_CATEGORY(LogELuaProfiler)
#define LOCTEXT_NAMESPACE "FELuaProfilerModule"

void FELuaProfilerModule::StartupModule()
{
#if WITH_EDITOR

	FELuaProfilerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FELuaProfilerCommands::Get().OpenMonitorPanel,
		FExecuteAction::CreateRaw(this, &FELuaProfilerModule::OnClickedOpenMonitorPanel),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FELuaProfilerCommands::Get().OpenMemAnalyzerPanel,
		FExecuteAction::CreateRaw(this, &FELuaProfilerModule::OnClickedOpenMemAnalyzerPanel),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FELuaProfilerModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}


	if (GIsEditor && !IsRunningCommandlet())
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ELuaProfiler::ELuaMonitorTabName,
			FOnSpawnTab::CreateRaw(this, &FELuaProfilerModule::OnSpawnELuaMonitorTab))
			.SetDisplayName(LOCTEXT("Flua_wrapperTabTitle", "DONT WORRY BE HAPPY"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ELuaProfiler::ELuaMemAnalyzerTabName,
			FOnSpawnTab::CreateRaw(this, &FELuaProfilerModule::OnSpawnELuaMemAnalyzerTab))
			.SetDisplayName(LOCTEXT("Flua_wrapperTabTitle", "DONT WORRY BE HAPPY"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		TickDelegate = FTickerDelegate::CreateRaw(this, &FELuaProfilerModule::Tick);
		TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	}

	if (GIsEditor)
	{
		FELuaStyle::Initialize();
	}
#endif
}

void FELuaProfilerModule::ShutdownModule()
{
#if WITH_EDITOR
	if (MonitorPanel.IsValid())
	{
		MonitorPanel = nullptr;
	}

	if (MemAnalyzerPanel.IsValid())
	{
		MemAnalyzerPanel = nullptr;
	}

	FELuaProfilerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ELuaProfiler::ELuaMonitorTabName);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ELuaProfiler::ELuaMemAnalyzerTabName);

	if (GIsEditor)
	{
		FELuaStyle::Shutdown();
	}
#endif
}

void FELuaProfilerModule::OnClickedOpenMonitorPanel()
{
	FGlobalTabmanager::Get()->InvokeTab(ELuaProfiler::ELuaMonitorTabName);
}

void FELuaProfilerModule::OnClickedOpenMemAnalyzerPanel()
{
	FGlobalTabmanager::Get()->InvokeTab(ELuaProfiler::ELuaMemAnalyzerTabName);
}

bool FELuaProfilerModule::Tick(float DeltaTime)
{
	if (MonitorPanel.IsValid() && MonitorPanel->IsOpening())
	{
		MonitorPanel->DeferredTick(DeltaTime);
	}

	if (MemAnalyzerPanel.IsValid() && MemAnalyzerPanel->IsOpening())
	{
		MemAnalyzerPanel->DeferredTick(DeltaTime);
	}

	return true;
}

TSharedRef<class SDockTab> FELuaProfilerModule::OnSpawnELuaMonitorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if (!MonitorPanel.IsValid())
	{
		SAssignNew(MonitorPanel, SELuaMonitorPanel);
	}
	TSharedRef<SDockTab> DockTab = MonitorPanel->GetSDockTab();
	return DockTab;
}

TSharedRef<class SDockTab> FELuaProfilerModule::OnSpawnELuaMemAnalyzerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if (!MemAnalyzerPanel.IsValid())
	{
		SAssignNew(MemAnalyzerPanel, SELuaMemAnalyzerPanel);
	}
	TSharedRef<SDockTab> DockTab = MemAnalyzerPanel->GetSDockTab();
	return DockTab;
}

void FELuaProfilerModule::AddMenuExtension(FMenuBuilder & Builder)
{
#if WITH_EDITOR
	Builder.BeginSection("ELuaProfiler", TAttribute<FText>(FText::FromString("ELuaProfiler")));
	{
		Builder.AddSubMenu(FText::FromString("ELuaProfiler"),
			FText::FromString("DONT WORRY BE HAPPY"),
			FNewMenuDelegate::CreateRaw(this, &FELuaProfilerModule::AddELuaProfilerMenu));
	}
	Builder.EndSection();
#endif
}

void FELuaProfilerModule::AddELuaProfilerMenu(FMenuBuilder& Builder)
{
	Builder.BeginSection("ELuaProfiler", TAttribute<FText>(FText::FromString("ELuaProfiler")));
	{
		Builder.AddMenuEntry(FELuaProfilerCommands::Get().OpenMonitorPanel);

		Builder.AddMenuEntry(FELuaProfilerCommands::Get().OpenMemAnalyzerPanel);
	}
	Builder.EndSection();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FELuaProfilerModule, ELuaProfiler)
