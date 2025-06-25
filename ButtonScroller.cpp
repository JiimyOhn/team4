// ButtonScroller.cpp

#pragma hdrstop
#include "ButtonScroller.h"
#include <Vcl.Graphics.hpp>
#include <memory>
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TButtonScroller::TButtonScroller(TButton *TargetButton)
    // FIsInitialDelay를 false로 초기화
    : TObject(), FTargetButton(TargetButton), FScrollTimer(nullptr), FIsInitialDelay(false), FScrollPosition(1)
{
    if (!FTargetButton) {
        throw new Exception("TargetButton cannot be null");
    }
    UpdateCaption(FTargetButton->Caption);
}

__fastcall TButtonScroller::~TButtonScroller()
{
    // 타이머가 하나이므로 정리 코드도 간결해집니다.
    if (FScrollTimer) {
        FScrollTimer->Enabled = false;
        delete FScrollTimer;
        FScrollTimer = nullptr;
    }
}

void __fastcall TButtonScroller::UpdateCaption(const String& NewCaption)
{
    if (FOriginalCaption == NewCaption)
    {
        return;
    }

    if (FScrollTimer) {
        FScrollTimer->Enabled = false;
        delete FScrollTimer;
        FScrollTimer = nullptr;
    }

    FOriginalCaption = NewCaption;
    FScrollPosition = 1;
    FIsInitialDelay = false; // 상태 플래그 초기화

    std::unique_ptr<TBitmap> tempBitmap(new TBitmap());
    tempBitmap->Canvas->Font->Assign(FTargetButton->Font);
    int textWidth = tempBitmap->Canvas->TextWidth(NewCaption);

    const int PADDING = 8;

    if (textWidth > (FTargetButton->Width - PADDING))
    {
        String initialDisplayCaption = NewCaption;
        for (int i = NewCaption.Length(); i >= 1; --i) {
             String sub = NewCaption.SubString(1, i);
             if (tempBitmap->Canvas->TextWidth(sub + "...") <= (FTargetButton->Width - PADDING)) {
                 initialDisplayCaption = sub + "...";
                 break;
             }
             if (tempBitmap->Canvas->TextWidth(sub) <= (FTargetButton->Width - PADDING)) {
                 initialDisplayCaption = sub;
                 break;
             }
        }
        FTargetButton->Caption = initialDisplayCaption;

        FIsInitialDelay = true;
        FScrollTimer = new TTimer(nullptr);
        FScrollTimer->Interval = 600; // 초기 지연 시간 설정
        FScrollTimer->OnTimer = AnimateText;
        FScrollTimer->Enabled = true;
    }
    else
    {
        FTargetButton->Caption = NewCaption;
    }
}


void __fastcall TButtonScroller::AnimateText(TObject *Sender)
{
    if (FIsInitialDelay)
    {
        FIsInitialDelay = false;
        FScrollTimer->Interval = 250;
        return;
    }

    String textToScroll = FOriginalCaption + "   ";
    if (FScrollPosition > textToScroll.Length())
    {
        FScrollPosition = 1;
    }

    String part1 = textToScroll.SubString(FScrollPosition, textToScroll.Length());
    String part2 = textToScroll.SubString(1, FScrollPosition - 1);
    FTargetButton->Caption = part1 + part2;

    FScrollPosition++;
}