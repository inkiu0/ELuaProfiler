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

#include "ELuaStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FELuaStyle::StyleInst = NULL;

void FELuaStyle::Initialize()
{
	if (!StyleInst.IsValid())
	{
		StyleInst = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInst);
	}
}

void FELuaStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInst);
	ensure(StyleInst.IsUnique());
	StyleInst.Reset();
}

FName FELuaStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ELuaStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FELuaStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("ELuaStyle"));
	Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate/Common"));

	Style->Set("Black", new BOX_BRUSH(TEXT("Common/FlatButton"), 2.0f / 8.0f, FLinearColor(0.125f, 0.125f, 0.125f, 0.8f)));
	Style->Set("Yellow", new BOX_BRUSH(TEXT("Common/FlatButton"), 2.0f / 8.0f, FLinearColor(0.87514, 0.42591, 0.07383)));
	Style->Set("Blue", new BOX_BRUSH(TEXT("Common/FlatButton"), 2.0f / 8.0f, FLinearColor(0.10363, 0.53564, 0.7372)));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH

void FELuaStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FELuaStyle::Get()
{
	return *StyleInst;
}
