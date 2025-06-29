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
#include <Vcl.Dialogs.hpp>
#include "cspin.h"
#include <set>
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
#include <utility>
#include "ntds2d.h"
#include "TCPDataHandler.h"
#include "AircraftDataModel.h"
#include "IMAPProvider.h"
#include <Vcl.ExtCtrls.hpp>
#include "AircraftApi.h"
#include "AircraftInfo.h"
#include "ButtonScroller.h"
#include "ProximityAssessor.h"
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
        TLabel *AirlineNameLabel;
        TLabel *AirlineCountryLabel;
		TLabel *LabelModel;
        TLabel *AircraftModelLabel;
        TLabel *LabelAirlineName;
        TLabel *LabelAirlineCountry;
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
	TEdit *FilterOriginEdit;
	TEdit *FilterAirlineEdit;
	TEdit *FilterDestinationEdit;
	TLabel *Label25;
	TLabel *Label26;
	TLabel *Label27;
	TLabel *Label28;
	TTimer *AssessmentTimer;
	TListView *ConflictListView;
	TCheckBox *FilterPolygonOnlyCheckBox; // 다각형 내 비행기만 표시하는 체크박스
	TCheckBox *FilterWaypointsOnlyCheckBox; // 정의된 경유지 내 비행기만 표시하는 체크박스
	// --- 속도/고도 필터 UI ---
    TTrackBar *SpeedMinTrackBar;
    TTrackBar *SpeedMaxTrackBar;
    TTrackBar *AltitudeMinTrackBar;
    TTrackBar *AltitudeMaxTrackBar;
    TLabel *SpeedFilterLabel;
    TLabel *AltitudeFilterLabel;
    void __fastcall SpeedFilterTrackBarChange(TObject *Sender);
    void __fastcall AltitudeFilterTrackBarChange(TObject *Sender);
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
	void __fastcall ObjectDisplayDblClick(TObject *Sender);
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
	void __fastcall FilterAirlineEditChange(TObject *Sender);
	void __fastcall FilterOriginEditChange(TObject *Sender);
	void __fastcall FilterDestinationEditChange(TObject *Sender);
	void __fastcall AssessmentTimerTimer(TObject *Sender);
	void __fastcall OnAssessmentComplete(TObject *Sender);
	void __fastcall FilterPolygonOnlyCheckBoxClick(TObject *Sender); // 다각형 필터 체크박스 이벤트
	void __fastcall FilterWaypointsOnlyCheckBoxClick(TObject *Sender); // 경유지 필터 체크박스 이벤트

private:	// User declarations
	TCPDataHandler *FRawDataHandler;
    TCPDataHandler *FSBSDataHandler;
	AircraftDataModel *FAircraftModel;
	TButtonScroller *FRawButtonScroller;
    TButtonScroller *FSBSButtonScroller;
	AnsiString filterAirline;
    AnsiString filterOrigin;
    AnsiString filterDestination;
	ProximityAssessor* FProximityAssessor;
	std::map<unsigned int, std::vector<RelatedConflictInfo>> FConflictMap;
	std::vector<ConflictPair> FSortedConflictList;
	std::pair<unsigned int, unsigned int> FSelectedConflictPair;
	bool filterPolygonOnly; // 다각형 내 비행기만 표시하는 필터
	bool filterWaypointsOnly; // 정의된 경유지 내 비행기만 표시하는 필터
	
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

	void __fastcall UpdateConflictList();
    void __fastcall ConflictListViewSelectItem(TObject *Sender, TListItem *Item, bool Selected);
	void __fastcall CenterMapOnPair(unsigned int icao1, unsigned int icao2);

public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
	__fastcall ~TForm1();
	void __fastcall LatLon2XY(double lat,double lon, double &x, double &y);
	int __fastcall  XY2LatLon2(int x, int y,double &lat,double &lon );
	void __fastcall HookTrack(int X, int Y,bool CPA_Hook);
	void __fastcall DrawObjects(void);
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
	bool IsRouteMatched(const RouteInfo* route) const;
	void InitRouteAirportMaps(); 


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

        // 선택된 항공기의 경로(대권) 좌표 목록
        std::vector<std::vector<std::pair<double,double>>> m_selectedRoutePaths;

        // --- Playback Speed UI 함수 선언 추가 ---
    void SetupPlaybackSpeedUI();
    void __fastcall PlaybackSpeedTrackBarChange(TObject *Sender);
    void __fastcall PlaybackSpeedComboBoxChange(TObject *Sender);
};


//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------


#endif
