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
#include "Runtime/ELuaMonitor/ELuaMonitor.h"
#include "EditorStyleSet.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SNumericDropDown.h"

#define LOCTEXT_NAMESPACE "ELuaProfiler"

void SELuaMonitorPanel::Construct(const SELuaMonitorPanel::FArguments& InArgs)
{

}

TSharedRef<class SDockTab> SELuaMonitorPanel::GetSDockTab()
{
	TabIsOpening = true;

	FELuaMonitor::GetInstance()->SetMonitorMode(MonitorMode);

	UpdateShowingRoot();


	TArray<SNumericDropDown<float>::FNamedValue> NamedValuesForMonitorMode;
	NamedValuesForMonitorMode.Add(SNumericDropDown<float>::FNamedValue((float)ELuaMonitorMode::PerFrame, FText::FromName("PerFrame"), FText::FromName("PerFrameRecord")));
	NamedValuesForMonitorMode.Add(SNumericDropDown<float>::FNamedValue((float)ELuaMonitorMode::Total, FText::FromName("Total"), FText::FromName("RecordTotalInfo")));
	NamedValuesForMonitorMode.Add(SNumericDropDown<float>::FNamedValue((float)ELuaMonitorMode::Statistics, FText::FromName("Statistics"), FText::FromName("Tile all node")));


	// Init TreeViewWidget
	SAssignNew(TreeViewWidget, STreeView<TSharedPtr<FELuaTraceInfoNode>>)
		.ItemHeight(800)
		.TreeItemsSource(&ShowingNodeList)
		.OnGenerateRow(this, &SELuaMonitorPanel::OnGenerateRow)
		.OnGetChildren(this, &SELuaMonitorPanel::OnGetChildrenRaw)
		.SelectionMode(ESelectionMode::None)
		.HighlightParentNodesForSelection(true)
		.HeaderRow
		(
			SNew(SHeaderRow)
			+ SHeaderRow::Column("Name").DefaultLabel(FText::FromName("Name")).HAlignHeader(HAlign_Fill)
			+ SHeaderRow::Column("TotalTime(ms)").DefaultLabel(FText::FromName("TotalTime(ms)")).FixedWidth(90)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == TotalTime ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(TotalTime);
			})
			+ SHeaderRow::Column("TotalTime(%)").DefaultLabel(FText::FromName("TotalTime(%)")).FixedWidth(COL_WIDTH)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == TotalTime ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(TotalTime);
			})

			+ SHeaderRow::Column("SelfTime(ms)").DefaultLabel(FText::FromName("SelfTime(ms)")).FixedWidth(COL_WIDTH)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == SelfTime ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(SelfTime);
			})
			+ SHeaderRow::Column("SelfTime(%)").DefaultLabel(FText::FromName("SelfTime(%)")).FixedWidth(COL_WIDTH)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == SelfTime ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(SelfTime);
			})

			+ SHeaderRow::Column("Average(ms)").DefaultLabel(FText::FromName("Average(ms)")).FixedWidth(COL_WIDTH)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == Average ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(Average);
			})

			+ SHeaderRow::Column("Alloc(kb)").DefaultLabel(FText::FromName("Alloc(kb)")).FixedWidth(COL_WIDTH)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == Alloc ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(Alloc);
			})
			+ SHeaderRow::Column("Alloc(%)").DefaultLabel(FText::FromName("Alloc(%)")).FixedWidth(60)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == Alloc ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(Alloc);
			})

			+ SHeaderRow::Column("GC(kb)").DefaultLabel(FText::FromName("GC(kb)")).FixedWidth(60)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == GC ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(GC);
			})
			+ SHeaderRow::Column("GC(%)").DefaultLabel(FText::FromName("GC(%)")).FixedWidth(50)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == GC ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(GC);
			})

			+ SHeaderRow::Column("Calls").DefaultLabel(FText::FromName("Calls")).FixedWidth(60)
			.SortMode_Lambda([&]() { return FELuaMonitor::GetInstance()->GetSortMode() == Calls ? EColumnSortMode::Descending : EColumnSortMode::None; })
			.OnSort_Lambda([&](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
			{
				FELuaMonitor::GetInstance()->SetSortMode(Calls);
			})
		);


	SAssignNew(ControllerBar, SHorizontalBox);
	ControllerBar->AddSlot().HAlign(HAlign_Left).VAlign(VAlign_Center).AutoWidth()
	[
		SNew(SNumericDropDown<float>)
			.LabelText(FText::FromName("Mode:"))
			.bShowNamedValue(true)
			.DropDownValues(NamedValuesForMonitorMode)
			.IsEnabled(true)
			.Value_Lambda([this]() { return (float)MonitorMode; })
			.OnValueChanged(this, &SELuaMonitorPanel::OnModeChanged)
	];
	ControllerBar->AddSlot()
	[
		SNew(SSpacer)
	];
	ControllerBar->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Center).AutoWidth()
	[
		SNew(SButton)
		.ButtonStyle(FEditorStyle::Get(), "NoBorder")																// 无底图以免按钮发白
		.ContentPadding(2.0)
		.IsFocusable(false)
		.OnClicked(this, &SELuaMonitorPanel::OnForwardBtnClicked)
		[
			SNew(SImage)
			.Image(this, &SELuaMonitorPanel::GetForwardIcon)
		]
	];
	ControllerBar->AddSlot()
	[
		SNew(SSpacer)
	];

	ControllerBar->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Center).AutoWidth()
	[
		SNew(STextBlock)
		.Text(FText::FromName("Depth: "))
	];

	ControllerBar->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Center).AutoWidth()
	[
		SNew(SSpinBox<int32>)
		.Delta(1)
		.MinValue(1)
		.MaxValue(1000)
		.Value(this, &SELuaMonitorPanel::OnGetMaxDepth)
		.OnValueChanged(this, &SELuaMonitorPanel::OnMaxDepthChanged)
	];

	ControllerBar->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Center).AutoWidth()
	[
		SNew(SButton)
		.ButtonStyle(FEditorStyle::Get(), "NoBorder")																// 无底图以免按钮发白
		.ContentPadding(2.0)
		.IsFocusable(false)
		.OnClicked(this, &SELuaMonitorPanel::OnClearBtnClicked)
		.ToolTipText(FText::FromName("Clear and stop monitor"))
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("Cross"))
		]
	];

	OnGenerateFrameController();


	TSharedPtr<SDockTab> Tab;
	SAssignNew(Tab, SDockTab)
	.Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
	.Label(FText::FromName("ELuaMonitor"))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			//.BorderBackgroundColor(FLinearColor(.50f, .50f, .50f, 1.0f))
			[
				ControllerBar.ToSharedRef()
			]
		]

		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				TreeViewWidget.ToSharedRef()
			]
		]
	];
	Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &SELuaMonitorPanel::OnCloseTab));
	return Tab.ToSharedRef();
}

