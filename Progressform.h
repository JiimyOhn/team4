#ifndef ProgressformH
#define ProgressformH
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp> // TProgressBar를 위해 추가

class TProgressform : public TForm
{
__published:    // IDE-managed Components
    TLabel *ProgressLabel;
    TProgressBar *ProgressBar1;
	TButton *Cancel;
	void __fastcall CancelClick(TObject *Sender); // ProgressBar 추가
private:    // User declarations
public:        // User declarations
    __fastcall TProgressform(TComponent* Owner);
    void ShowProgress(const AnsiString& msg);
    void UpdateProgress(int percentage); // 진행률 업데이트 메소드 추가
	void HideProgress(void); // 숨김 메소드 추가 (선택 사항)
	void SetCaption(const AnsiString& msg);
	bool CancelRequested; // 취소 요청 플래그

};
extern PACKAGE TProgressform *Progressform;
#endif