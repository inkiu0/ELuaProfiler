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
#include "LevelEditor.h"
#include "ELuaProfilerCommands.h"
#endif

DEFINE_LOG_CATEGORY(LogELuaProfiler)
#define LOCTEXT_NAMESPACE "FELuaProfilerModule"

void FELuaProfilerModule::StartupModule()
{
#if WITH_EDITOR

	FELuaProfilerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FELuaProfilerCommands::Get().OpenWindow,
		FExecuteAction::CreateRaw(this, &FELuaProfilerModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FELuaProfilerModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}


	if (GIsEditor && !IsRunningCommandlet())
	{
		MonitorPanel = MakeShareable(new SELuaMonitorPanel);
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ELuaProfiler::ELuaProfilerTabName,
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
	MonitorPanel = nullptr;

	FELuaProfilerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ELuaProfiler::ELuaProfilerTabName);
#endif
}

void FELuaProfilerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(ELuaProfiler::ELuaProfilerTabName);
}

bool FELuaProfilerModule::Tick(float DeltaTime)
{
	if (!m_bTabOpened)
	{
		return true;
	}

	MonitorPanel->Tick(DeltaTime);

	return true;
}

TSharedRef<class SDockTab> FELuaProfilerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if (MonitorPanel.IsValid())
	{
		TSharedRef<SDockTab> DockTab = MonitorPanel->GetSDockTab();
		DockTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FELuaProfilerModule::OnTabClosed));
		m_bTabOpened = true;
		return DockTab;
	}
	else
	{
		return SNew(SDockTab).TabRole(ETabRole::NomadTab);
	}
}

void FELuaProfilerModule::AddMenuExtension(FMenuBuilder & Builder)
{
#if WITH_EDITOR
	Builder.AddMenuEntry(FELuaProfilerCommands::Get().OpenWindow);
#endif
}

void FELuaProfilerModule::OnTabClosed(TSharedRef<SDockTab> Tab)
{
	m_bTabOpened = false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FELuaProfilerModule, ELuaProfiler)
