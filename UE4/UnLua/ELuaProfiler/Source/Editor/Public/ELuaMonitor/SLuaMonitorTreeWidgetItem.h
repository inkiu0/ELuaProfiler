// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELuaTraceInfoNode.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"

/**
 * Widget that visualizes the contents of a FReflectorNode.
 */
class SLuaMonitorTreeWidgetItem
	: public SMultiColumnTableRow<TSharedRef<FELuaTraceInfoNode>>
{
public:

	static FName NAME_NodeName;
	static FName NAME_TotalTimeMs;
	static FName NAME_TotalTimePct;
	static FName NAME_SelfTimeMs;
	static FName NAME_SelfTimePct;
	static FName NAME_AverageMs;
	static FName NAME_AllocKb;
	static FName NAME_AllocPct;
	static FName NAME_GCKb;
	static FName NAME_GCPct;
	static FName NAME_Calls;
	
	SLATE_BEGIN_ARGS(SLuaMonitorTreeWidgetItem)
		: _InfoToVisualize()
	{ }

		SLATE_ARGUMENT(TSharedPtr<FELuaTraceInfoNode>, InfoToVisualize)
	SLATE_END_ARGS()

public:

	/**
	 * Construct child widgets that comprise this widget.
	 *
	 * @param InArgs Declaration from which to construct this widget.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

public:

	// SMultiColumnTableRow overrides
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

protected:


private:

	/** The info about the widget that we are visualizing. */
	TSharedPtr<FELuaTraceInfoNode> Info;

};
