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

#include "LuaCore/ELuaBase.h"
#include "ModuleManager.h"
#include "Editor/ELuaMonitor/ELuaMonitorPanel.h"
#include "Editor/ELuaMemAnalyzer/ELuaMemAnalyzerPanel.h"
#include "Widgets/Docking/SDockTab.h"

/** Declares a log category for this module. */
DECLARE_LOG_CATEGORY_EXTERN(LogELuaProfiler, Log, All);

#ifdef ENABLE_ELUAPROFILER
#endif

class FELuaProfilerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
    
	void OnClickedOpenMonitorPanel();

	void OnClickedOpenMemAnalyzerPanel();

private:
	TSharedRef<class SDockTab> OnSpawnELuaMonitorTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnELuaMemAnalyzerTab(const class FSpawnTabArgs& SpawnTabArgs);
	bool Tick(float DeltaTime);
	void AddMenuExtension(FMenuBuilder& Builder);
	void AddELuaProfilerMenu(FMenuBuilder& Builder);

private:
	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;
	bool m_bTabOpened = false;
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<SELuaMonitorPanel> MonitorPanel;
	TSharedPtr<SELuaMemAnalyzerPanel> MemAnalyzerPanel;
};