TSharedRef<ITableRow> SELuaMonitorPanel::OnGenerateRow(TSharedPtr<FELuaTraceInfoNode> TINode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
	SNew(STableRow<TSharedPtr<FELuaTraceInfoNode>>, OwnerTable)
	[
		SNew(SHeaderRow)
		+ SHeaderRow::Column("Name").DefaultLabel(FText::FromString(TINode->ID))
		.DefaultTooltip(FText::FromString(TINode->ID)).HAlignHeader(HAlign_Fill)
		+SHeaderRow::Column("TotalTime(ms)").FixedWidth(90).DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->TotalTime);
		}))
		+ SHeaderRow::Column("TotalTime(%)").FixedWidth(COL_WIDTH).DefaultLabel(TAttribute<FText>::Create([=]() {
			if (TINode->Parent && TINode->Parent->TotalTime > 0)
			{
				double d = TINode->TotalTime / TINode->Parent->TotalTime;
				return FText::AsPercent(d);
			} 
			else
			{
				return FText::AsPercent(0.f);
			}
		}))
		+ SHeaderRow::Column("SelfTime(ms)").FixedWidth(COL_WIDTH).DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->SelfTime);
		}))
		+ SHeaderRow::Column("SelfTime(%)").FixedWidth(COL_WIDTH).DefaultLabel(TAttribute<FText>::Create([=]() {
			if (TINode->Parent && TINode->Parent->TotalTime > 0)
			{
				double d = TINode->SelfTime / TINode->Parent->TotalTime;
				return FText::AsPercent(d);
			}
			return FText::AsPercent(0.f);
		}))
		+ SHeaderRow::Column("Avg(ms)").FixedWidth(COL_WIDTH).DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->Average);
		}))
		+ SHeaderRow::Column("Alloc(kb)").FixedWidth(COL_WIDTH).DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->AllocSize);
		}))
		+ SHeaderRow::Column("Alloc(%)").FixedWidth(60).DefaultLabel(TAttribute<FText>::Create([=]() {
			if (TINode->Parent && TINode->Parent->AllocSize > 0)
			{
				float p = TINode->AllocSize / TINode->Parent->AllocSize;
				return FText::AsPercent(p);
			}
			return FText::AsPercent(0.f);
		}))
		+ SHeaderRow::Column("GC(kb)").FixedWidth(60).DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->GCSize);
		}))
		+ SHeaderRow::Column("GC(%)").FixedWidth(50).DefaultLabel(TAttribute<FText>::Create([=]() {
			if (TINode->Parent && TINode->Parent->GCSize > 0)
			{
				float p = TINode->GCSize / TINode->Parent->GCSize;
				return FText::AsPercent(p);
			}
			return FText::AsPercent(0.f);
		}))
		+ SHeaderRow::Column("Calls").FixedWidth(60).DefaultLabel(TAttribute<FText>::Create([=]() {
			return FText::AsNumber(TINode->Count);
		}))
		
	];
}

