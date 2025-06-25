// ButtonScroller.h

#ifndef ButtonScrollerH
#define ButtonScrollerH

#include <vcl.h>
#include <Vcl.ExtCtrls.hpp>

class TButtonScroller : public TObject
{
private:
    TButton *FTargetButton;
    String FOriginalCaption;
    int FScrollPosition;

    TTimer *FScrollTimer;      // 이제 이 타이머 하나만 사용합니다.
    bool FIsInitialDelay;      // 초기 지연 상태인지를 확인하는 플래그

    void __fastcall AnimateText(TObject *Sender);

public:
    __fastcall TButtonScroller(TButton *TargetButton);
    virtual __fastcall ~TButtonScroller();

    void UpdateCaption(const String& NewCaption);
};

#endif