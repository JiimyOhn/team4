#include <vcl.h>
#pragma hdrstop

#include "Progressform.h"
#pragma package(smart_init)
#pragma resource "*.dfm"
TProgressform *Progressform;
//---------------------------------------------------------------------------
__fastcall TProgressform::TProgressform(TComponent* Owner)
	: TForm(Owner), CancelRequested(false)
{
}
//---------------------------------------------------------------------------
void TProgressform::ShowProgress(const AnsiString& msg)
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
void TProgressform::UpdateProgress(int percentage)
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
void TProgressform::HideProgress(void)
{
    if (ProgressBar1)
	{
		ProgressBar1->Visible = false;
	}
	this->Hide();
}
//---------------------------------------------------------------------------
void __fastcall TProgressform::CancelClick(TObject *Sender)
{
	printf("button clicked!\n");
	this->CancelRequested = true;
	this->Hide();
	printf("hide!\n");
}

void TProgressform::SetCaption(const AnsiString& msg)
{
	ProgressLabel->Caption = msg;
	this->Show();
	this->Update();
}
//---------------------------------------------------------------------------