void SELuaMonitorPanel::OnGetChildrenRaw(TSharedPtr<FELuaTraceInfoNode> TINode, TArray<TSharedPtr<FELuaTraceInfoNode>>& OutChildren)
{
	if (TINode)
	{
		OutChildren = TINode->Children;
	}
}

void SELuaMonitorPanel::UpdateShowingRoot()
{
	CurTIRoot = FELuaMonitor::GetInstance()->GetRoot();
	if (CurTIRoot)
	{
		ShowingNodeList = CurTIRoot->Children;
	}
	else
	{
		ShowingNodeList = {};
	}
}

void SELuaMonitorPanel::DeferredTick(float DeltaTime)
{
	FELuaMonitor::GetInstance()->Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	if (MonitorMode == PerFrame || ElapsedTime > UPDATE_INTERVAL)
	{
		UpdateShowingRoot();

		if (TreeViewWidget.IsValid())
		{
			//TreeViewWidget->RebuildList();

			TreeViewWidget->RequestTreeRefresh();
		}

		ElapsedTime = 0.f;
	}
}

FReply SELuaMonitorPanel::OnForwardBtnClicked()
{
	FELuaMonitor::GetInstance()->OnForward();
	return FReply::Handled();
}

FReply SELuaMonitorPanel::OnClearBtnClicked()
{
	FELuaMonitor::GetInstance()->OnClear();
	return FReply::Handled();
}

FReply SELuaMonitorPanel::OnPrevFrameBtnClicked()
{
	FELuaMonitor::GetInstance()->SetCurFrameIndex(FELuaMonitor::GetInstance()->GetCurFrameIndex() - 1);
	return FReply::Handled();
}

FReply SELuaMonitorPanel::OnNextFrameBtnClicked()
{
	FELuaMonitor::GetInstance()->SetCurFrameIndex(FELuaMonitor::GetInstance()->GetCurFrameIndex() + 1);
	return FReply::Handled();
}

const FSlateBrush* SELuaMonitorPanel::GetPrevFrameIcon() const
{
	return  &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Animation.Backward_Step").Normal;
}

const FSlateBrush* SELuaMonitorPanel::GetForwardIcon() const
{
	if (FELuaMonitor::GetInstance()->IsRuning())
	{
		return &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Animation.Pause").Normal;
	}
	return  &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Animation.Forward").Normal;
}

const FSlateBrush* SELuaMonitorPanel::GetNextFrameIcon() const
{
	return  &FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Animation.Forward_Step").Normal;
}

void SELuaMonitorPanel::OnModeChanged(float InMode)
{
	if (InMode > -1 && InMode < ((uint8)ELuaMonitorMode::MAX))
	{
		if (!FMath::IsNearlyEqual((float)MonitorMode, InMode))
		{
			MonitorMode = (ELuaMonitorMode)((uint8)InMode);
			FELuaMonitor::GetInstance()->SetMonitorMode(MonitorMode);
			switch (MonitorMode)
			{
			case PerFrame:
				OnGenerateFrameController();
				break;
			case Statistics:
				OnRemoveFrameController();
				break;
			case Total:
			default:
				OnRemoveFrameController();
				break;
			}
		}
	}
}

