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

#include "ELuaMemAnalyzerPanel.h"
#include "EditorStyleSet.h"
#include "ELuaMemAnalyzer.h"
#include "Widgets/Text/STextBlock.h"

void SELuaMemAnalyzerPanel::Construct(const SELuaMemAnalyzerPanel::FArguments& InArgs)
{

}

TSharedRef<class SDockTab> SELuaMemAnalyzerPanel::GetSDockTab()
{
	TabIsOpening = true;

	TSharedPtr<SDockTab> Tab;
	SAssignNew(Tab, SDockTab)
	.Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
	.Label(FText::FromName("ELuaMemAnalyzer"))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).HAlign(HAlign_Center)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(2.0)
					.IsFocusable(false)
					.OnClicked(this, &SELuaMemAnalyzerPanel::OnSnapshotBtnClicked)
					[
						SNew(STextBlock)
						.Text(FText::FromName("Snapshot"))
					]
				]

				+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(2.0)
					.IsFocusable(false)
					.OnClicked(this, &SELuaMemAnalyzerPanel::OnGCBtnClicked)
					[
						SNew(STextBlock)
						.Text(FText::FromName("GC"))
					]
				]
			]
		]
	];
	Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &SELuaMemAnalyzerPanel::OnCloseTab));
	return Tab.ToSharedRef();
}

void SELuaMemAnalyzerPanel::DeferredTick(float DeltaTime)
{

}

void SELuaMemAnalyzerPanel::OnCloseTab(TSharedRef<SDockTab> Tab)
{
	TabIsOpening = false;
}

FReply SELuaMemAnalyzerPanel::OnSnapshotBtnClicked()
{
	FELuaMemAnalyzer::GetInstance()->Snapshot();
	return FReply::Handled();
}

FReply SELuaMemAnalyzerPanel::OnGCBtnClicked()
{
	FELuaMemAnalyzer::GetInstance()->ForceLuaGC();
	return FReply::Handled();
}
