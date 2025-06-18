#include <vcl.h>
#pragma hdrstop

#include "NetworkConnectionStatusGUI.h"
#pragma package(smart_init)
#pragma resource "*.dfm"
TNetworkConnectionStatusGUI *NetworkConnectionStatusGUI;
//---------------------------------------------------------------------------
__fastcall TNetworkConnectionStatusGUI::TNetworkConnectionStatusGUI(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void TNetworkConnectionStatusGUI::ShowProgress(const AnsiString& msg)
{
    ProgressLabel->Caption = msg;
    if (ProgressBar1) // ProgressBar가 생성되었는지 확인
    {
        ProgressBar1->Position = 0; // 진행률 초기화
        ProgressBar1->Visible = true;
    }
    this->Show();
	this->Update(); // 또는 Application->ProcessMessages(); 를 사용하여 UI 즉시 업데이트
}
//---------------------------------------------------------------------------
void TNetworkConnectionStatusGUI::UpdateProgress(int percentage)
{
    if (ProgressBar1)
    {
        ProgressBar1->Position = percentage;
    }
    // 필요에 따라 Application->ProcessMessages(); 호출하여 UI가 즉시 업데이트되도록 할 수 있습니다.
    // 특히 긴 작업의 루프 내에서 호출될 때 유용합니다.
    // Application->ProcessMessages();
}
//---------------------------------------------------------------------------
void TNetworkConnectionStatusGUI::HideProgress(void)
{
    if (ProgressBar1)
    {
        ProgressBar1->Visible = false;
    }
	this->Hide();
}
//---------------------------------------------------------------------------
void __fastcall TNetworkConnectionStatusGUI::CancelClick(TObject *Sender)
{
    CancelRequested = true;
    // 필요에 따라 진행률을 초기화하거나 UI를 업데이트할 수 있습니다.
    if (ProgressBar1)
    {
        ProgressBar1->Position = 0; // 진행률 초기화
    }
	this->Hide();
}
//---------------------------------------------------------------------------
AnsiString __fastcall TNetworkConnectionStatusGUI::getCancelButtonCaption(void)
{
	return this->Cancel->Caption;
}
//---------------------------------------------------------------------------
void __fastcall TNetworkConnectionStatusGUI::SetCancelButtonCaption(const AnsiString& msg)
{
	this->Cancel->Caption = msg;
	this->Update();
}