void SELuaMonitorPanel::OnGenerateFrameController()
{
	if (MonitorMode == PerFrame)
	{
		if (!PrevFrameBtn)
		{
			SAssignNew(PrevFrameBtn, SButton)
			.ButtonStyle(FEditorStyle::Get(), "NoBorder")																// 无底图以免按钮发白
			.ContentPadding(2.0)
			.IsFocusable(false)
			.OnClicked(this, &SELuaMonitorPanel::OnPrevFrameBtnClicked)
			[
				SNew(SImage)
				.Image(this, &SELuaMonitorPanel::GetPrevFrameIcon)
			];
		}

		if (!NextFrameBtn)
		{
			SAssignNew(NextFrameBtn, SButton)
			.ButtonStyle(FEditorStyle::Get(), "NoBorder")																// 无底图以免按钮发白
			.ContentPadding(2.0)
			.IsFocusable(false)
			.OnClicked(this, &SELuaMonitorPanel::OnNextFrameBtnClicked)
			[
				SNew(SImage)
				.Image(this, &SELuaMonitorPanel::GetNextFrameIcon)
			];
		}

		if (!CurFrameSpin)
		{
			SAssignNew(CurFrameSpin, SSpinBox<int32>)
			.Delta(1)
			.MinValue(0)
			.MaxValue(0x7FFFFFFF)
			.Value(this, &SELuaMonitorPanel::OnGetCurFrameIndex)
			.OnValueChanged(this, &SELuaMonitorPanel::OnCurFrameIndexChanged);
		}

		if (!TotalFrameText)
		{
			SAssignNew(TotalFrameText, STextBlock)
			.Text(this, &SELuaMonitorPanel::OnGetTotalFrameText);
		}

		if (ControllerBar.IsValid())
		{
			ControllerBar->InsertSlot(2).HAlign(HAlign_Center).VAlign(VAlign_Center).AutoWidth()
			[
				PrevFrameBtn.ToSharedRef()
			];
			ControllerBar->InsertSlot(4).HAlign(HAlign_Center).VAlign(VAlign_Center).AutoWidth()
			[
				NextFrameBtn.ToSharedRef()
			];
			ControllerBar->InsertSlot(6).HAlign(HAlign_Right).VAlign(VAlign_Center).AutoWidth()
			[
				CurFrameSpin.ToSharedRef()
			];
			ControllerBar->InsertSlot(7).HAlign(HAlign_Right).VAlign(VAlign_Center).AutoWidth()
			[
				TotalFrameText.ToSharedRef()
			];
		}
	}
}

void SELuaMonitorPanel::OnRemoveFrameController()
{
	if (PrevFrameBtn && NextFrameBtn && MonitorMode != PerFrame)
	{
		if (ControllerBar.IsValid())
		{
			ControllerBar->RemoveSlot(PrevFrameBtn.ToSharedRef());
			ControllerBar->RemoveSlot(NextFrameBtn.ToSharedRef());
			ControllerBar->RemoveSlot(CurFrameSpin.ToSharedRef());
			ControllerBar->RemoveSlot(TotalFrameText.ToSharedRef());
		}
	}
}

int32 SELuaMonitorPanel::OnGetMaxDepth() const
{
	return FELuaMonitor::GetInstance()->GetMaxDepth();
}

void SELuaMonitorPanel::OnMaxDepthChanged(int32 Depth)
{
	FELuaMonitor::GetInstance()->SetMaxDepth(Depth);
}

FText SELuaMonitorPanel::OnGetTotalFrameText() const
{
	return FText::FromString(FString::Printf(TEXT("/%d"), FELuaMonitor::GetInstance()->GetTotalFrames()));
}

int32 SELuaMonitorPanel::OnGetCurFrameIndex() const
{
	return FELuaMonitor::GetInstance()->GetCurFrameIndex();
}

void SELuaMonitorPanel::OnCurFrameIndexChanged(int32 Index)
{
	FELuaMonitor::GetInstance()->SetCurFrameIndex(Index);
}

void SELuaMonitorPanel::OnCloseTab(TSharedRef<SDockTab> Tab)
{
	TabIsOpening = false;
}

#undef LOCTEXT_NAMESPACE
