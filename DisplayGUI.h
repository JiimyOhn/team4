﻿//---------------------------------------------------------------------------

#ifndef DisplayGUIH
#define DisplayGUIH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "Components\OpenGLv0.5BDS2006\Component\OpenGLPanel.h"
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <Graphics.hpp>
#include "FilesystemStorage.h"
#include "KeyholeConnection.h"
#include "GoogleLayer.h"
#include "FlatEarthView.h"
#include "ght_hash_table.h"
#include "TriangulatPoly.h"
#include <Dialogs.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include "cspin.h"
#include <vector>
#include "ntds2d.h"
#include "TCPDataHandler.h"
#include "AircraftDataModel.h"
#include "IMAPProvider.h"
#include <Vcl.ExtCtrls.hpp>
#include "AircraftApi.h"
#include "AircraftInfo.h"
#include "ButtonScroller.h"
typedef float T_GL_Color[4];


typedef struct
{
 bool Valid_CC;
 bool Valid_CPA;
 uint32_t ICAO_CC;
 uint32_t ICAO_CPA;
}TTrackHook;

typedef struct
{
 double lat;
 double lon;
 double hae;
}TPolyLine;


#define MAX_AREA_POINTS 500
typedef struct
{
 AnsiString  Name;
 TColor      Color;
 DWORD       NumPoints;
 pfVec3      Points[MAX_AREA_POINTS];
 pfVec3      PointsAdj[MAX_AREA_POINTS];
 TTriangles *Triangles;
 bool        Selected;
}TArea;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TMainMenu *MainMenu1;
	TPanel *RightPanel;
	TMenuItem *File1;
	TMenuItem *Exit1;
	TTimer *Timer1;
	TOpenGLPanel *ObjectDisplay;
	TPanel *Panel1;
	TPanel *Panel3;
	TButton *ZoomIn;
	TButton *ZoomOut;
	TCheckBox *DrawMap;
	TCheckBox *PurgeStale;
	TTimer *Timer2;
	TCSpinEdit *CSpinStaleTime;
	TButton *PurgeButton;
	TListView *AreaListView;
	TButton *Insert;
	TButton *Delete;
	TButton *Complete;
	TButton *Cancel;
	TButton *RawConnectButton;
	TLabel *Label16;
	TSaveDialog *RecordRawSaveDialog;
	TOpenDialog *PlaybackRawDialog;
	TCheckBox *CycleImages;
	TPanel *Panel4;
	TLabel *CLatLabel;
	TLabel *CLonLabel;
	TLabel *SpdLabel;
	TLabel *HdgLabel;
	TLabel *AltLabel;
	TLabel *MsgCntLabel;
	TLabel *TrkLastUpdateTimeLabel;
	TLabel *Label14;
	TLabel *Label13;
	TLabel *Label10;
	TLabel *Label9;
	TLabel *Label8;
	TLabel *Label7;
	TLabel *Label6;
	TLabel *Label18;
	TLabel *FlightNumLabel;
	TLabel *ICAOLabel;
	TLabel *Label5;
	TLabel *Label4;
	TPanel *Panel5;
	TLabel *Lon;
	TLabel *Label3;
	TLabel *Lat;
	TLabel *Label2;
	TStaticText *SystemTime;
	TLabel *SystemTimeLabel;
	TLabel *ViewableAircraftCountLabel;
	TLabel *AircraftCountLabel;
	TLabel *Label11;
	TLabel *Label1;
	TEdit *RawIpAddress;
	TButton *RawPlaybackButton;
	TButton *RawRecordButton;
	TButton *SBSConnectButton;
	TEdit *SBSIpAddress;
	TButton *SBSRecordButton;
	TButton *SBSPlaybackButton;
	TSaveDialog *RecordSBSSaveDialog;
	TOpenDialog *PlaybackSBSDialog;
	TTrackBar *TimeToGoTrackBar;
	TCheckBox *TimeToGoCheckBox;
	TStaticText *TimeToGoText;
	TLabel *Label12;
	TLabel *Label19;
	TLabel *CpaTimeValue;
	TLabel *CpaDistanceValue;
	TPanel *Panel2;
	TComboBox *MapComboBox;
	TCheckBox *BigQueryCheckBox;
	TMenuItem *UseSBSLocal;
	TMenuItem *UseSBSRemote;
	TMenuItem *LoadARTCCBoundaries1;
	TLabel *Label20;
	TPanel *Panel6;
	TLabel *Label21;
	TLabel *Label22;
	TLabel *Label23;
	TLabel *Label24;
	TLabel *RawConnectStatus;
	TLabel *SBSConnectStatus;
	TLabel *APILastUpdateTime;
	TMemo *RouteInfoMemo;
	TTrackBar *PlaybackSpeedTrackBar; // UI for playback speed
	TLabel *PlaybackSpeedLabel;
	TComboBox *PlaybackSpeedComboBox; // ComboBox for playback speed
	void __fastcall ApiCallTimerTimer(TObject *Sender);
	void __fastcall ObjectDisplayInit(TObject *Sender);
	void __fastcall ObjectDisplayResize(TObject *Sender);
	void __fastcall ObjectDisplayPaint(TObject *Sender);
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall ResetXYOffset(void);
	void __fastcall ObjectDisplayMouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, int X, int Y);
	void __fastcall ObjectDisplayMouseMove(TObject *Sender, TShiftState Shift,
		  int X, int Y);
	void __fastcall AddPoint(int X, int Y);	  
	void __fastcall ObjectDisplayMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall Exit1Click(TObject *Sender);
	void __fastcall ZoomInClick(TObject *Sender);
	void __fastcall ZoomOutClick(TObject *Sender);
	void __fastcall Timer2Timer(TObject *Sender);
	void __fastcall PurgeButtonClick(TObject *Sender);
	void __fastcall InsertClick(TObject *Sender);
	void __fastcall CancelClick(TObject *Sender);
	void __fastcall CompleteClick(TObject *Sender);
	void __fastcall AreaListViewSelectItem(TObject *Sender, TListItem *Item,
          bool Selected);
	void __fastcall DeleteClick(TObject *Sender);
	void __fastcall AreaListViewCustomDrawItem(TCustomListView *Sender,
          TListItem *Item, TCustomDrawState State, bool &DefaultDraw);
	void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, TPoint &MousePos, bool &Handled);
	void __fastcall RawConnectButtonClick(TObject *Sender);
	void __fastcall RawRecordButtonClick(TObject *Sender);
	void __fastcall RawPlaybackButtonClick(TObject *Sender);
	void __fastcall CycleImagesClick(TObject *Sender);
	void __fastcall SBSConnectButtonClick(TObject *Sender);
	void __fastcall SBSRecordButtonClick(TObject *Sender);
	void __fastcall SBSPlaybackButtonClick(TObject *Sender);
	void __fastcall TimeToGoTrackBarChange(TObject *Sender);
	void __fastcall MapComboBoxChange(TObject *Sender);
	void __fastcall BigQueryCheckBoxClick(TObject *Sender);
	void __fastcall UseSBSRemoteClick(TObject *Sender);
	void __fastcall UseSBSLocalClick(TObject *Sender);
	void __fastcall LoadARTCCBoundaries1Click(TObject *Sender);

