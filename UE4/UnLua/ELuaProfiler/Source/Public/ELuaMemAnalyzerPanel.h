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
#include "ELuaMemInfoNode.h"
#include "ELuaMemAnalyzer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Docking/SDockTab.h"

class SELuaMemAnalyzerPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SELuaMemAnalyzerPanel) {}
	SLATE_END_ARGS()

	void Construct(const SELuaMemAnalyzerPanel::FArguments& InArgs);

	TSharedRef<class SDockTab> GetSDockTab();

	void DeferredTick(float DeltaTime);

	bool IsOpening() { return TabIsOpening; }

private:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FELuaMemInfoNode> MINode, const TSharedRef<STableViewBase>& OwnerTable);

	void OnGetChildrenRaw(TSharedPtr<FELuaMemInfoNode> MINode, TArray<TSharedPtr<FELuaMemInfoNode>>& OutChildren);

	void UpdateShowingRoot();

	void OnCloseTab(TSharedRef<SDockTab> Tab);

	FReply OnSnapshotBtnClicked();

	FReply OnGCBtnClicked();

	void OnRefreshSOPToggleStyle();

	FReply OnSOPBtnClicked(ESnapshotOp ESOP);

	const FButtonStyle& GetToggleStyle(ESnapshotOp ESOP);

private:

	TSharedPtr<SDockTab> DockTab;

	TSharedPtr<SVerticalBox> MemToggleListWidget;

	TSharedPtr<STreeView<TSharedPtr<FELuaMemInfoNode>>> TreeViewWidget;

	TArray<TSharedPtr<FELuaMemInfoNode>> ShowingNodeList;

	TSharedPtr<FELuaMemInfoNode> CurMIRoot;

	bool TabIsOpening = false;

	TSharedPtr<SButton> AndBtn;

	TSharedPtr<SButton> OrBtn;

	TSharedPtr<SButton> XorBtn;
};

