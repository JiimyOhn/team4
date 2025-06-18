//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include "AirportProvider.h"
//---------------------------------------------------------------------------
USEFORM("NetworkConnectionStatusGUI.cpp", NetworkConnectionStatusGUI);
USEFORM("DisplayGUI.cpp", Form1);
USEFORM("AreaDialog.cpp", AreaConfirm);
//---------------------------------------------------------------------------
static FILE* pCout = NULL;
static void SetStdOutToNewConsole(void);
//---------------------------------------------------------------------------
static void SetStdOutToNewConsole(void)
{
    // Allocate a console for this app
    AllocConsole();
    //AttachConsole(ATTACH_PARENT_PROCESS);
	freopen_s(&pCout, "CONOUT$", "w", stdout);
}

//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
#if 0
	// Test
	AirportProvider provider;
	provider.DownloadAndParseAirportsCSV(
		[](const std::vector<AirportInfo>& batch, bool finished) {
			if (!batch.empty()) {
				// batch 처리
				printf("받음!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
			if (finished) {
				// 전체 완료 처리
			}
		}
	);
#endif
	try
	{
        SetStdOutToNewConsole();
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->CreateForm(__classid(TForm1), &Form1);
		Application->CreateForm(__classid(TAreaConfirm), &AreaConfirm);
		Application->CreateForm(__classid(TNetworkConnectionStatusGUI), &NetworkConnectionStatusGUI);
		Application->Run();
	   if (pCout)
		{
		 fclose(pCout);
		 FreeConsole();
	    }
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
