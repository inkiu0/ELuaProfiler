// Copyright Epic Games, Inc. All Rights Reserved.

#include "SLuaMonitorTreeWidgetItem.h"
#include "SlateOptMacros.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SHyperlink.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Input/SCheckBox.h"

#define LOCTEXT_NAMESPACE "SWidgetReflector"

/* SMultiColumnTableRow overrides
 *****************************************************************************/

FName SLuaMonitorTreeWidgetItem::NAME_NodeName(TEXT("Name"));
FName SLuaMonitorTreeWidgetItem::NAME_TotalTimeMs(TEXT("TotalTime(ms)"));
FName SLuaMonitorTreeWidgetItem::NAME_TotalTimePct(TEXT("TotalTime(%)"));
FName SLuaMonitorTreeWidgetItem::NAME_SelfTimeMs(TEXT("SelfTime(ms)"));
FName SLuaMonitorTreeWidgetItem::NAME_SelfTimePct(TEXT("SelfTime(%)"));
FName SLuaMonitorTreeWidgetItem::NAME_AverageMs(TEXT("Average(ms)"));
FName SLuaMonitorTreeWidgetItem::NAME_AllocKb(TEXT("Alloc(kb)"));
FName SLuaMonitorTreeWidgetItem::NAME_AllocPct(TEXT("Alloc(%)"));
FName SLuaMonitorTreeWidgetItem::NAME_GCKb(TEXT("GC(kb)"));
FName SLuaMonitorTreeWidgetItem::NAME_GCPct(TEXT("GC(%)"));
FName SLuaMonitorTreeWidgetItem::NAME_Calls(TEXT("Calls"));

void SLuaMonitorTreeWidgetItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	this->Info = InArgs._InfoToVisualize;
	this->SetPadding(0);

	check(Info.IsValid());
	SMultiColumnTableRow< TSharedRef<FELuaTraceInfoNode> >::Construct(SMultiColumnTableRow< TSharedRef<FELuaTraceInfoNode> >::FArguments().Padding(0), InOwnerTableView);
}

TSharedRef<SWidget> SLuaMonitorTreeWidgetItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == NAME_NodeName )
	{
		return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SExpanderArrow, SharedThis(this))
			.IndentAmount(20)
			.ShouldDrawWires(true)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(this->Info->ID))
		];
	}
	else if (ColumnName == NAME_TotalTimeMs )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
            return FText::AsNumber(this->Info->TotalTime);
        }));
	}
	else if (ColumnName == NAME_TotalTimePct )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
			if (this->Info->Parent && this->Info->Parent->GCSize > 0)
			{
				double d = this->Info->TotalTime / this->Info->Parent->TotalTime;
				return FText::AsPercent(d);
			}
			return FText::AsPercent(0.f);
        }));
	}
	else if (ColumnName == NAME_SelfTimeMs )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
            return FText::AsNumber(this->Info->SelfTime);
        }));
	}
	else if (ColumnName == NAME_SelfTimePct )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
			if (this->Info->Parent && this->Info->Parent->GCSize > 0)
			{
				double d = this->Info->SelfTime / this->Info->TotalTime;
				return FText::AsPercent(d);
			}
			return FText::AsPercent(0.f);
        }));
	}
	else if (ColumnName == NAME_AverageMs )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
            return FText::AsNumber(this->Info->Average);
        }));
	}
	else if (ColumnName == NAME_AllocKb )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
            return FText::AsNumber(this->Info->AllocSize);
        }));
	}
	else if (ColumnName == NAME_AllocPct )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
			if (this->Info->Parent && this->Info->Parent->GCSize > 0)
			{
				float p = this->Info->AllocSize / this->Info->Parent->AllocSize;
				return FText::AsPercent(p);
			}
			return FText::AsPercent(0.f);
        }));
	}
	else if (ColumnName == NAME_GCKb )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
            return FText::AsNumber(this->Info->GCSize);
        }));
	}
	else if (ColumnName == NAME_GCPct )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
			if (this->Info->Parent && this->Info->Parent->GCSize > 0)
			{
				float p = this->Info->GCSize / this->Info->Parent->GCSize;
				return FText::AsPercent(p);
			}
			return FText::AsPercent(0.f);
        }));
	}
	else if (ColumnName == NAME_Calls )
	{
		return 	SNew(STextBlock)
			.Text(TAttribute<FText>::Create([=]() {
            return (FText::AsNumber(this->Info->Count));
        }));
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

#undef LOCTEXT_NAMESPACE