private:	// User declarations
	TCPDataHandler *FRawDataHandler;
    TCPDataHandler *FSBSDataHandler;
	AircraftDataModel *FAircraftModel;
	TButtonScroller *FRawButtonScroller;
    TButtonScroller *FSBSButtonScroller;
	
	TTimer *ApiCallTimer;

    // 콜백에 의해 호출될 함수들
    void __fastcall HandleRawData(const AnsiString& data);
    void __fastcall HandleRawConnected();
    void __fastcall HandleRawDisconnected(const String& reason);
	void __fastcall HandleRawReconnecting();

    void __fastcall HandleSBSData(const AnsiString& data);
    void __fastcall HandleSBSConnected();
    void __fastcall HandleSBSDisconnected(const String& reason);
	void __fastcall HandleSBSReconnecting();


public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
	__fastcall ~TForm1();
	void __fastcall LatLon2XY(double lat,double lon, double &x, double &y);
	int __fastcall  XY2LatLon2(int x, int y,double &lat,double &lon );
	void __fastcall HookTrack(int X, int Y,bool CPA_Hook);
	void __fastcall DrawObjects(void);
	void __fastcall DrawAirportMarkers(void);
	void __fastcall DeleteAllAreas(void);
	void __fastcall Purge(void);
	void __fastcall SendCotMessage(AnsiString IP_address, unsigned short Port,char *Buffer,DWORD Length);
	void __fastcall RegisterWithCoTRouter(void);
    void __fastcall SetMapCenter(double &x, double &y);
    void __fastcall LoadMap(int Type);
    void __fastcall CreateBigQueryCSV(void);
    void __fastcall CloseBigQueryCSV(void);
    bool __fastcall LoadARTCCBoundaries(AnsiString FileName);
	void UpdateCloseControlPanel(TADS_B_Aircraft* ac, const RouteInfo* route);
    void OnAircraftSelected(uint32_t icao);


	int                        MouseDownX,MouseDownY;
	bool                       MouseDown;
	TTrackHook                 TrackHook;
	Vector3d                   Map_v[4],Map_p[4];
	Vector2d                   Map_w[2];
	double                     Mw1,Mw2,Mh1,Mh2,xf,yf;
	KeyholeConnection	      *g_Keyhole;
	FilesystemStorage	      *g_Storage;
	std::unique_ptr<IMAPProvider> provider;
	MasterLayer	      	      *g_MasterLayer;
	TileManager		          *g_GETileManager;
	EarthView		          *g_EarthView;
	double                     MapCenterLat,MapCenterLon;
	int			               g_MouseLeftDownX;
	int			               g_MouseLeftDownY;
	int			               g_MouseDownMask ;
	bool                       LoadMapFromInternet;
	TList                     *Areas;
	TArea                     *AreaTemp;
	ght_hash_table_t          *HashTable;
	TStreamWriter              *RecordRawStream;
	TStreamReader              *PlayBackRawStream;
    TStreamWriter              *RecordSBSStream;
	TStreamReader              *PlayBackSBSStream;
	TStreamWriter              *BigQueryCSV;
    AnsiString                 BigQueryCSVFileName;
	unsigned int               BigQueryRowCount;
	unsigned int               BigQueryFileCount;
    AnsiString                 BigQueryPythonScript;
	AnsiString                 BigQueryPath;
    AnsiString                 BigQueryLogFileName;
	int                        NumSpriteImages;
	int                        CurrentSpriteImage;
    AnsiString                 AircraftDBPathFileName;
	AnsiString                 ARTCCBoundaryDataPathFileName;

        std::vector<AirplaneInstance> m_planeBatch;
        std::vector<AirplaneLineInstance> m_lineBatch;
        std::vector<HexCharInstance> m_textBatch;

        // --- Playback Speed UI 함수 선언 추가 ---
    void SetupPlaybackSpeedUI();
    void __fastcall PlaybackSpeedTrackBarChange(TObject *Sender);
    void __fastcall PlaybackSpeedComboBoxChange(TObject *Sender);
};
// --- [공항 마커(Atlas 아이콘) 관련 선언 추가] ---

// 공항 타입(민간, 군용, 헬기장)
enum AirportType { Civil = 0, Military = 1, Helipad = 2 };

// Atlas PNG 내에서 아이콘의 위치/크기 정보
struct AtlasRect { int x, y, w, h; };

// Atlas 아이콘 배열(정의는 cpp에서!)
extern const int iconW, iconH, atlasW, atlasH;
extern AtlasRect airportAtlasRects[3];

// 텍스처 핸들(정의는 cpp에서!)
extern GLuint atlasTexId;

// 공항 타입 분류 함수 선언
AirportType GetAirportType(const AirportInfo& airport);

// 아이콘 그리기 함수 선언
void DrawAtlasIcon(double x, double y, const AtlasRect& rect, GLuint atlasTexId, int iconDrawSize = 32);

//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------


#endif
