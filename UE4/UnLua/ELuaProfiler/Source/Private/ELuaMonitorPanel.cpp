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

#include "ELuaMonitorPanel.h"
#include "ELuaMonitor.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SScrollBox.h"

SELuaMonitorPanel::SELuaMonitorPanel()
{

}

SELuaMonitorPanel::~SELuaMonitorPanel()
{

}

TSharedRef<class SDockTab> SELuaMonitorPanel::GetSDockTab()
{
	CurRootTINode = FELuaMonitor::GetInstance()->GetRoot();
	ShowRootList = { CurRootTINode };


	// Init TreeViewWidget
	SAssignNew(TreeViewWidget, STreeView<TSharedPtr<FELuaTraceInfoNode>>)
		.ItemHeight(800)
		.TreeItemsSource(&ShowRootList)
		.OnGenerateRow_Raw(this, &SELuaMonitorPanel::OnGenerateRow)
		.OnGetChildren_Raw(this, &SELuaMonitorPanel::OnGetChildrenRaw)
		.SelectionMode(ESelectionMode::None)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column("Name").DefaultLabel(FText::FromName("Name")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("TotalTime(ms)").DefaultLabel(FText::FromName("TotalTime(ms)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("TotalTime(%)").DefaultLabel(FText::FromName("TotalTime(%)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("SelfTime(ms)").DefaultLabel(FText::FromName("SelfTime(ms)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("SelfTime(%)").DefaultLabel(FText::FromName("SelfTime(%)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("Average(ms)").DefaultLabel(FText::FromName("Average(ms)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("Alloc(kb)").DefaultLabel(FText::FromName("Alloc(kb)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("Alloc(%)").DefaultLabel(FText::FromName("Alloc(%)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("GC(kb)").DefaultLabel(FText::FromName("GC(kb)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("GC(%)").DefaultLabel(FText::FromName("GC(%)")).FixedWidth(COL_WIDTH)
			+ SHeaderRow::Column("Calls").DefaultLabel(FText::FromName("Calls")).FixedWidth(COL_WIDTH)
		);


	return SNew(SDockTab)
	.Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
	.Label(FText::FromName("ELuaMonitor"))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			TreeViewWidget.ToSharedRef()
			//SNew(SScrollBox)
			//+ SScrollBox::Slot()
			//[
			//]
		]
	];
}

TSharedRef<ITableRow> SELuaMonitorPanel::OnGenerateRow(TSharedPtr<FELuaTraceInfoNode> TINode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
	SNew(STableRow<TSharedPtr<FELuaTraceInfoNode>>, OwnerTable)
	.Padding(2.0f)/*.Visibility_Lambda([=]() {
		return EVisibility::Visible;
	})*/
	[
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::FromString(TINode->Name);
		}))
		.FixedWidth(COL_WIDTH).DefaultTooltip(TAttribute<FText>::Create([=]() {
			return FText::FromString(TINode->ID);
		}))
		//+ SHeaderRow::Column("Global TotalTime(%)").DefaultLabel(TAttribute<FText>::Create([=]() {
		//	return FText::AsNumber(TINode->TotalTime / CurRootTINode->TotalTime);
		//}))
		//+ SHeaderRow::Column("Global SelfTime(%)").DefaultLabel(TAttribute<FText>::Create([=]() {
		//	return FText::AsNumber(TINode->SelfTime / CurRootTINode->SelfTime);
		//}))
		+SHeaderRow::Column("TotalTime(ms)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->TotalTime / 1000.f);
		}))
		+ SHeaderRow::Column("TotalTime(%)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(CurRootTINode->TotalTime > 0 ? TINode->TotalTime / CurRootTINode->TotalTime : 0);
		}))
		+ SHeaderRow::Column("SelfTime(ms)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->SelfTime / 1000.f);
		}))
		+ SHeaderRow::Column("SelfTime(%)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(CurRootTINode->SelfTime > 0 ? TINode->SelfTime / CurRootTINode->SelfTime : 0);
		}))
		+ SHeaderRow::Column("Average(ms)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->Count > 0 ? TINode->TotalTime / TINode->Count : 0);
		}))
		+ SHeaderRow::Column("Alloc(kb)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->AllocSize / 1000.f);
		}))
		+ SHeaderRow::Column("Alloc(%)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(CurRootTINode->AllocSize > 0 ? TINode->AllocSize / CurRootTINode->AllocSize : 0);
		}))
		+ SHeaderRow::Column("GC(kb)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->GCSize / 1000.f);
		}))
		+ SHeaderRow::Column("GC(%)").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(CurRootTINode->GCSize > 0 ? TINode->GCSize / CurRootTINode->GCSize : 0);
		}))
		.FixedWidth(COL_WIDTH)
		+ SHeaderRow::Column("Calls").DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->Count);
		}))
		.FixedWidth(COL_WIDTH)
	];
}

void SELuaMonitorPanel::OnGetChildrenRaw(TSharedPtr<FELuaTraceInfoNode> TINode, TArray<TSharedPtr<FELuaTraceInfoNode>>& OutChildren)
{
	if (TINode)
	{
		OutChildren = TINode->Children;
	}
}

void SELuaMonitorPanel::Tick(float DeltaTime)
{
	FELuaMonitor::GetInstance()->Tick(DeltaTime);

	if (false/*NeedReGenerate*/)
	{
		TreeViewWidget->RebuildList();
	}

	TreeViewWidget->RequestTreeRefresh();
}
