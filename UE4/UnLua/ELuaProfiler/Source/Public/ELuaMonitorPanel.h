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

#include "CoreMinimal.h"
#include "ELuaTraceInfoNode.h"
#include "ELuaTraceInfoTree.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Docking/SDockTab.h"

class SELuaMonitorPanel// : public SCompoundWidget
{
public:
	SELuaMonitorPanel();
	~SELuaMonitorPanel();
	//SLATE_BEGIN_ARGS(SELuaMonitorPanel)
	//:_StdLineVisibility(EVisibility::Visible)
	//{}

	//SLATE_ATTRIBUTE(EVisibility, StdLineVisibility)

	//SLATE_END_ARGS()

	//void Construct(const FArguments& InArgs);

 //   void Refresh(FELuaTraceInfoTree& InfoTree);

    TSharedRef<class SDockTab> GetSDockTab();

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FELuaTraceInfoNode> TINode, const TSharedRef<STableViewBase>& OwnerTable);

	void OnGetChildrenRaw(TSharedPtr<FELuaTraceInfoNode> Parent, TArray<TSharedPtr<FELuaTraceInfoNode>>& OutChildren);
private:
	TSharedPtr<STreeView<TSharedPtr<FELuaTraceInfoNode>>> TreeViewWidget;

	TArray<TSharedPtr<FELuaTraceInfoNode>> CurTINodeList;

	TSharedPtr<FELuaTraceInfoNode> CurRootTINode;

	const static int COL_WIDTH = 80;
};
