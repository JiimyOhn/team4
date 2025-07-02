//---------------------------------------------------------------------------

#include <vcl.h>
#include <new>
#include <math.h>
#include <dir.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <fileapi.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <Vcl.ComCtrls.hpp> // For TTrackBar
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <cctype>
#include <Sysutils.hpp>

#pragma hdrstop

#include "DisplayGUI.h"
#include "AreaDialog.h"
#include "LatLonConv.h"
#include "PointInPolygon.h"
#include "ght_hash_table.h"
#include "dms.h"
#include "Aircraft.h"
#include "TimeFunctions.h"
#include "CPA.h"
#include "AircraftDB.h"
#include "csv.h"
#include "MAPFactory.h"
#include "hex_font.h"
#include "AircraftApi.h"
#include "AircraftApiThread.h"
#include "ntds2d.h"
#include <memory>
#include "LogHandler.h"

#define AIRCRAFT_DATABASE_URL   "https://opensky-network.org/datasets/metadata/aircraftDatabase.zip"
#define AIRCRAFT_DATABASE_FILE   "aircraftDatabase.csv"
#define ARTCC_BOUNDARY_FILE      "Ground_Level_ARTCC_Boundary_Data_2025-05-15.csv"

#define MAP_CENTER_LAT  40.73612;
#define MAP_CENTER_LON -80.33158;

#define BIG_QUERY_UPLOAD_COUNT 50000
#define BIG_QUERY_RUN_FILENAME  "SimpleCSVtoBigQuery.py"
#define   LEFT_MOUSE_DOWN   1
#define   RIGHT_MOUSE_DOWN  2
#define   MIDDLE_MOUSE_DOWN 4


#define BG_INTENSITY   0.37
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "OpenGLPanel"
#pragma link "Map\libgefetch\Win64\Release\libgefetch.a"
#pragma link "Map\zlib\Win64\Release\zlib.a"
#pragma link "Map\jpeg\Win64\Release\jpeg.a"
#pragma link "Map\png\Win64\Release\png.a"
#pragma link "cspin"
#pragma link "cspin"
#pragma resource "*.dfm"
TForm1 *Form1;
 //---------------------------------------------------------------------------
 static void RunPythonScript(AnsiString scriptPath,AnsiString args);
 static bool DeleteFilesWithExtension(AnsiString dirPath, AnsiString extension);
 static int FinshARTCCBoundary(void);
 //---------------------------------------------------------------------------

static char *stristr(const char *String, const char *Pattern);
static const char * strnistr(const char * pszSource, DWORD dwLength, const char * pszFind) ;

static void UpdateCloseControlPanel(TADS_B_Aircraft* ac, const RouteInfo* route);
static void OnAircraftSelected(uint32_t icao);
//static const AirportInfo* FindAirportByIcao(const std::string& icao);
static const RouteInfo* FindRouteByCallsign(const std::string& callSign);
static std::string ICAO_to_string(uint32_t icao);

static std::unordered_map<std::string, const RouteInfo*> callSignToRoute;
static std::unordered_map<std::string, const AirportInfo*> icaoToAirport;
static std::unordered_map<std::string, std::pair<std::string, std::string>> airlineInfoMap;
extern ght_hash_table_t *AircraftDBHashTable;

static std::vector<std::vector<std::pair<double,double>>> BuildRouteSegment(double lat1, double lon1,
                                                                            double lat2, double lon2)
{
    auto normLon = [](double lon) {
        while (lon > 180.0) lon -= 360.0;
        while (lon < -180.0) lon += 360.0;
        return lon;
    };

    std::vector<std::vector<std::pair<double,double>>> segments;
    std::vector<std::pair<double,double>> current;

    double dist, az1, az2;
    if (VInverse(lat1, lon1, lat2, lon2, &dist, &az1, &az2) != OKNOERROR)
        return segments;

    int n = static_cast<int>(dist / 50) + 7; // scale with distance
    if (n < 7) n = 7;              // minimum points
    if (n > 77) n = 77;            // cap to avoid excess

    double prevLat = lat1;
    double prevLon = normLon(lon1);
    current.emplace_back(prevLat, prevLon);

    for (int i = 1; i <= n; ++i) {
        double lat, lon, junk;
        if (VDirect(lat1, lon1, az1, dist * (double(i) / (double)n), &lat, &lon, &junk) != OKNOERROR)
            break;

        lon = normLon(lon);
        double diff = lon - prevLon;

        if (diff > 180.0) {
            double adjLon = lon - 360.0;
            double t = (-180.0 - prevLon) / (adjLon - prevLon);
            double latEdge = prevLat + (lat - prevLat) * t;
            current.emplace_back(latEdge, -180.0);
            segments.push_back(current);
            current.clear();
            current.emplace_back(latEdge, 180.0);
            current.emplace_back(lat, lon);
        } else if (diff < -180.0) {
            double adjLon = lon + 360.0;
            double t = (180.0 - prevLon) / (adjLon - prevLon);
            double latEdge = prevLat + (lat - prevLat) * t;
            current.emplace_back(latEdge, 180.0);
            segments.push_back(current);
            current.clear();
            current.emplace_back(latEdge, -180.0);
            current.emplace_back(lat, lon);
        } else {
            current.emplace_back(lat, lon);
        }

        prevLat = lat;
        prevLon = lon;
    }

    if (!current.empty())
        segments.push_back(current);

    return segments;
}

static std::string trim(const std::string& s)
{
    const char* ws = " \t\r\n\"";
    size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

static void LoadAirlineInfo(const AnsiString& fileName)
{
    airlineInfoMap.clear();
    std::ifstream f(fileName.c_str());
    if (!f.is_open())
    {
        printf("Failed to open %s\n", fileName.c_str());
        return;
    }
    std::string line;
    std::getline(f, line); // header
    while (std::getline(f, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string code,country,name;
        if (std::getline(ss, code, ',') &&
            std::getline(ss, country, ',') &&
            std::getline(ss, name))
        {
            auto up = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), ::toupper); return trim(s); };
            code = up(code);
            country = trim(country);
            name = trim(name);
            if (!code.empty())
                airlineInfoMap[code] = {name, country};
        }
    }
}

static bool LookupAirline(const std::string& callSign,
                          std::string& airline,
                          std::string& country)
{
    if (callSign.length() < 2) return false;

    // Try with the first 3 characters
    if (callSign.length() >= 3) {
        std::string key3 = callSign.substr(0, 3);
        auto it = airlineInfoMap.find(key3);
        if (it != airlineInfoMap.end()) {
            airline = it->second.first;
            country = it->second.second;
            return true;
        }
    }

    // If not found or callsign is shorter than 3, try with the first 2 characters
    std::string key2 = callSign.substr(0, 2);
    auto it = airlineInfoMap.find(key2);
    if (it != airlineInfoMap.end()) {
        airline = it->second.first;
        country = it->second.second;
        return true;
    }

    return false;
}
static AnsiString GetAircraftModel(uint32_t icao)
{
    const TAircraftData* data = (const TAircraftData*) ght_get(AircraftDBHashTable, sizeof(icao), &icao);
    if (data)
    {
        AnsiString model = data->Fields[AC_DB_Model].Trim();
        if (!model.IsEmpty())
            return model;
    }
    return "N/A";
}
//---------------------------------------------------------------------------
uint32_t createRGB(uint8_t r, uint8_t g, uint8_t b)
{
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
//---------------------------------------------------------------------------
uint32_t PopularColors[] = {
	  createRGB(255, 0, 0),      // Red
	  createRGB(0, 255, 0),      // Green
	  createRGB(0, 0, 255),      // Blue
	  createRGB(255, 255, 0),   // Yellow
	  createRGB(255, 165, 0),   // Orange
	  createRGB(255, 192, 203), // Pink
	  createRGB(0, 255, 255),   // Cyan
	  createRGB(255, 0, 255),  // Magenta
	  createRGB(255,255, 255),    // White
	  //createRGB(0, 0, 0),        // Black
	  createRGB(128,128,128),      // Gray
	  createRGB(165,42,42)    // Brown
  };

  int NumColors = sizeof(PopularColors) / sizeof(PopularColors[0]);
 unsigned int CurrentColor=0;


 //---------------------------------------------------------------------------
 typedef struct
{
   union{
     struct{
	 System::Byte Red;
	 System::Byte Green;
	 System::Byte Blue;
	 System::Byte Alpha;
     };
     struct{
     TColor Cl;
     };
     struct{
     COLORREF Rgb;
     };
   };

}TMultiColor;
//---------------------------------------------------------------------------
static const char * strnistr(const char * pszSource, DWORD dwLength, const char * pszFind)
{
	DWORD        dwIndex   = 0;
	DWORD        dwStrLen  = 0;
	const char * pszSubStr = NULL;

	// check for valid arguments
	if (!pszSource || !pszFind)
	{
		return pszSubStr;
	}

	dwStrLen = strlen(pszFind);

	// can pszSource possibly contain pszFind?
	if (dwStrLen > dwLength)
	{
		return pszSubStr;
	}

	while (dwIndex <= dwLength - dwStrLen)
	{
		if (0 == strnicmp(pszSource + dwIndex, pszFind, dwStrLen))
		{
			pszSubStr = pszSource + dwIndex;
			break;
		}

		dwIndex ++;
	}

	return pszSubStr;
}
//---------------------------------------------------------------------------
static char *stristr(const char *String, const char *Pattern)
{
  char *pptr, *sptr, *start;
  size_t  slen, plen;

  for (start = (char *)String,pptr  = (char *)Pattern,slen  = strlen(String),plen  = strlen(Pattern);
       slen >= plen;start++, slen--)
      {
            /* find start of pattern in string */
            while (toupper(*start) != toupper(*Pattern))
            {
                  start++;
                  slen--;

                  /* if pattern longer than string */

                  if (slen < plen)
                        return(NULL);
            }

            sptr = start;
            pptr = (char *)Pattern;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if ('\0' == *pptr)
                        return (start);
            }
      }
   return(NULL);
}
//---------------------------------------------------------------------------
// Route/Airport map 채우는 함수
void __fastcall TForm1::InitRouteAirportMaps()
{
    callSignToRoute.clear();
    for (const auto& r : apiRouteList) {
        std::string key = r.callSign;
        callSignToRoute[key] = &r;
    }
    icaoToAirport.clear();
    for (const auto& ap : apiAirportList) {
        std::string key = ap.icao;
        icaoToAirport[key] = &ap;
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
  // LogHandler 초기화
  LogHandler::Initialize();
  LogHandler::SetMinLevel(LogHandler::LOG_INFO);
  LogHandler::EnableCategory(LogHandler::CAT_GENERAL | LogHandler::CAT_PURGE | 
                            LogHandler::CAT_PROXIMITY | LogHandler::CAT_SBS | 
                            LogHandler::CAT_PERFORMANCE | LogHandler::CAT_MAP);
  LogHandler::SetConsoleOutput(true);
  LogHandler::SetFileOutput("ads_b_debug.log");
  
  LOG_INFO(LogHandler::CAT_GENERAL, "ADS-B Display application starting...");

  AircraftDBPathFileName=ExtractFilePath(ExtractFileDir(Application->ExeName)) +AnsiString("..\\AircraftDB\\")+AIRCRAFT_DATABASE_FILE;
  ARTCCBoundaryDataPathFileName=ExtractFilePath(ExtractFileDir(Application->ExeName)) +AnsiString("..\\ARTCC_Boundary_Data\\")+ARTCC_BOUNDARY_FILE;
  BigQueryPath=ExtractFilePath(ExtractFileDir(Application->ExeName)) +AnsiString("..\\BigQuery\\");
  BigQueryPythonScript= BigQueryPath+ AnsiString(BIG_QUERY_RUN_FILENAME);
  DeleteFilesWithExtension(BigQueryPath, "csv");
  BigQueryLogFileName=BigQueryPath+"BigQuery.log";
  DeleteFileA(BigQueryLogFileName.c_str());
  CurrentSpriteImage=0;
  RecordRawStream=NULL;
  PlayBackRawStream=NULL;
  TrackHook.Valid_CC=false;
  TrackHook.Valid_CPA=false;
  FRenderThread = nullptr;

  AreaTemp=NULL;
  Areas= new TList;

 MouseDown=false;

 MapCenterLat=MAP_CENTER_LAT;
 MapCenterLon=MAP_CENTER_LON;

 LoadMapFromInternet=true;
 MapComboBox->ItemIndex=GoogleMaps;
 //MapComboBox->ItemIndex=SkyVector_VFR;
 //MapComboBox->ItemIndex=SkyVector_IFR_Low;
 //MapComboBox->ItemIndex=SkyVector_IFR_High;
 LoadMap(MapComboBox->ItemIndex);

 g_EarthView->m_Eye.h /= pow(1.3,18);//pow(1.3,43);
 SetMapCenter(g_EarthView->m_Eye.x, g_EarthView->m_Eye.y);
 TimeToGoTrackBar->Position=120;
 BigQueryCSV=NULL;
 BigQueryRowCount=0;
  BigQueryFileCount=0;
  InitAircraftDB(AircraftDBPathFileName);
  {
    AnsiString airlineFile = ExtractFilePath(ExtractFileDir(Application->ExeName)) +
                             AnsiString("..\\AircraftDB\\airline_names_countries.csv");
    LoadAirlineInfo(airlineFile);
  }
  m_planeBatch.reserve(5000);
  m_lineBatch.reserve(5000);
  m_textBatch.reserve(5000 * 6);
  SetHexTextScale(1.0f);
  SetHexTextBold(true);

  	// Raw 데이터 핸들러 생성 및 콜백 연결
	FRawDataHandler = new TCPDataHandler(this);
	FRawDataHandler->OnDataReceived = [this](const AnsiString& data){ this->HandleRawData(data); };
	FRawDataHandler->OnConnected = [this](){ this->HandleRawConnected(); };
	FRawDataHandler->OnDisconnected = [this](const String& reason){ this->HandleRawDisconnected(reason); };
	FRawDataHandler->OnReconnecting = [this](){ this->HandleRawReconnecting(); };

	// SBS 데이터 핸들러 생성 및 콜백 연결
	FSBSDataHandler = new TCPDataHandler(this);
	FSBSDataHandler->OnDataReceived = [this](const AnsiString& data){ this->HandleSBSData(data); };
	FSBSDataHandler->OnConnected = [this](){ this->HandleSBSConnected(); };
	FSBSDataHandler->OnDisconnected = [this](const String& reason){ this->HandleSBSDisconnected(reason); };
	FSBSDataHandler->OnReconnecting = [this](){ this->HandleSBSReconnecting(); };

  FAircraftModel = new AircraftDataModel();
  FRawButtonScroller = new TButtonScroller(RawConnectButton);
  FSBSButtonScroller = new TButtonScroller(SBSConnectButton);
  SetupPlaybackSpeedUI();

  FProximityAssessor = new ProximityAssessor();
  FProximityAssessor->OnComplete = OnAssessmentComplete;

  AssessmentTimer->Interval = 1000;
  AssessmentTimer->Enabled = true;

  ConflictListView->OnSelectItem = ConflictListViewSelectItem;
  ConflictListView->OnCustomDrawItem = ConflictListViewCustomDrawItem;
  ConflictListView->OnCustomDrawSubItem = ConflictListViewCustomDrawSubItem;
  ConflictListView->HideSelection = false; // 포커스가 없어도 선택 상태 유지
  FSelectedConflictPair = {0, 0};
  
  // 이탈감지 리스트 이벤트 핸들러 연결
  DeviationListView->OnSelectItem = DeviationListViewSelectItem;

  // 충돌 필터 초기값 설정
  m_tcpaMinThreshold = 10.0;     // 10초로 변경
  m_tcpaMaxThreshold = 900.0;    // 900초 (15분)
  m_horizontalMinDistance = 0.0;  // 0 NM
  m_horizontalMaxDistance = 1.0;  // 1 NM로 변경
  m_verticalMinDistance = 0.0;    // 0 feet
  m_verticalMaxDistance = 1000.0; // 1000 feet로 변경
  
  // 충돌 상태 표시 초기화
  m_criticalBlinkTimer = new TTimer(this);
  m_criticalBlinkTimer->Interval = 500; // 0.5초마다 깜빡임
  m_criticalBlinkTimer->OnTimer = CriticalBlinkTimerTimer;
  m_criticalBlinkTimer->Enabled = false;
  m_criticalBlinkState = false;
  m_hasCriticalConflicts = false;
  m_hasHighConflicts = false;
  
  // 충돌 항공기 표시 옵션 초기화
  m_showConflictAircraftAlways = false;
  m_showOnlyConflictAircraft = false;
  
  // 충돌 필터 UI 초기화
  UpdateConflictFilterLabels();

  // 필터 변수 초기화
  filterPolygonOnly = false;
  filterWaypointsOnly = false;

  m_renderParams.minSpeed = SpeedMinTrackBar->Position;
  m_renderParams.maxSpeed = SpeedMaxTrackBar->Position;
  m_renderParams.minAlt   = AltitudeMinTrackBar->Position;
  m_renderParams.maxAlt   = AltitudeMaxTrackBar->Position;
  m_renderParams.airlineFilter  = !filterAirline.IsEmpty();
  m_renderParams.originFilter   = !filterOrigin.IsEmpty();
  m_renderParams.destFilter     = !filterDestination.IsEmpty();
  m_renderParams.filterAirline  = filterAirline;
  m_renderParams.filterOrigin   = filterOrigin;
  m_renderParams.filterDestination = filterDestination;
  m_renderParams.showCommercial      = CommercialCheckBox->Checked;
  m_renderParams.showCargo           = CargoCheckBox->Checked;
  m_renderParams.showHelicopter      = HelicopterCheckBox->Checked;
  m_renderParams.showMilitary        = MilitaryCheckBox->Checked;
  m_renderParams.showBusinessJet     = BusinessJetCheckBox->Checked;
  m_renderParams.showGlider          = GliderCheckBox->Checked;
  m_renderParams.showUltralight      = UltralightCheckBox->Checked;
  m_renderParams.showGeneralAviation = GeneralAviationCheckBox->Checked;
  m_renderParams.filterPolygonOnly   = filterPolygonOnly;
  m_renderParams.filterWaypointsOnly = filterWaypointsOnly;

  FRenderThread = new TAircraftRenderThread(this);
    
  // 이탈감지 리스트 초기화
  UpdateDeviationList();

  LOG_INFO(LogHandler::CAT_GENERAL, "Initialization complete");
}

void __fastcall TForm1::ApiCallTimerTimer(TObject *Sender)
{
    try {
        new TLoadApiDataThread();
        LOG_INFO(LogHandler::CAT_NETWORK, "API data loading thread started successfully");
    } catch (const std::exception& e) {
        LOG_ERROR_F(LogHandler::CAT_NETWORK, "API call exception: %s", e.what());
    } catch (...) {
        LOG_ERROR(LogHandler::CAT_NETWORK, "Unknown API call exception");
    }
}

//---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
{
 Timer1->Enabled=false;
 Timer2->Enabled=false;
 AssessmentTimer->Enabled=false;
 delete g_EarthView;
 if (g_GETileManager) delete g_GETileManager;
 delete g_MasterLayer;
 delete g_Storage;
 provider.reset();
 if (LoadMapFromInternet)
 {
   if (g_Keyhole) delete g_Keyhole;
 }

  delete FRawDataHandler;
  delete FSBSDataHandler;
  delete FRawButtonScroller;
  delete FSBSButtonScroller;
  delete FAircraftModel;
  delete FProximityAssessor;
  if (FRenderThread) { FRenderThread->Terminate(); FRenderThread->WaitFor(); delete FRenderThread; }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SetMapCenter(double &x, double &y)
{
  double siny;
  x=(MapCenterLon+0.0)/360.0;
  siny=sin((MapCenterLat * M_PI) / 180.0);
  siny = fmin(fmax(siny, -0.9999), 0.9999);
  y=(log((1 + siny) / (1 - siny)) / (4 * M_PI));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayInit(TObject *Sender)
{
	glViewport(0,0,(GLsizei)ObjectDisplay->Width,(GLsizei)ObjectDisplay->Height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable (GL_LINE_STIPPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    NumSpriteImages = MakeAirplaneImages();
    MakeAirportImages();
	MakeAirTrackFriend();
	MakeAirTrackHostile();
	MakeAirTrackUnknown();
	MakePoint();
	MakeTrackHook();
	InitAirplaneInstancing();
	InitAirplaneLinesInstancing();
	InitHexTextInstancing();
	if(g_EarthView)
		g_EarthView->Resize(ObjectDisplay->Width,ObjectDisplay->Height);
	glPushAttrib (GL_LINE_BIT);
	glPopAttrib ();
    LOG_INFO_F(LogHandler::CAT_GENERAL, "OpenGL Version: %s", glGetString(GL_VERSION));
	 // API 로딩 비동기 호출
    new TLoadApiDataThread();

    // 1시간마다 주기 호출 설정
    ApiCallTimer = new TTimer(this);
    ApiCallTimer->Interval = 3600000; // 1시간
    ApiCallTimer->OnTimer = ApiCallTimerTimer;
    ApiCallTimer->Enabled = true;

    InitRouteAirportMaps();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ObjectDisplayResize(TObject *Sender)
{
	 double Value;
	//ObjectDisplay->Width=ObjectDisplay->Height;
	glViewport(0,0,(GLsizei)ObjectDisplay->Width,(GLsizei)ObjectDisplay->Height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable (GL_LINE_STIPPLE);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	if(g_EarthView)
		g_EarthView->Resize(ObjectDisplay->Width,ObjectDisplay->Height);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayPaint(TObject *Sender)
{

 if (DrawMap->Checked)glClearColor(0.0,0.0,0.0,0.0);
 else	glClearColor(BG_INTENSITY,BG_INTENSITY,BG_INTENSITY,0.0);

 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 if (g_EarthView)
 {
	g_EarthView->Animate();
	g_EarthView->Render(DrawMap->Checked);
 }
 if( g_GETileManager)
  g_GETileManager->Cleanup();
 Mw1 = Map_w[1].x-Map_w[0].x;
 Mw2 = Map_v[1].x-Map_v[0].x;
 Mh1 = Map_w[1].y-Map_w[0].y;
 Mh2 = Map_v[3].y-Map_v[0].y;

 xf=Mw1/Mw2;
 yf=Mh1/Mh2;
 #ifdef MEASURE_PERFORMANCE
 auto start = std::chrono::steady_clock::now();
 #endif
 DrawAirportMarkers();      // 공항 위치 점 그리기
 DrawObjects();
 #ifdef MEASURE_PERFORMANCE
 auto end = std::chrono::steady_clock::now();
 auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
 std::cout << "DrawObjects execution time: " << duration << " ms" << std::endl;
 #endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
 __int64 CurrentTime;

 CurrentTime=GetCurrentTimeInMsec();
 SystemTime->Caption=TimeToChar(CurrentTime);

ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------

void TForm1::SetupRenderingState()
{
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glLineWidth(3.0);
  glPointSize(4.0);
  glColor4f(1.0, 1.0, 1.0, 1.0);
}

void TForm1::DrawMapCenterCross()
{
  double ScrX, ScrY;
  LatLon2XY(MapCenterLat, MapCenterLon, ScrX, ScrY);

  glBegin(GL_LINE_STRIP);
  glVertex2f(ScrX-20.0,ScrY);
  glVertex2f(ScrX+20.0,ScrY);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex2f(ScrX,ScrY-20.0);
  glVertex2f(ScrX,ScrY+20.0);
  glEnd();
}

void TForm1::DrawTemporaryArea()
{
  if (!AreaTemp) return;
  glPointSize(3.0);
  for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
    LatLon2XY(AreaTemp->Points[i][1], AreaTemp->Points[i][0],
              AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);

  glBegin(GL_POINTS);
  for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
    glVertex2f(AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);
  glEnd();
  glBegin(GL_LINE_STRIP);
  for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
    glVertex2f(AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);
  glEnd();
}

void TForm1::DrawDefinedAreas()
{
  DWORD Count = Areas->Count;
  for (DWORD i = 0; i < Count; i++)
  {
    TArea *Area = (TArea *)Areas->Items[i];
    TMultiColor MC;
    MC.Rgb = ColorToRGB(Area->Color);
    if (Area->Selected)
    {
      glLineWidth(4.0);
      glPushAttrib(GL_LINE_BIT);
      glLineStipple(3, 0xAAAA);
    }

    glColor4f(MC.Red/255.0, MC.Green/255.0, MC.Blue/255.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (DWORD j = 0; j < Area->NumPoints; j++)
    {
      double x,y;
      LatLon2XY(Area->Points[j][1], Area->Points[j][0], x, y);
      glVertex2f(x, y);
    }
    glEnd();
    if (Area->Selected)
    {
      glPopAttrib();
      glLineWidth(2.0);
    }

    glColor4f(MC.Red/255.0, MC.Green/255.0, MC.Blue/255.0, 0.4);
    for (DWORD j = 0; j < Area->NumPoints; j++)
      LatLon2XY(Area->Points[j][1], Area->Points[j][0],
                Area->PointsAdj[j][0], Area->PointsAdj[j][1]);
    TTriangles *Tri = Area->Triangles;
    while (Tri)
    {
      glBegin(GL_TRIANGLES);
      glVertex2f(Area->PointsAdj[Tri->indexList[0]][0], Area->PointsAdj[Tri->indexList[0]][1]);
      glVertex2f(Area->PointsAdj[Tri->indexList[1]][0], Area->PointsAdj[Tri->indexList[1]][1]);
      glVertex2f(Area->PointsAdj[Tri->indexList[2]][0], Area->PointsAdj[Tri->indexList[2]][1]);
      glEnd();
      Tri = Tri->next;
    }
  }
}

bool TForm1::ShouldDisplayAircraft(TADS_B_Aircraft* Data, const RouteInfo* route, AircraftCategory category,
                                   int minSpeed, int maxSpeed, int minAlt, int maxAlt,
                                   bool airlineFilter, bool originFilter, bool destFilter)
{
  // 선택된 충돌 페어에 해당하는 항공기는 무조건 표시
  if (FSelectedConflictPair.first != 0 && FSelectedConflictPair.second != 0) {
    if (Data->ICAO == FSelectedConflictPair.first || Data->ICAO == FSelectedConflictPair.second) {
      return true; // 다른 필터 무시하고 무조건 표시
    }
  }
  
  // 충돌감지된 항공기인지 확인
  bool isConflictAircraft = FConflictMap.count(Data->ICAO) > 0;
  
  // "충돌감지된 항공기만 표시" 옵션이 활성화된 경우
  if (m_showOnlyConflictAircraft) {
    return isConflictAircraft; // 충돌감지된 항공기만 표시
  }
  
  // "충돌감지된 항공기는 필터 관계없이 항상 표시" 옵션이 활성화된 경우
  if (m_showConflictAircraftAlways && isConflictAircraft) {
    return true; // 다른 필터 무시하고 무조건 표시
  }
  
  if (airlineFilter && route)
  {
    AnsiString code = route->airlineCode.c_str();
    if (code.SubString(1, filterAirline.Length()) != filterAirline)
      return false;
  }

  if (originFilter && route && !route->airportCodes.empty())
  {
    if (AnsiString(route->airportCodes.front().c_str()) != filterOrigin)
      return false;
  }

  if (destFilter && route && !route->airportCodes.empty())
  {
    if (AnsiString(route->airportCodes.back().c_str()) != filterDestination)
      return false;
  }


  if (Data->HaveSpeedAndHeading)
    if (Data->Speed < minSpeed || Data->Speed > maxSpeed)
      return false;

  if (Data->HaveAltitude)
    if (Data->Altitude < minAlt || Data->Altitude > maxAlt)
      return false;

  if (filterPolygonOnly && Areas->Count > 0)
  {
    bool inAnyPolygon = false;
    pfVec3 aircraftPoint;
    aircraftPoint[0] = Data->Longitude;
    aircraftPoint[1] = Data->Latitude;
    aircraftPoint[2] = 0.0;

    for (DWORD i = 0; i < Areas->Count; i++)
    {
      TArea *Area = (TArea *)Areas->Items[i];
      if (PointInPolygon(Area->Points, Area->NumPoints, aircraftPoint))
      {
        inAnyPolygon = true;
        break;
      }
    }

    if (!inAnyPolygon && Data->HaveSpeedAndHeading)
    {
      const double timeIntervals[] = {5.0, 10.0, 15.0};
      const int numIntervals = sizeof(timeIntervals) / sizeof(timeIntervals[0]);
      for (int t = 0; t < numIntervals && !inAnyPolygon; t++)
      {
        double futureLat, futureLon, junk;
        double timeInHours = timeIntervals[t] / 60.0;
        if (VDirect(Data->Latitude, Data->Longitude,
                   Data->Heading, Data->Speed * timeInHours,
                   &futureLat, &futureLon, &junk) == OKNOERROR)
        {
          pfVec3 futurePoint;
          futurePoint[0] = futureLon;
          futurePoint[1] = futureLat;
          futurePoint[2] = 0.0;
          for (DWORD i = 0; i < Areas->Count; i++)
          {
            TArea *Area = (TArea *)Areas->Items[i];
            if (PointInPolygon(Area->Points, Area->NumPoints, futurePoint))
            {
              inAnyPolygon = true;
              break;
            }
          }
        }
      }
    }
    if (!inAnyPolygon)
      return false;
  }

  if (filterWaypointsOnly && route && !route->airportCodes.empty())
  {
    bool inWaypointArea = false;
    pfVec3 aircraftPoint;
    aircraftPoint[0] = Data->Longitude;
    aircraftPoint[1] = Data->Latitude;
    aircraftPoint[2] = 0.0;

    for (size_t i = 0; i < route->airportCodes.size() - 1 && !inWaypointArea; i++)
    {
      const AirportInfo* ap1 = nullptr;
      const AirportInfo* ap2 = nullptr;
      auto it1 = icaoToAirport.find(route->airportCodes[i]);
      auto it2 = icaoToAirport.find(route->airportCodes[i + 1]);
      if (it1 != icaoToAirport.end()) ap1 = it1->second;
      if (it2 != icaoToAirport.end()) ap2 = it2->second;
      if (ap1 && ap2)
      {
        double lat1 = ap1->latitude, lon1 = ap1->longitude;
        double lat2 = ap2->latitude, lon2 = ap2->longitude;
        double midLat = (lat1 + lat2) / 2.0;
        double midLon = (lon1 + lon2) / 2.0;
        double distToMid = sqrt(pow(aircraftPoint[1] - midLat, 2) + pow(aircraftPoint[0] - midLon, 2));
        double routeDist = sqrt(pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2));
        if (distToMid <= routeDist / 3.0)
        {
          inWaypointArea = true;
          break;
        }
      }
    }

    if (!inWaypointArea && Data->HaveSpeedAndHeading)
    {
      const double timeIntervals[] = {5.0, 10.0, 15.0};
      const int numIntervals = sizeof(timeIntervals) / sizeof(timeIntervals[0]);
      for (int t = 0; t < numIntervals && !inWaypointArea; t++)
      {
        double futureLat, futureLon, junk;
        double timeInHours = timeIntervals[t] / 60.0;
        if (VDirect(Data->Latitude, Data->Longitude,
                   Data->Heading, Data->Speed * timeInHours,
                   &futureLat, &futureLon, &junk) == OKNOERROR)
        {
          for (size_t i = 0; i < route->airportCodes.size() - 1; i++)
          {
            const AirportInfo* ap1 = nullptr;
            const AirportInfo* ap2 = nullptr;
            auto it1 = icaoToAirport.find(route->airportCodes[i]);
            auto it2 = icaoToAirport.find(route->airportCodes[i + 1]);
            if (it1 != icaoToAirport.end()) ap1 = it1->second;
            if (it2 != icaoToAirport.end()) ap2 = it2->second;
            if (ap1 && ap2)
            {
              double lat1 = ap1->latitude, lon1 = ap1->longitude;
              double lat2 = ap2->latitude, lon2 = ap2->longitude;
              double midLat = (lat1 + lat2) / 2.0;
              double midLon = (lon1 + lon2) / 2.0;
              double distToMid = sqrt(pow(futureLat - midLat, 2) + pow(futureLon - midLon, 2));
              double routeDist = sqrt(pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2));
              if (distToMid <= routeDist / 3.0)
              {
                inWaypointArea = true;
                break;
              }
            }
          }
        }
      }
    }

    if (!inWaypointArea)
      return false;
  }

  bool showAircraft = false;
  switch(category)
  {
    case CATEGORY_COMMERCIAL: showAircraft = CommercialCheckBox->Checked; break;
    case CATEGORY_CARGO: showAircraft = CargoCheckBox->Checked; break;
    case CATEGORY_HELICOPTER: showAircraft = HelicopterCheckBox->Checked; break;
    case CATEGORY_MILITARY: showAircraft = MilitaryCheckBox->Checked; break;
    case CATEGORY_BUSINESS_JET: showAircraft = BusinessJetCheckBox->Checked; break;
    case CATEGORY_GLIDER: showAircraft = GliderCheckBox->Checked; break;
    case CATEGORY_ULTRALIGHT: showAircraft = UltralightCheckBox->Checked; break;
    case CATEGORY_GENERAL_AVIATION:
    case CATEGORY_UNKNOWN:
    default:
      showAircraft = GeneralAviationCheckBox->Checked;
      break;
  }

  return showAircraft;
}

bool TForm1::EvaluateDisplay(const TADS_B_Aircraft* Data, const RouteInfo* route,
                             AircraftCategory category, const RenderThreadParams& p)
{
  if (p.airlineFilter && route)
  {
    AnsiString code = route->airlineCode.c_str();
    if (code.SubString(1, p.filterAirline.Length()) != p.filterAirline)
      return false;
  }

  if (p.originFilter && route && !route->airportCodes.empty())
  {
    if (AnsiString(route->airportCodes.front().c_str()) != p.filterOrigin)
      return false;
  }

  if (p.destFilter && route && !route->airportCodes.empty())
  {
    if (AnsiString(route->airportCodes.back().c_str()) != p.filterDestination)
      return false;
  }

  if (Data->HaveSpeedAndHeading)
    if (Data->Speed < p.minSpeed || Data->Speed > p.maxSpeed)
      return false;

  if (Data->HaveAltitude)
    if (Data->Altitude < p.minAlt || Data->Altitude > p.maxAlt)
      return false;

  if (p.filterPolygonOnly && Areas->Count > 0)
  {
    bool inAnyPolygon = false;
    pfVec3 aircraftPoint;
    aircraftPoint[0] = Data->Longitude;
    aircraftPoint[1] = Data->Latitude;
    aircraftPoint[2] = 0.0;

    for (DWORD i = 0; i < Areas->Count; i++)
    {
      TArea *Area = (TArea *)Areas->Items[i];
      if (PointInPolygon(Area->Points, Area->NumPoints, aircraftPoint))
      {
        inAnyPolygon = true;
        break;
      }
    }

    if (!inAnyPolygon && Data->HaveSpeedAndHeading)
    {
      const double timeIntervals[] = {5.0, 10.0, 15.0};
      const int numIntervals = sizeof(timeIntervals) / sizeof(timeIntervals[0]);
      for (int t = 0; t < numIntervals && !inAnyPolygon; t++)
      {
        double futureLat, futureLon, junk;
        double timeInHours = timeIntervals[t] / 60.0;
        if (VDirect(Data->Latitude, Data->Longitude,
                   Data->Heading, Data->Speed * timeInHours,
                   &futureLat, &futureLon, &junk) == OKNOERROR)
        {
          pfVec3 futurePoint;
          futurePoint[0] = futureLon;
          futurePoint[1] = futureLat;
          futurePoint[2] = 0.0;
          for (DWORD i = 0; i < Areas->Count; i++)
          {
            TArea *Area = (TArea *)Areas->Items[i];
            if (PointInPolygon(Area->Points, Area->NumPoints, futurePoint))
            {
              inAnyPolygon = true;
              break;
            }
          }
        }
      }
    }
    if (!inAnyPolygon)
      return false;
  }

  if (p.filterWaypointsOnly && route && !route->airportCodes.empty())
  {
    bool inWaypointArea = false;
    pfVec3 aircraftPoint;
    aircraftPoint[0] = Data->Longitude;
    aircraftPoint[1] = Data->Latitude;
    aircraftPoint[2] = 0.0;

    for (size_t i = 0; i < route->airportCodes.size() - 1 && !inWaypointArea; i++)
    {
      const AirportInfo* ap1 = nullptr;
      const AirportInfo* ap2 = nullptr;
      auto it1 = icaoToAirport.find(route->airportCodes[i]);
      auto it2 = icaoToAirport.find(route->airportCodes[i + 1]);
      if (it1 != icaoToAirport.end()) ap1 = it1->second;
      if (it2 != icaoToAirport.end()) ap2 = it2->second;
      if (ap1 && ap2)
      {
        double lat1 = ap1->latitude, lon1 = ap1->longitude;
        double lat2 = ap2->latitude, lon2 = ap2->longitude;
        double midLat = (lat1 + lat2) / 2.0;
        double midLon = (lon1 + lon2) / 2.0;
        double distToMid = sqrt(pow(aircraftPoint[1] - midLat, 2) + pow(aircraftPoint[0] - midLon, 2));
        double routeDist = sqrt(pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2));
        if (distToMid <= routeDist / 3.0)
        {
          inWaypointArea = true;
          break;
        }
      }
    }

    if (!inWaypointArea && Data->HaveSpeedAndHeading)
    {
      const double timeIntervals[] = {5.0, 10.0, 15.0};
      const int numIntervals = sizeof(timeIntervals) / sizeof(timeIntervals[0]);
      for (int t = 0; t < numIntervals && !inWaypointArea; t++)
      {
        double futureLat, futureLon, junk;
        double timeInHours = timeIntervals[t] / 60.0;
        if (VDirect(Data->Latitude, Data->Longitude,
                   Data->Heading, Data->Speed * timeInHours,
                   &futureLat, &futureLon, &junk) == OKNOERROR)
        {
          for (size_t i = 0; i < route->airportCodes.size() - 1; i++)
          {
            const AirportInfo* ap1 = nullptr;
            const AirportInfo* ap2 = nullptr;
            auto it1 = icaoToAirport.find(route->airportCodes[i]);
            auto it2 = icaoToAirport.find(route->airportCodes[i + 1]);
            if (it1 != icaoToAirport.end()) ap1 = it1->second;
            if (it2 != icaoToAirport.end()) ap2 = it2->second;
            if (ap1 && ap2)
            {
              double lat1 = ap1->latitude, lon1 = ap1->longitude;
              double lat2 = ap2->latitude, lon2 = ap2->longitude;
              double midLat = (lat1 + lat2) / 2.0;
              double midLon = (lon1 + lon2) / 2.0;
              double distToMid = sqrt(pow(futureLat - midLat, 2) + pow(futureLon - midLon, 2));
              double routeDist = sqrt(pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2));
              if (distToMid <= routeDist / 3.0)
              {
                inWaypointArea = true;
                break;
              }
            }
          }
        }
      }
    }

    if (!inWaypointArea)
      return false;
  }

  bool showAircraft = false;
  switch(category)
  {
    case CATEGORY_COMMERCIAL: showAircraft = p.showCommercial; break;
    case CATEGORY_CARGO: showAircraft = p.showCargo; break;
    case CATEGORY_HELICOPTER: showAircraft = p.showHelicopter; break;
    case CATEGORY_MILITARY: showAircraft = p.showMilitary; break;
    case CATEGORY_BUSINESS_JET: showAircraft = p.showBusinessJet; break;
    case CATEGORY_GLIDER: showAircraft = p.showGlider; break;
    case CATEGORY_ULTRALIGHT: showAircraft = p.showUltralight; break;
    case CATEGORY_GENERAL_AVIATION:
    case CATEGORY_UNKNOWN:
    default:
      showAircraft = p.showGeneralAviation;
      break;
  }

  return showAircraft;
}

__fastcall TAircraftRenderThread::TAircraftRenderThread(TForm1* owner)
  : TThread(false), FOwner(owner) {}

void __fastcall TAircraftRenderThread::Execute()
{
  while(!Terminated)
  {
    RenderThreadParams params;
    bool check_dead_reckoning = false;
    {
      std::lock_guard<std::mutex> lock(FOwner->m_renderInfoMutex);
      params = FOwner->m_renderParams;
      check_dead_reckoning = FOwner->DeadReckoningCheckBox->Checked;
    }

    std::unordered_map<unsigned int, AircraftRenderInfo> local;
    const void* Key;
    ght_iterator_t iterator;
    TADS_B_Aircraft* Data;
    for(Data = FOwner->FAircraftModel->GetFirstAircraft(&iterator, &Key);
        Data; Data = FOwner->FAircraftModel->GetNextAircraft(&iterator, &Key))
    {
      AircraftRenderInfo info = {};
      const RouteInfo* route = nullptr;
      if (params.airlineFilter || params.originFilter || params.destFilter)
      {
        auto it = callSignToRoute.find(AnsiString(Data->FlightNum).c_str());
        route = (it != callSignToRoute.end()) ? it->second : nullptr;
        if (!FOwner->IsRouteMatched(route, params.airlineFilter, params.originFilter, params.destFilter))
        {
          local[Data->ICAO] = info;
          continue;
        }
      }

      AircraftCategory category = Data->Category;
      if (!FOwner->EvaluateDisplay(Data, route, category, params))
      {
        local[Data->ICAO] = info;
        continue;
      }

      info.show = true;
      float color[4] = {1.0f,1.0f,1.0f,1.0f};
      bool isPartOfSelectedPair = (Data->ICAO == FOwner->FSelectedConflictPair.first || Data->ICAO == FOwner->FSelectedConflictPair.second);

      if (aircraft_is_helicopter(Data->ICAO, NULL))
        { color[0]=0.0f; color[1]=1.0f; color[2]=0.0f; }
      else if (aircraft_is_military(Data->ICAO, NULL))
        { color[0]=1.0f; color[1]=0.0f; color[2]=0.0f; }

      if (Data->HaveSpeedAndHeading)
        color[0]=1.0f, color[1]=0.0f, color[2]=1.0f, color[3]=1.0f;
      else
        color[0]=1.0f, color[1]=0.0f, color[2]=0.0f, color[3]=1.0f;

      if (isPartOfSelectedPair)
        { color[0]=0.0f; color[1]=1.0f; color[2]=1.0f; }
      else if (FOwner->FConflictMap.count(Data->ICAO))
        { color[0]=0.0f; color[1]=0.0f; color[2]=1.0f; }

      float scale = 1.5f;
      if (Data->HaveAltitude)
      {
        if (Data->Altitude > 30000) scale = 1.0f;
        else if (Data->Altitude < 10000) scale = 2.0f;
      }

      info.scale = scale;
      info.heading = Data->HaveSpeedAndHeading ? Data->Heading : 0.0f;
      info.imageNum = static_cast<int>(category);
      memcpy(info.planeColor, color, sizeof(color));
      memcpy(info.textColor, color, sizeof(color));
      info.glyphCount = 0;
      for(int i=0;i<6 && Data->HexAddr[i]; ++i)
      {
        char c = Data->HexAddr[i];
        if(c>='0' && c<='9') info.glyphs[i] = c-'0';
        else if(c>='A' && c<='F') info.glyphs[i] = 10 + (c-'A');
        else info.glyphs[i] = 0;
        info.glyphCount = i+1;
      }
      local[Data->ICAO] = info;
    }

    {
      std::lock_guard<std::mutex> lock(FOwner->m_renderInfoMutex);
      FOwner->m_renderInfoTable.swap(local);
    }

     if (check_dead_reckoning)
    {
      double futureLat, futureLon, junk;
      double diff_time;
      __int64 cur_time = GetCurrentTimeInMsec();

      for(Data = FOwner->FAircraftModel->GetFirstAircraft(&iterator, &Key);
          Data; Data = FOwner->FAircraftModel->GetNextAircraft(&iterator, &Key))
      {
        diff_time = (cur_time - Data->LastSeen) / 1000.0 / 3600;
        if (VDirect(Data->Latitude, Data->Longitude,
                   Data->Heading, Data->Speed * diff_time,
                   &futureLat, &futureLon, &junk) == OKNOERROR)
        {
          Data->LastSeen = cur_time;
          Data->Latitude = futureLat;
          Data->Longitude = futureLon;  
        }
      }
    }

    TThread::Sleep(5);
  }
}

void TForm1::BuildAircraftBatches(int &ViewableAircraft)
{
  const void *Key;
  ght_iterator_t iterator;
  TADS_B_Aircraft* Data;
  {
    std::lock_guard<std::mutex> lock(m_renderInfoMutex);
    m_renderParams.minSpeed = SpeedMinTrackBar->Position;
    m_renderParams.maxSpeed = SpeedMaxTrackBar->Position;
    m_renderParams.minAlt   = AltitudeMinTrackBar->Position;
    m_renderParams.maxAlt   = AltitudeMaxTrackBar->Position;
    m_renderParams.airlineFilter  = !filterAirline.IsEmpty();
    m_renderParams.originFilter   = !filterOrigin.IsEmpty();
    m_renderParams.destFilter     = !filterDestination.IsEmpty();
    m_renderParams.filterAirline  = filterAirline;
    m_renderParams.filterOrigin   = filterOrigin;
    m_renderParams.filterDestination = filterDestination;
    m_renderParams.showCommercial      = CommercialCheckBox->Checked;
    m_renderParams.showCargo           = CargoCheckBox->Checked;
    m_renderParams.showHelicopter      = HelicopterCheckBox->Checked;
    m_renderParams.showMilitary        = MilitaryCheckBox->Checked;
    m_renderParams.showBusinessJet     = BusinessJetCheckBox->Checked;
    m_renderParams.showGlider          = GliderCheckBox->Checked;
    m_renderParams.showUltralight      = UltralightCheckBox->Checked;
    m_renderParams.showGeneralAviation = GeneralAviationCheckBox->Checked;
    m_renderParams.filterPolygonOnly   = filterPolygonOnly;
    m_renderParams.filterWaypointsOnly = filterWaypointsOnly;
  }

  m_planeBatch.clear();
  m_lineBatch.clear();
  m_textBatch.clear();

  for(Data = FAircraftModel->GetFirstAircraft(&iterator, &Key);
      Data; Data = FAircraftModel->GetNextAircraft(&iterator, &Key))
  {
    if (!Data->HaveLatLon) continue;

    double ScrX, ScrY;
    LatLon2XY(Data->Latitude, Data->Longitude, ScrX, ScrY);
    if (ScrX < Map_v[0].x || ScrX > Map_v[1].x ||
        ScrY < Map_v[0].y || ScrY > Map_v[3].y)
      continue;

    auto itInfo = m_renderInfoTable.find(Data->ICAO);
    if (itInfo == m_renderInfoTable.end() || !itInfo->second.show)
      continue;
    const AircraftRenderInfo& info = itInfo->second;

    ViewableAircraft++;

    double ScrX2 = ScrX, ScrY2 = ScrY;
    if (Data->HaveSpeedAndHeading && TimeToGoCheckBox->State==cbChecked)
    {
      double lat2,lon2,az2;
      VDirect(Data->Latitude,Data->Longitude,
              Data->Heading,Data->Speed*(double)TimeToGoTrackBar->Position/3600.0,&lat2,&lon2,&az2);
      LatLon2XY(lat2,lon2, ScrX2, ScrY2);
    }

    AirplaneInstance inst;
    inst.x = ScrX; inst.y = ScrY; inst.scale = info.scale;
    inst.heading = info.heading;
    inst.imageNum = info.imageNum;
    memcpy(inst.color, info.planeColor, sizeof(inst.color));
    m_planeBatch.push_back(inst);

    AirplaneLineInstance line; line.x1 = ScrX; line.y1 = ScrY; line.x2 = ScrX2; line.y2 = ScrY2;
    m_lineBatch.push_back(line);

    for(int i=0; i<info.glyphCount; ++i)
    {
      HexCharInstance tc;
      tc.x = ScrX + 40 + i * (HEX_FONT_WIDTH - 15) * GetHexTextScale();
      tc.y = ScrY - 10;
      tc.glyph = info.glyphs[i];
      memcpy(tc.color, info.textColor, sizeof(tc.color));
      m_textBatch.push_back(tc);
    }
  }
}

void TForm1::RenderAircraftBatches()
{
  // for (const auto& line : m_lineBatch)
  // {
  //   TADS_B_Aircraft* aircraft = FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CC);
  //   if (aircraft && aircraft_is_military(aircraft->ICAO, NULL))
  //     DrawLeaderThick(line.x1, line.y1, line.x2, line.y2, 4.0f);
  //   else if (aircraft && aircraft_is_helicopter(aircraft->ICAO, NULL))
  //     DrawLeaderDashed(line.x1, line.y1, line.x2, line.y2);
  //   else
  //     DrawLeaderArrow(line.x1, line.y1, line.x2, line.y2, 8.0f);
  // }

  DrawAirplaneLinesInstanced(m_lineBatch);
  DrawAirplaneImagesInstanced(m_planeBatch);
  DrawHexTextInstanced(m_textBatch);
}

void TForm1::UpdateTrackHookDisplay()
{
  TADS_B_Aircraft* Data = nullptr;
  double ScrX,ScrY;
  if (TrackHook.Valid_CC)
  {
    Data = FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CC);
    if (Data)
    {
      ICAOLabel->Caption=Data->HexAddr;
      if (Data->HaveFlightNum) FlightNumLabel->Caption=Data->FlightNum; else FlightNumLabel->Caption="N/A";
      std::string airline, country;
      if (LookupAirline(AnsiString(Data->FlightNum).Trim().c_str(), airline, country))
      {
        AirlineNameLabel->Caption = airline.c_str();
        AirlineCountryLabel->Caption = country.c_str();
      }
      else
      {
        AirlineNameLabel->Caption = "N/A";
        AirlineCountryLabel->Caption = "N/A";
      }
      if (Data->HaveLatLon)
      {
        CLatLabel->Caption=DMS::DegreesMinutesSecondsLat(Data->Latitude).c_str();
        CLonLabel->Caption=DMS::DegreesMinutesSecondsLon(Data->Longitude).c_str();
      }
      else
      {
        CLatLabel->Caption="N/A";
        CLonLabel->Caption="N/A";
      }
      if (Data->HaveSpeedAndHeading)
      {
        SpdLabel->Caption=FloatToStrF(Data->Speed, ffFixed,12,2)+" KTS VRATE:"+FloatToStrF(Data->VerticalRate,ffFixed,12,2);
        HdgLabel->Caption=FloatToStrF(Data->Heading, ffFixed,12,2)+" DEG";
      }
      else
      {
        SpdLabel->Caption="N/A";
        HdgLabel->Caption="N/A";
      }
      if (Data->Altitude)
        AltLabel->Caption= FloatToStrF(Data->Altitude, ffFixed,12,2)+"FT";
      else AltLabel->Caption="N/A";

      MsgCntLabel->Caption="Raw: "+IntToStr((int)Data->NumMessagesRaw)+" SBS: "+IntToStr((int)Data->NumMessagesSBS);
      TrkLastUpdateTimeLabel->Caption=TimeToChar(Data->LastSeen);

      glColor4f(1.0, 0.0, 0.0, 1.0);
      LatLon2XY(Data->Latitude,Data->Longitude, ScrX, ScrY);
      DrawTrackHook(ScrX, ScrY);
    }
  }
  else
  {
    TrackHook.Valid_CC=false;
    ICAOLabel->Caption="N/A";
    FlightNumLabel->Caption="N/A";
    AircraftModelLabel->Caption="N/A";
    AirlineNameLabel->Caption="N/A";
    AirlineCountryLabel->Caption="N/A";
    CLatLabel->Caption="N/A";
    CLonLabel->Caption="N/A";
    SpdLabel->Caption="N/A";
    HdgLabel->Caption="N/A";
    AltLabel->Caption="N/A";
    MsgCntLabel->Caption="N/A";
    TrkLastUpdateTimeLabel->Caption="N/A";
  }
}

void TForm1::DrawCPAVisualization()
{
  if (!TrackHook.Valid_CPA) return;
  bool CpaDataIsValid=false;
  TADS_B_Aircraft* Data = FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CC);
  TADS_B_Aircraft* DataCPA= FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CPA);
  double ScrX,ScrY;
  if ((DataCPA) && (TrackHook.Valid_CC) && Data)
  {
    double tcpa,cpa_distance_nm, vertical_cpa;
    double lat1, lon1,lat2, lon2, junk;
    if (computeCPA(Data->Latitude, Data->Longitude, Data->Altitude,
                   Data->Speed,Data->Heading,
                   DataCPA->Latitude, DataCPA->Longitude,DataCPA->Altitude,
                   DataCPA->Speed,DataCPA->Heading,
                   tcpa,cpa_distance_nm, vertical_cpa))
    {
      if (VDirect(Data->Latitude,Data->Longitude,
                  Data->Heading,Data->Speed/3600.0*tcpa,&lat1,&lon1,&junk)==OKNOERROR)
      {
        if (VDirect(DataCPA->Latitude,DataCPA->Longitude,
                    DataCPA->Heading,DataCPA->Speed/3600.0*tcpa,&lat2,&lon2,&junk)==OKNOERROR)
        {
          glColor4f(0.0, 1.0, 0.0, 1.0);
          glBegin(GL_LINE_STRIP);
          LatLon2XY(Data->Latitude,Data->Longitude, ScrX, ScrY);
          glVertex2f(ScrX, ScrY);
          LatLon2XY(lat1,lon1, ScrX, ScrY);
          glVertex2f(ScrX, ScrY);
          glEnd();
          glBegin(GL_LINE_STRIP);
          LatLon2XY(DataCPA->Latitude,DataCPA->Longitude, ScrX, ScrY);
          glVertex2f(ScrX, ScrY);
          LatLon2XY(lat2,lon2, ScrX, ScrY);
          glVertex2f(ScrX, ScrY);
          glEnd();
          glColor4f(1.0, 0.0, 0.0, 1.0);
          glBegin(GL_LINE_STRIP);
          LatLon2XY(lat1,lon1, ScrX, ScrY);
          glVertex2f(ScrX, ScrY);
          LatLon2XY(lat2,lon2, ScrX, ScrY);
          glVertex2f(ScrX, ScrY);
          glEnd();
          CpaTimeValue->Caption=TimeToChar(tcpa*1000);
          CpaDistanceValue->Caption= FloatToStrF(cpa_distance_nm, ffFixed,10,2)+" NM VDIST: "+IntToStr((int)vertical_cpa)+" FT";
          CpaDataIsValid=true;
        }
      }
    }
  }
  if (!CpaDataIsValid)
  {
    TrackHook.Valid_CPA=false;
    CpaTimeValue->Caption="None";
    CpaDistanceValue->Caption="None";
  }
}

void TForm1::DrawSelectedRoutes()
{
  if (m_selectedRoutePaths.empty()) return;
  glLineWidth(2.0f);
  glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
  for (const auto& seg : m_selectedRoutePaths)
  {
    if (seg.size() < 2) continue;
    double px = 0, py = 0;
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < seg.size(); ++i)
    {
      double x,y; LatLon2XY(seg[i].first, seg[i].second, x, y);
      glVertex2f(x, y); if (i == seg.size() - 2) { px = x; py = y; }
    }
    glEnd();
    double xe, ye; LatLon2XY(seg.back().first, seg.back().second, xe, ye);
    DrawLeaderArrow(px, py, xe, ye, 10.0f);
  }
  glLineWidth(1.0f);
}

void TForm1::DrawSelectedConflictPair()
{
  if (FSelectedConflictPair.first == 0) return;
  TADS_B_Aircraft* ac1 = FAircraftModel->FindAircraftByICAO(FSelectedConflictPair.first);
  TADS_B_Aircraft* ac2 = FAircraftModel->FindAircraftByICAO(FSelectedConflictPair.second);
  if (ac1 && ac2 && ac1->HaveLatLon && ac2->HaveLatLon)
  {
    double tcpa = 0;
    double cpa_distance_nm = 0;
    bool found = false;
    if (FConflictMap.count(ac1->ICAO))
    {
      for (const auto& info : FConflictMap.at(ac1->ICAO))
      {
        if (info.otherAircraftICAO == ac2->ICAO)
        {
          tcpa = info.timeToCPA;
          cpa_distance_nm = info.cpaDistance;
          found = true;
          break;
        }
      }
    }
    if (found)
    {
      double lat1, lon1, lat2, lon2, junk;
      VDirect(ac1->Latitude, ac1->Longitude, ac1->Heading, ac1->Speed / 3600.0 * tcpa, &lat1, &lon1, &junk);
      VDirect(ac2->Latitude, ac2->Longitude, ac2->Heading, ac2->Speed / 3600.0 * tcpa, &lat2, &lon2, &junk);
      double x1_curr, y1_curr, x1_fut, y1_fut;
      double x2_curr, y2_curr, x2_fut, y2_fut;
      LatLon2XY(ac1->Latitude, ac1->Longitude, x1_curr, y1_curr);
      LatLon2XY(lat1, lon1, x1_fut, y1_fut);
      LatLon2XY(ac2->Latitude, ac2->Longitude, x2_curr, y2_curr);
      LatLon2XY(lat2, lon2, x2_fut, y2_fut);
      glLineWidth(3.0);
      glColor4f(0.0, 1.0, 0.0, 0.6);
      glBegin(GL_LINES);
      glVertex2d(x1_curr, y1_curr); glVertex2d(x1_fut, y1_fut);
      glVertex2d(x2_curr, y2_curr); glVertex2d(x2_fut, y2_fut);
      glEnd();
      glLineWidth(5.0);
      glColor4f(1.0, 0.0, 0.0, 0.8);
      glBegin(GL_LINES);
      glVertex2d(x1_fut, y1_fut);
      glVertex2d(x2_fut, y2_fut);
      glEnd();
    }
  }
}

void __fastcall TForm1::DrawObjects(void)
{
  int ViewableAircraft = 0;

  SetupRenderingState();
  DrawMapCenterCross();
  DrawTemporaryArea();
  DrawDefinedAreas();
  BuildAircraftBatches(ViewableAircraft);
  RenderAircraftBatches();
  ViewableAircraftCountLabel->Caption = ViewableAircraft;
  UpdateTrackHookDisplay();
  DrawCPAVisualization();
  DrawSelectedRoutes();
  DrawSelectedConflictPair();
}
bool TForm1::IsRouteMatched(const RouteInfo* route,
                            bool airlineFilter,
                            bool originFilter,
                            bool destFilter) const {
    if (!route) return false;
    if (airlineFilter && AnsiString(route->airlineCode.c_str()) != filterAirline)
        return false;
    if (originFilter && route->airportCodes.size() &&
        AnsiString(route->airportCodes.front().c_str()) != filterOrigin)
        return false;
    if (destFilter && route->airportCodes.size() &&
        AnsiString(route->airportCodes.back().c_str()) != filterDestination)
        return false;
    return true;
}


void __fastcall TForm1::ObjectDisplayMouseDown(TObject *Sender,
	  TMouseButton Button, TShiftState Shift, int X, int Y)
{

 if (Button==mbLeft)
   {
	if (Shift.Contains(ssCtrl))
	{

	}
	else
	{
	 g_MouseLeftDownX = X;
	 g_MouseLeftDownY = Y;
	 g_MouseDownMask |= LEFT_MOUSE_DOWN ;
	 if(g_EarthView)
	 	g_EarthView->StartDrag(X, Y, NAV_DRAG_PAN);
	}
  }
 else if (Button==mbRight)
  {
  if (AreaTemp)
   {
    // 오른쪽 마우스 더블클릭 감지를 위한 정적 변수
    static DWORD lastRightClickTime = 0;
    static int lastRightClickX = 0;
    static int lastRightClickY = 0;
    
    DWORD currentTime = GetTickCount();
    
    // 더블클릭 조건: 500ms 이내, 같은 위치에서 클릭 (5픽셀 이내)
    if (currentTime - lastRightClickTime < 500 && 
        abs(X - lastRightClickX) < 5 && 
        abs(Y - lastRightClickY) < 5)
    {
      // 오른쪽 마우스 더블클릭 - 폴리곤 완성
      if (AreaTemp->NumPoints >= 3)
      {
        CompleteClick(NULL);
        return;
      }
    }
    
    lastRightClickTime = currentTime;
    lastRightClickX = X;
    lastRightClickY = Y;
    
	if (AreaTemp->NumPoints<MAX_AREA_POINTS)
	{
	  AddPoint(X, Y);
	}
	else ShowMessage("Max Area Points Reached");
   }
  else
   {
   if (Shift.Contains(ssCtrl))   HookTrack(X,Y,true);
   else  HookTrack(X,Y,false);
   }
  }

 else if (Button==mbMiddle)  ResetXYOffset();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ObjectDisplayMouseUp(TObject *Sender,
	  TMouseButton Button, TShiftState Shift, int X, int Y)
{
  if (Button == mbLeft) g_MouseDownMask &= ~LEFT_MOUSE_DOWN;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayMouseMove(TObject *Sender,
	  TShiftState Shift, int X, int Y)
{
 int X1,Y1;
 double VLat,VLon;
 int i;
 Y1=(ObjectDisplay->Height-1)-Y;
 X1=X;
 if  ((X1>=Map_v[0].x) && (X1<=Map_v[1].x) &&
	  (Y1>=Map_v[0].y) && (Y1<=Map_v[3].y))

  {
   pfVec3 Point;
   VLat=atan(sinh(M_PI * (2 * (Map_w[1].y-(yf*(Map_v[3].y-Y1))))))*(180.0 / M_PI);
   VLon=(Map_w[1].x-(xf*(Map_v[1].x-X1)))*360.0;
   Lat->Caption=DMS::DegreesMinutesSecondsLat(VLat).c_str();
   Lon->Caption=DMS::DegreesMinutesSecondsLon(VLon).c_str();
   Point[0]=VLon;
   Point[1]=VLat;
   Point[2]=0.0;

   for (i = 0; i < Areas->Count; i++)
	 {
	   TArea *Area = (TArea *)Areas->Items[i];
	   if (PointInPolygon(Area->Points,Area->NumPoints,Point))
	   {
#if 0
		  MsgLog->Lines->Add("In Polygon "+ Area->Name);
#endif
       }
	 }
  }

  if (g_MouseDownMask & LEFT_MOUSE_DOWN)
  {
    if(g_EarthView)
		g_EarthView->Drag(g_MouseLeftDownX, g_MouseLeftDownY, X,Y, NAV_DRAG_PAN);
   ObjectDisplay->Repaint();
  }

}
//---------------------------------------------------------------------------
void __fastcall TForm1::ResetXYOffset(void)
{
 if(g_EarthView)
 	SetMapCenter(g_EarthView->m_Eye.x, g_EarthView->m_Eye.y);
 ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Exit1Click(TObject *Sender)
{
 Close();
}
//---------------------------------------------------------------------------
 void __fastcall TForm1::AddPoint(int X, int Y)
 {
  double Lat,Lon;

 if (XY2LatLon2(X,Y,Lat,Lon)==0)
 {

	AreaTemp->Points[AreaTemp->NumPoints][1]=Lat;
	AreaTemp->Points[AreaTemp->NumPoints][0]=Lon;
	AreaTemp->Points[AreaTemp->NumPoints][2]=0.0;
	AreaTemp->NumPoints++;
	ObjectDisplay->Repaint();
 }
 }
//---------------------------------------------------------------------------
 void __fastcall TForm1::HookTrack(int X, int Y,bool CPA_Hook)
 {
  double VLat,VLon, dlat,dlon,Range;
  int X1,Y1;
   const void *Key;

   uint32_t Current_ICAO;
   double MinRange;
  ght_iterator_t iterator;
  TADS_B_Aircraft* Data;

  Y1=(ObjectDisplay->Height-1)-Y;
  X1=X;

  if  ((X1<Map_v[0].x) || (X1>Map_v[1].x) ||
	   (Y1<Map_v[0].y) || (Y1>Map_v[3].y)) return;

  VLat=atan(sinh(M_PI * (2 * (Map_w[1].y-(yf*(Map_v[3].y-Y1))))))*(180.0 / M_PI);
  VLon=(Map_w[1].x-(xf*(Map_v[1].x-X1)))*360.0;

  MinRange=16.0;

  for(Data = FAircraftModel->GetFirstAircraft(&iterator, &Key);
			  Data; Data = FAircraftModel->GetNextAircraft(&iterator, &Key))
	{
	  if (Data->HaveLatLon)
	  {
	   dlat= VLat-Data->Latitude;
	   dlon= VLon-Data->Longitude;
	   Range=sqrt(dlat*dlat+dlon*dlon);
	   if (Range<MinRange)
	   {
		Current_ICAO=Data->ICAO;
		MinRange=Range;
	   }
	  }
	}
	if (MinRange< 0.2)
	{
	  TADS_B_Aircraft * ADS_B_Aircraft = FAircraftModel->FindAircraftByICAO(Current_ICAO);
	  if (ADS_B_Aircraft)
	  {
		if (!CPA_Hook)
		{
		 TrackHook.Valid_CC=true;
		 TrackHook.ICAO_CC=ADS_B_Aircraft->ICAO;
		 LOG_DEBUG_F(LogHandler::CAT_GENERAL, "Selected aircraft info: %s", GetAircraftDBInfo(ADS_B_Aircraft->ICAO));
     OnAircraftSelected(ADS_B_Aircraft->ICAO);
		}
		else
		{
		 TrackHook.Valid_CPA=true;
		 TrackHook.ICAO_CPA=ADS_B_Aircraft->ICAO;
        }
;
	  }

	}
	else
		{
		 if (!CPA_Hook)
		  {
		   TrackHook.Valid_CC=false;
           m_selectedRoutePaths.clear();
           UpdateCloseControlPanel(nullptr, nullptr);
		  }
		 else
		   {
			TrackHook.Valid_CPA=false;
			CpaTimeValue->Caption="None";
	        CpaDistanceValue->Caption="None";
           }
		}

 }
//---------------------------------------------------------------------------
void __fastcall TForm1::LatLon2XY(double lat,double lon, double &x, double &y)
{
 x=(Map_v[1].x-((Map_w[1].x-(lon/360.0))/xf));
 y= Map_v[3].y- (Map_w[1].y/yf)+ (asinh(tan(lat*M_PI/180.0))/(2*M_PI*yf));
}
//---------------------------------------------------------------------------
int __fastcall TForm1::XY2LatLon2(int x, int y,double &lat,double &lon )
{
  double Lat,Lon, dlat,dlon,Range;
  int X1,Y1;

  Y1=(ObjectDisplay->Height-1)-y;
  X1=x;

  if  ((X1<Map_v[0].x) || (X1>Map_v[1].x) ||
	   (Y1<Map_v[0].y) || (Y1>Map_v[3].y)) return -1;

  lat=atan(sinh(M_PI * (2 * (Map_w[1].y-(yf*(Map_v[3].y-Y1))))))*(180.0 / M_PI);
  lon=(Map_w[1].x-(xf*(Map_v[1].x-X1)))*360.0;

  return 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomInClick(TObject *Sender)
{
  if(g_EarthView)
  	g_EarthView->SingleMovement(NAV_ZOOM_IN);
  ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ZoomOutClick(TObject *Sender)
{
 if(g_EarthView)
	g_EarthView->SingleMovement(NAV_ZOOM_OUT);

 ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer2Timer(TObject *Sender)
{
    if (PurgeStale->Checked == false) return;
    if (FOwner->DeadReckoningCheckBox->Checked) return;

    // Model에게 "오래된 항공기 삭제" 작업을 위임
    FAircraftModel->PurgeStaleAircraft(CSpinStaleTime->Value);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PurgeButtonClick(TObject *Sender)
{
    // 1. Model에게 "모든 항공기 삭제" 작업을 위임
    if (FOwner->DeadReckoningCheckBox->Checked) return;
    m_selectedRoutePaths.clear();
    FAircraftModel->PurgeAllAircraft();
    
    // 2. CycleImages가 체크되어 있으면 FCurrentSpriteImage도 리셋
    if (CycleImages->Checked) {
        FAircraftModel->ResetCurrentSpriteImage();
    }

    // 3. 화면에서 모든 항공기가 사라졌으므로, 즉시 화면을 새로고침
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertClick(TObject *Sender)
{
 Insert->Enabled=false;
 LoadARTCCBoundaries1->Enabled=false;
 Complete->Enabled=true;
 Cancel->Enabled=true;
 //Delete->Enabled=false;

 AreaTemp= new TArea;
 AreaTemp->NumPoints=0;
 AreaTemp->Name="";
 AreaTemp->Selected=false;
 AreaTemp->Triangles=NULL;

}
//---------------------------------------------------------------------------
void __fastcall TForm1::CancelClick(TObject *Sender)
{
 TArea *Temp;
 Temp= AreaTemp;
 AreaTemp=NULL;
 delete  Temp;
 Insert->Enabled=true;
 Complete->Enabled=false;
 Cancel->Enabled=false;
 LoadARTCCBoundaries1->Enabled=true;
 //if (Areas->Count>0)  Delete->Enabled=true;
 //else   Delete->Enabled=false;

}
//---------------------------------------------------------------------------
void __fastcall TForm1::CompleteClick(TObject *Sender)
{

  int or1=orientation2D_Polygon( AreaTemp->Points,AreaTemp->NumPoints);
  if (or1==0)
   {
	ShowMessage("Degenerate Polygon");
    CancelClick(NULL);
	return;
   }
  if (or1==CLOCKWISE)
  {
	DWORD i;

	memcpy(AreaTemp->PointsAdj,AreaTemp->Points,sizeof(AreaTemp->Points));
	for (i = 0; i <AreaTemp->NumPoints; i++)
	 {
	   memcpy(AreaTemp->Points[i],
			 AreaTemp->PointsAdj[AreaTemp->NumPoints-1-i],sizeof( pfVec3));
	 }
  }
  if (checkComplex( AreaTemp->Points,AreaTemp->NumPoints))
   {
	ShowMessage("Polygon is Complex");
	CancelClick(NULL);
	return;
   }

  AreaConfirm->ShowDialog();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AreaListViewSelectItem(TObject *Sender, TListItem *Item,
      bool Selected)
{
   DWORD Count;
   TArea *AreaS=(TArea *)Item->Data;
   bool HaveSelected=false;
	Count=Areas->Count;
	for (unsigned int i = 0; i < Count; i++)
	 {
	   TArea *Area = (TArea *)Areas->Items[i];
	   if (Area==AreaS)
	   {
		if (Item->Selected)
		{
		 Area->Selected=true;
		 HaveSelected=true;
		}
		else
		 Area->Selected=false;
	   }
	   else
		 Area->Selected=false;

	 }
	if (HaveSelected)  Delete->Enabled=true;
	else Delete->Enabled=false;
	ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DeleteClick(TObject *Sender)
{
 int i = 0;

 while (i < AreaListView->Items->Count)
  {
	if (AreaListView->Items->Item[i]->Selected)
	{
	 TArea *Area;
	 int Index;

	 Area=(TArea *)AreaListView->Items->Item[i]->Data;
	 for (Index = 0; Index < Areas->Count; Index++)
	 {
	  if (Area==Areas->Items[Index])
	  {
	   Areas->Delete(Index);
	   AreaListView->Items->Item[i]->Delete();
	   TTriangles *Tri=Area->Triangles;
	   while(Tri)
	   {
		TTriangles *temp=Tri;
		Tri=Tri->next;
		free(temp->indexList);
		free(temp);
	   }
	   delete Area;
	   break;
	  }
	 }
	}
	else
	{
	  ++i;
	}
  }
 //if (Areas->Count>0)  Delete->Enabled=true;
 //else   Delete->Enabled=false;

 ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AreaListViewCustomDrawItem(TCustomListView *Sender,
	  TListItem *Item, TCustomDrawState State, bool &DefaultDraw)
{
   TRect   R;
   int Left;
  AreaListView->Canvas->Brush->Color = AreaListView->Color;
  AreaListView->Canvas->Font->Color = AreaListView->Font->Color;
  R=Item->DisplayRect(drBounds);
  AreaListView->Canvas->FillRect(R);

  AreaListView->Canvas->TextWidth(Item->Caption);

 AreaListView->Canvas->TextOut(2, R.Top, Item->Caption );

 Left = AreaListView->Column[0]->Width;

  for(int   i=0   ;i<Item->SubItems->Count;i++)
	 {
	  R=Item->DisplayRect(drBounds);
	  R.Left=R.Left+Left;
	   TArea *Area=(TArea *)Item->Data;
	  AreaListView->Canvas->Brush->Color=Area->Color;
	  AreaListView->Canvas->FillRect(R);
	 }

  if (Item->Selected)
	 {
	  R=Item->DisplayRect(drBounds);
	  AreaListView->Canvas->DrawFocusRect(R);
	 }
   DefaultDraw=false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DeleteAllAreas(void)
{
 int i = 0;

 while (AreaListView->Items->Count)
  {

	 TArea *Area;
	 int Index;

	 Area=(TArea *)AreaListView->Items->Item[i]->Data;
	 for (Index = 0; Index < Areas->Count; Index++)
	 {
	  if (Area==Areas->Items[Index])
	  {
	   Areas->Delete(Index);
	   AreaListView->Items->Item[i]->Delete();
	   TTriangles *Tri=Area->Triangles;
	   while(Tri)
	   {
		TTriangles *temp=Tri;
		Tri=Tri->next;
		free(temp->indexList);
		free(temp);
	   }
	   delete Area;
	   break;
	  }
	 }
  }

 ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseWheel(TObject *Sender, TShiftState Shift,
	  int WheelDelta, TPoint &MousePos, bool &Handled)
{
 if(g_EarthView){
	if (WheelDelta>0)
	  g_EarthView->SingleMovement(NAV_ZOOM_IN);
 	else g_EarthView->SingleMovement(NAV_ZOOM_OUT);
  	  ObjectDisplay->Repaint();
 }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Raw 데이터 수신 처리 (기존 TTCPClientRawHandleThread::HandleInput의 내용)
void __fastcall TForm1::HandleRawData(const AnsiString& data)
{
    FAircraftModel->ProcessRawMessage(data, CycleImages->Checked, NumSpriteImages);
}

//---------------------------------------------------------------------------
// Raw 연결 성공 시
void __fastcall TForm1::HandleRawConnected()
{
	FRawButtonScroller->UpdateCaption("Raw Disconnect");
    RawPlaybackButton->Enabled = false;
}

//---------------------------------------------------------------------------
// Raw 연결 종료 시
void __fastcall TForm1::HandleRawDisconnected(const String& reason)
{
	FRawButtonScroller->UpdateCaption("Raw Connect");
    RawPlaybackButton->Enabled = true;

    // 파일 재생 중이었다면 관련 상태도 초기화
    if (PlayBackRawStream) {
        delete PlayBackRawStream;
        PlayBackRawStream = NULL;
    }
    RawPlaybackButton->Caption = "Raw Playback";
    RawConnectButton->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::HandleRawReconnecting()
{
    // UI를 "재연결 중" 상태로 업데이트
	FRawButtonScroller->UpdateCaption("Reconnecting... (Cancel)");
    RawPlaybackButton->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RawConnectButtonClick(TObject *Sender)
{
	if (!FRawDataHandler->IsActive())
    {
		FRawButtonScroller->UpdateCaption("Connecting... (Cancel)");
        FRawDataHandler->Connect(RawIpAddress->Text, 30002, TConnectionType::Raw);
    }
    else
    {
        FRawDataHandler->Disconnect();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RawRecordButtonClick(TObject *Sender)
{
    if (RawRecordButton->Caption == "Raw Record")
    {
        if (RecordRawSaveDialog->Execute())
        {
            FRawDataHandler->StartRecording(RecordRawSaveDialog->FileName);
            RawRecordButton->Caption = "Stop Raw Recording";
        }
    }
    else
    {
        FRawDataHandler->StopRecording();
        RawRecordButton->Caption = "Raw Record";
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RawPlaybackButtonClick(TObject *Sender)
{
	if (!FRawDataHandler->IsActive())
    {
        if (PlaybackRawDialog->Execute())
        {
            FRawDataHandler->StartPlayback(PlaybackRawDialog->FileName);
        }
    }
    else
    {
        FRawDataHandler->Disconnect();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CycleImagesClick(TObject *Sender)
{
 CurrentSpriteImage=0;
 // AircraftDataModel의 FCurrentSpriteImage도 리셋
 if (FAircraftModel) {
     FAircraftModel->ResetCurrentSpriteImage();
 }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SBSConnectButtonClick(TObject *Sender)
{
	if (!FSBSDataHandler->IsActive())
    {
        FSBSDataHandler->Connect(SBSIpAddress->Text, 5002, TConnectionType::SBS);
    }
    else
    {
        FSBSDataHandler->Disconnect();
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TForm1::HandleSBSData(const AnsiString& data)
{
    if (BigQueryCSV)
    {
        BigQueryCSV->WriteLine(data);
        BigQueryRowCount++;
        if (BigQueryRowCount >= BIG_QUERY_UPLOAD_COUNT)
        {
            CloseBigQueryCSV();
            RunPythonScript(this->BigQueryPythonScript, this->BigQueryPath + " " + this->BigQueryCSVFileName);
            CreateBigQueryCSV();

            AnsiString HomeDir = ExtractFilePath(ExtractFileDir(Application->ExeName));
            AnsiString csvPath = HomeDir + "..\\BigQuery\\result.csv";
            if (FileExists(csvPath)) {
                TStreamReader *reader = new TStreamReader(csvPath, false);
                try {
                    FDeviationAircraftList.clear(); // 기존 목록 초기화
                    bool isFirstLine = true;
                    while (!reader->EndOfStream) {
                        AnsiString line = reader->ReadLine();
                        
                        // 첫 번째 라인(헤더)은 건너뛰기
                        if (isFirstLine) {
                            isFirstLine = false;
                            // 헤더 라인인지 확인 (Row, CENTROID_ID 등의 컬럼명이 포함되어 있으면)
                            if (line.Pos("Row") > 0 || line.Pos("CENTROID_ID") > 0 || 
                                line.Pos("HexIdent") > 0 || line.Pos("distance_from_centroid") > 0) {
                                continue; // 헤더 라인이므로 건너뛰기
                            }
                        }
                        
                        if (!line.IsEmpty() && line.Trim().Length() > 0) {
                            // CSV 형식 검증: 8개의 컬럼(Row, CENTROID_ID, Altitude, Latitude, Longitude, HexIdent, timestamp_utc, distance_from_centroid)이 있는지 확인
                            int commaCount = 0;
                            for (int i = 1; i <= line.Length(); i++) {
                                if (line[i] == ',') commaCount++;
                            }
                            FDeviationAircraftList.push_back(line);
                        }
                    }
                    // UI 업데이트
                    UpdateDeviationList();
                } __finally {
                    delete reader;
                }
            } else {
                FDeviationAircraftList.clear();
                UpdateDeviationList();
            }			
        }
    }

	FAircraftModel->ProcessSbsMessage(data, CycleImages->Checked, NumSpriteImages);
}

void __fastcall TForm1::HandleSBSConnected()
{
	FSBSButtonScroller->UpdateCaption("SBS Disconnect");
    SBSPlaybackButton->Enabled = false;
}

void __fastcall TForm1::HandleSBSDisconnected(const String& reason)
{
	FSBSButtonScroller->UpdateCaption("SBS Connect");
    SBSPlaybackButton->Enabled = true;
     if (PlayBackSBSStream) {
        delete PlayBackSBSStream;
        PlayBackSBSStream = NULL;
               SBSPlaybackButton->Caption = "SBS Playback";
        SBSConnectButton->Enabled = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::HandleSBSReconnecting()
{
	// UI를 "재연결 중" 상태로 업데이트
	FSBSButtonScroller->UpdateCaption("Reconnecting... (Cancel)");
	SBSPlaybackButton->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SBSRecordButtonClick(TObject *Sender)
{
    if (SBSRecordButton->Caption == "SBS Record")
    {
        if (RecordSBSSaveDialog->Execute())
        {
            if (FileExists(RecordSBSSaveDialog->FileName))
            {
                ShowMessage("File " + RecordSBSSaveDialog->FileName + "already exists. Cannot overwrite.");
            }
            else
            {
                FSBSDataHandler->StartRecording(RecordSBSSaveDialog->FileName);
                SBSRecordButton->Caption = "Stop SBS Recording";
            }
        }
    }
    else
    {
        FSBSDataHandler->StopRecording();
        SBSRecordButton->Caption = "SBS Record";
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SBSPlaybackButtonClick(TObject *Sender)
{
	if (!FSBSDataHandler->IsActive())
    {
        if (PlaybackSBSDialog->Execute())
        {
            FSBSDataHandler->StartPlayback(PlaybackSBSDialog->FileName);
        }
    }
    else
    {
        FSBSDataHandler->Disconnect();
    }

}
//---------------------------------------------------------------------------

void __fastcall TForm1::TimeToGoTrackBarChange(TObject *Sender)
{
  _int64 hmsm;
  hmsm=TimeToGoTrackBar->Position*1000;
  TimeToGoText->Caption=TimeToChar(hmsm);
}
//---------------------------------------------------------------------------
#include "MAPFactory.h"
#include "IMAPProvider.h"
#include <memory>
void __fastcall TForm1::LoadMap(int Type)
{
    // 1. Provider 생성
    AnsiString HomeDir = ExtractFilePath(ExtractFileDir(Application->ExeName));
	provider = MAPFactory::Create(static_cast<MapType>(Type), HomeDir.c_str(), LoadMapFromInternet);

	if (!provider) {
        LOG_ERROR(LogHandler::CAT_MAP, "Failed to create map provider - Unknown map type");
        throw Sysutils::Exception("Unknown map type");
    }

    // 2. 캐시 디렉터리는  생성
    std::string cachedir = provider->GetCacheDir();
    LOG_DEBUG_F(LogHandler::CAT_MAP, "Map cache directory: %s", cachedir.c_str());
    
    if (mkdir(cachedir.c_str()) != 0 && errno != EEXIST) {
        LOG_ERROR_F(LogHandler::CAT_MAP, "Failed to create cache directory: %s", cachedir.c_str());
	    throw Sysutils::Exception("Can not create cache directory");
    }

    g_Storage = new FilesystemStorage(cachedir, true);
	g_Keyhole = new KeyholeConnection(provider->GetURI());
	g_Keyhole->SetFetchTileCallback([p = provider.get()](TilePtr tile, KeyholeConnection* conn) {
		if (!p) {
			LOG_ERROR(LogHandler::CAT_MAP, "Provider is nullptr in tile fetch callback");
			return;
		}
        //printf("[Callback] LAMDA invoked!\n");
		p->FetchTile(tile, conn);
	});
	g_Keyhole->SetSaveStorage(g_Storage);
	g_Storage->SetNextLoadStorage(g_Keyhole);
	
    g_GETileManager = new TileManager(g_Storage);
    g_MasterLayer = new GoogleLayer(g_GETileManager);
    g_EarthView = new FlatEarthView(g_MasterLayer);
    g_EarthView->Resize(ObjectDisplay->Width, ObjectDisplay->Height);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MapComboBoxChange(TObject *Sender)
{
  double    m_Eyeh = g_EarthView ? g_EarthView->m_Eye.h : 0.0;
  double    m_Eyex = g_EarthView ? g_EarthView->m_Eye.x : 0.0;
  double    m_Eyey = g_EarthView ? g_EarthView->m_Eye.y : 0.0;

  Timer1->Enabled = false;
  Timer2->Enabled = false;

  // 해제 순서: 생성의 역순
  if (g_EarthView) {
    delete g_EarthView;
    g_EarthView = nullptr;
  }
  if (g_MasterLayer) {
    delete g_MasterLayer;
    g_MasterLayer = nullptr;
  }
  if (g_GETileManager) {
    delete g_GETileManager;
    g_GETileManager = nullptr;
  }
  //if (LoadMapFromInternet) {
    if (g_Keyhole) {
      delete g_Keyhole;
      g_Keyhole = nullptr;
    }
  //}
  if (g_Storage) {
    delete g_Storage;
    g_Storage = nullptr;
  }

  provider.reset();

  if (MapComboBox->ItemIndex == 0)      LoadMap(GoogleMaps);
  else if (MapComboBox->ItemIndex == 1) LoadMap(SkyVector_VFR);
  else if (MapComboBox->ItemIndex == 2) LoadMap(SkyVector_IFR_Low);
  else if (MapComboBox->ItemIndex == 3) LoadMap(SkyVector_IFR_High);
  else if (MapComboBox->ItemIndex == 4) LoadMap(OpenStreetMap);

  if (g_EarthView) {
	g_EarthView->m_Eye.h = m_Eyeh;
	g_EarthView->m_Eye.x = m_Eyex;
	g_EarthView->m_Eye.y = m_Eyey;
  }
  Timer1->Enabled = true;
  Timer2->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::BigQueryCheckBoxClick(TObject *Sender)
{
 if (BigQueryCheckBox->State==cbChecked) CreateBigQueryCSV();
 else {
	   CloseBigQueryCSV();
	   RunPythonScript(BigQueryPythonScript,BigQueryPath+" "+BigQueryCSVFileName);
	  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CreateBigQueryCSV(void)
{
    AnsiString  HomeDir = ExtractFilePath(ExtractFileDir(Application->ExeName));
    BigQueryCSVFileName="BigQuery"+UIntToStr(BigQueryFileCount)+".csv";
    BigQueryRowCount=0;
    BigQueryFileCount++;
    BigQueryCSV=new TStreamWriter(HomeDir+"..\\BigQuery\\"+BigQueryCSVFileName, false);
    if (BigQueryCSV==NULL)
	  {
		ShowMessage("Cannot Open BigQuery CSV File "+HomeDir+"..\\BigQuery\\"+BigQueryCSVFileName);
        BigQueryCheckBox->State=cbUnchecked;
	  }
	AnsiString Header=AnsiString("Message Type,Transmission Type,SessionID,AircraftID,HexIdent,FlightID,Date_MSG_Generated,Time_MSG_Generated,Date_MSG_Logged,Time_MSG_Logged,Callsign,Altitude,GroundSpeed,Track,Latitude,Longitude,VerticalRate,Squawk,Alert,Emergency,SPI,IsOnGround");
	BigQueryCSV->WriteLine(Header);
}
//--------------------------------------------------------------------------
void __fastcall TForm1::CloseBigQueryCSV(void)
{
    if (BigQueryCSV)
    {
     delete BigQueryCSV;
     BigQueryCSV=NULL;
    }
}
//--------------------------------------------------------------------------
	 static void RunPythonScript(AnsiString scriptPath,AnsiString args)
     {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        AnsiString commandLine = "python " + scriptPath+" "+args;
        char* cmdLineCharArray = new char[strlen(commandLine.c_str()) + 1];
		strcpy(cmdLineCharArray, commandLine.c_str());
	#define  LOG_PYTHON 1
	#if LOG_PYTHON
        //printf("%s\n", cmdLineCharArray);
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
	    sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
		HANDLE h = CreateFileA(Form1->BigQueryLogFileName.c_str(),
		FILE_APPEND_DATA,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        &sa,
		OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
		NULL );

        si.hStdInput = NULL;
	    si.hStdOutput = h;
	    si.hStdError = h; // Redirect standard error as well, if needed
	    si.dwFlags |= STARTF_USESTDHANDLES;
    #endif
        if (!CreateProcessA(
            nullptr,          // No module name (use command line)
            cmdLineCharArray, // Command line
            nullptr,          // Process handle not inheritable
            nullptr,          // Thread handle not inheritable
	 #if LOG_PYTHON
            TRUE,
     #else
            FALSE,            // Set handle inheritance to FALSE
     #endif
            CREATE_NO_WINDOW, // Don't create a console window
			nullptr,          // Use parent's environment block
            nullptr,          // Use parent's starting directory
            &si,             // Pointer to STARTUPINFO structure
            &pi))             // Pointer to PROCESS_INFORMATION structure
         {
            std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
            delete[] cmdLineCharArray;
            return;
         }

        // Optionally, detach from the process
        CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		delete[] cmdLineCharArray;
    }

 //--------------------------------------------------------------------------
void __fastcall TForm1::UseSBSRemoteClick(TObject *Sender)
{
 SBSIpAddress->Text="data.adsbhub.org";
}
//---------------------------------------------------------------------------

void __fastcall TForm1::UseSBSLocalClick(TObject *Sender)
{
 SBSIpAddress->Text="128.237.96.41";
}
//---------------------------------------------------------------------------
static bool DeleteFilesWithExtension(AnsiString dirPath, AnsiString extension)
 {
	AnsiString searchPattern = dirPath + "\\*." + extension;
	WIN32_FIND_DATAA findData;

	HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return false; // No files found or error
	}

	do {
		AnsiString filePath = dirPath + "\\" + findData.cFileName;
		if (DeleteFileA(filePath.c_str()) == 0) {
			FindClose(hFind);
			return false; // Failed to delete a file
		}
	} while (FindNextFileA(hFind, &findData) != 0);

	FindClose(hFind);
	return true;
}
static bool IsFirstRow=true;
static bool CallBackInit=false;
//---------------------------------------------------------------------------
 static int CSV_callback_ARTCCBoundaries (struct CSV_context *ctx, const char *value)
{
  int    rc = 1;
  static char LastArea[512];
  static char Area[512];
  static char Lat[512];
  static char Lon[512];
  int    Deg,Min,Sec,Hsec;
  char   Dir;

   if (ctx->field_num==0)
   {
	strcpy(Area,value);
   }
   else if (ctx->field_num==3)
   {
	strcpy(Lat,value);
   }
   else if (ctx->field_num==4)
   {
    strcpy(Lon,value);
   }

   if (ctx->field_num == (ctx->num_fields - 1))
   {

	float fLat, fLon;
   if (!IsFirstRow)
   {
	 if (!CallBackInit)
	 {
	  strcpy(LastArea,Area);
	  CallBackInit=true;
	 }
	   if(strcmp(LastArea,Area)!=0)
		{

		 if (FinshARTCCBoundary())
		   {
			LOG_ERROR_F(LogHandler::CAT_MAP, "Failed to load area ID: %s", LastArea);
		   }
		 else LOG_INFO_F(LogHandler::CAT_MAP, "Successfully loaded area ID: %s", LastArea);
		 strcpy(LastArea,Area);
		 }
	   if (Form1->AreaTemp==NULL)
		   {
			Form1->AreaTemp= new TArea;
			Form1->AreaTemp->NumPoints=0;
			Form1->AreaTemp->Name=Area;
			Form1->AreaTemp->Selected=false;
			Form1->AreaTemp->Triangles=NULL;
			 LOG_INFO_F(LogHandler::CAT_MAP, "Loading area ID: %s", Area);
		   }
	   if (sscanf(Lat,"%2d%2d%2d%2d%c",&Deg,&Min,&Sec,&Hsec,&Dir)!=5)
		 printf("Latitude Parse Error\n");
	   fLat=Deg+Min/60.0+Sec/3600.0+Hsec/360000.00;
	   if (Dir=='S') fLat=-fLat;

	   if (sscanf(Lon,"%3d%2d%2d%2d%c",&Deg,&Min,&Sec,&Hsec,&Dir)!=5)
		 printf("Longitude Parse Error\n");
	   fLon=Deg+Min/60.0+Sec/3600.0+Hsec/360000.00;
	   if (Dir=='W') fLon=-fLon;
	   //printf("%f, %f\n",fLat,fLon);
	   if (Form1->AreaTemp->NumPoints<MAX_AREA_POINTS)
	   {
		Form1->AreaTemp->Points[Form1->AreaTemp->NumPoints][1]=fLat;
		Form1->AreaTemp->Points[Form1->AreaTemp->NumPoints][0]=fLon;
		Form1->AreaTemp->Points[Form1->AreaTemp->NumPoints][2]=0.0;
		Form1->AreaTemp->NumPoints++;
	   }
		else printf("Max Area Points Reached\n");

   }
   if (IsFirstRow) IsFirstRow=false;
   }
  return(rc);
}
//---------------------------------------------------------------------------
bool __fastcall TForm1::LoadARTCCBoundaries(AnsiString FileName)
{
  CSV_context  csv_ctx;
   memset (&csv_ctx, 0, sizeof(csv_ctx));
   csv_ctx.file_name = FileName.c_str();
   csv_ctx.delimiter = ',';
   csv_ctx.callback  = CSV_callback_ARTCCBoundaries;
   csv_ctx.line_size = 2000;
   IsFirstRow=true;
   CallBackInit=false;
   if (!CSV_open_and_parse_file(&csv_ctx))
    {
	  LOG_ERROR_F(LogHandler::CAT_MAP, "Parsing of file failed: %s - %s", FileName.c_str(), strerror(errno));
      return (false);
	}
   if ((Form1->AreaTemp!=NULL) && (Form1->AreaTemp->NumPoints>0))
   {
     char Area[512];
     strcpy(Area,Form1->AreaTemp->Name.c_str());
     if (FinshARTCCBoundary())
	    {
        LOG_ERROR_F(LogHandler::CAT_MAP, "Error finalizing area ID: %s", Area);
	    }
        else LOG_INFO_F(LogHandler::CAT_MAP, "Successfully finalized area ID: %s", Area);
   }
   printf("Done\n");
   return(true);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LoadARTCCBoundaries1Click(TObject *Sender)
{
   LoadARTCCBoundaries(ARTCCBoundaryDataPathFileName);
}
//---------------------------------------------------------------------------
static int FinshARTCCBoundary(void)
{
  int or1=orientation2D_Polygon( Form1->AreaTemp->Points,Form1->AreaTemp->NumPoints);
  if (or1==0)
   {
	TArea *Temp;
	Temp= Form1->AreaTemp;
	Form1->AreaTemp=NULL;
	delete  Temp;
	printf("Degenerate Polygon\n");
	return(-1);
   }
  if (or1==CLOCKWISE)
  {
	DWORD i;

	memcpy(Form1->AreaTemp->PointsAdj,Form1->AreaTemp->Points,sizeof(Form1->AreaTemp->Points));
	for (i = 0; i <Form1->AreaTemp->NumPoints; i++)
	 {
	   memcpy(Form1->AreaTemp->Points[i],
			 Form1->AreaTemp->PointsAdj[Form1->AreaTemp->NumPoints-1-i],sizeof( pfVec3));
	 }
  }
  if (checkComplex( Form1->AreaTemp->Points,Form1->AreaTemp->NumPoints))
   {
	TArea *Temp;
	Temp= Form1->AreaTemp;
	Form1->AreaTemp=NULL;
	delete  Temp;
	printf("Polygon is Complex\n");
    return(-2);
   }
  DWORD Row,Count,i;


 Count=Form1->Areas->Count;
 for (i = 0; i < Count; i++)
 {
  TArea *Area = (TArea *)Form1->Areas->Items[i];
  if (Area->Name==Form1->AreaTemp->Name) {

   TArea *Temp;
   Temp= Form1->AreaTemp;
   LOG_WARNING_F(LogHandler::CAT_MAP, "Duplicate area name: %s", Form1->AreaTemp->Name.c_str());
   Form1->AreaTemp=NULL;
   delete  Temp;
   return(-3);
   }
 }

 triangulatePoly(Form1->AreaTemp->Points,Form1->AreaTemp->NumPoints,
				 &Form1->AreaTemp->Triangles);

 Form1->AreaTemp->Color=TColor(PopularColors[CurrentColor]);
 CurrentColor++ ;
 CurrentColor=CurrentColor%NumColors;
 Form1->Areas->Add(Form1->AreaTemp);
 Form1->AreaListView->Items->BeginUpdate();
 Form1->AreaListView->Items->Add();
 Row=Form1->AreaListView->Items->Count-1;
 Form1->AreaListView->Items->Item[Row]->Caption=Form1->AreaTemp->Name;
 Form1->AreaListView->Items->Item[Row]->Data=Form1->AreaTemp;
 Form1->AreaListView->Items->Item[Row]->SubItems->Add("");
 Form1->AreaListView->Items->EndUpdate();
 Form1->AreaTemp=NULL;
 return 0 ;
}
//---------------------------------------------------------------------------

// 공항 정보 찾기 (ICAO 코드로)
/*
const AirportInfo* FindAirportByIcao(const std::string& icao) {
    for (const auto& ap : apiAirportList) {
        if (ap.icao == icao)
            return &ap;
    }
    return nullptr;
}*/

// 항공기 Callsign으로 RouteInfo 찾기
const RouteInfo* FindRouteByCallsign(const std::string& callSign) {
    for (const auto& route : apiRouteList) {
        if (route.callSign == callSign)
            return &route;
    }
    return nullptr;
}

// ICAO 코드 uint32 → 6자리 대문자  16진 문자열로 변환
std::string ICAO_to_string(uint32_t icao) {
    char buf[7] = {0};
    snprintf(buf, sizeof(buf), "%06X", icao);
    return std::string(buf);
}

// ========================
// 2. 메인 패널 갱신 함수
// ========================

static const char* GetAircraftType(enum AircraftCategory cate) {
    
        switch (cate) {
            case CATEGORY_COMMERCIAL:
                return "COMMERCIAL";
            case CATEGORY_CARGO:
                return "CARGO";
            case CATEGORY_HELICOPTER:
                return "HELICOPTER";
            case CATEGORY_MILITARY:
                return "MILITARY";
            case CATEGORY_BUSINESS_JET:
                return "BUSINESS_JET";
            case CATEGORY_GENERAL_AVIATION:
                return "GENERAL_AVIATION";
            case CATEGORY_GLIDER:
                return "GLIDER";
            case CATEGORY_ULTRALIGHT:
                return "ULTRALIGHT";
            case CATEGORY_UNKNOWN:
            default:
                return "UNKNOWN";
        }
}

void __fastcall TForm1::UpdateCloseControlPanel(TADS_B_Aircraft* ac, const RouteInfo* route)
{
    // -------- 기존 항공기 정보 갱신 --------
    if (!ac) {
        // 패널 초기화 (None 처리)
        ICAOLabel->Caption      = "N/A";
        FlightNumLabel->Caption = "N/A";
        AircraftModelLabel->Caption = "N/A";
        AirlineNameLabel->Caption = "N/A";
        AirlineCountryLabel->Caption = "N/A";
        CLatLabel->Caption      = "N/A";
        CLonLabel->Caption      = "N/A";
        SpdLabel->Caption       = "N/A";
        HdgLabel->Caption       = "N/A";
        AltLabel->Caption       = "N/A";
        MsgCntLabel->Caption    = "N/A";
        TrkLastUpdateTimeLabel->Caption = "N/A";
        RouteInfoMemo->Lines->Text = "N/A";
        RouteInfoMemo->Hint = "";          // 툴팁도 동일하게
        RouteInfoMemo->ShowHint = false;
        return;
    }

    ICAOLabel->Caption      = ac->HexAddr;         // ICAO(16진)
    FlightNumLabel->Caption = ac->FlightNum;       // Callsign
    AircraftModelLabel->Caption = GetAircraftType(ac->Category);
    {
        std::string airline, country;
        if (LookupAirline(AnsiString(ac->FlightNum).Trim().c_str(), airline, country))
        {
            AirlineNameLabel->Caption = airline.c_str();
            AirlineCountryLabel->Caption = country.c_str();
        }
        else
        {
            AirlineNameLabel->Caption = "N/A";
            AirlineCountryLabel->Caption = "N/A";
        }
    }
    if (ac->HaveLatLon) {
        CLatLabel->Caption = DMS::DegreesMinutesSecondsLat(ac->Latitude).c_str();
        CLonLabel->Caption = DMS::DegreesMinutesSecondsLon(ac->Longitude).c_str();
    } else {
        CLatLabel->Caption = "N/A";
        CLonLabel->Caption = "N/A";
    }
    if (ac->HaveSpeedAndHeading) {
        SpdLabel->Caption = FloatToStrF(ac->Speed, ffFixed, 12, 2) + " KTS";
        Form1->HdgLabel->Caption = FloatToStrF(ac->Heading, ffFixed, 12, 2) + " DEG";
    } else {
        SpdLabel->Caption = "N/A";
        HdgLabel->Caption = "N/A";
    }
    if (ac->Altitude)
        AltLabel->Caption = FloatToStrF(ac->Altitude, ffFixed, 12, 2) + " FT";
    else
        AltLabel->Caption = "N/A";
    MsgCntLabel->Caption = "Raw: " + IntToStr((int)ac->NumMessagesRaw) +
                           " SBS: " + IntToStr((int)ac->NumMessagesSBS);
    TrkLastUpdateTimeLabel->Caption = TimeToChar(ac->LastSeen);

    // -------- 경로(Route) 정보 표시부 --------
    AnsiString routeText, toolTipText;

    if (route) {
        // 화면에는 공항 코드만
        for (size_t i = 0; i < route->airportCodes.size(); ++i) {
            routeText += route->airportCodes[i].c_str();
            if (i < route->airportCodes.size() - 1)
                routeText += "\n  ↓\n";
        }
        // 툴팁에는 전체 공항명 + Airline, Flight No 등 자세한 정보
        for (size_t i = 0; i < route->airportCodes.size(); ++i) {
            const AirportInfo* ap = nullptr;
            auto it = icaoToAirport.find(route->airportCodes[i]);
            if (it != icaoToAirport.end()) ap = it->second;
            AnsiString codeOnly = route->airportCodes[i].c_str();
            AnsiString fullName = ap ? AnsiString(ap->name.c_str()) : "";
            if (!toolTipText.IsEmpty()) toolTipText += "\n↓\n";
            toolTipText += codeOnly;
            if (!fullName.IsEmpty())
                toolTipText += " (" + fullName + ")";
        }
        // 추가 정보 (Airline/Flight No)는 툴팁에만 추가
//        if (!AnsiString(route->airlineCode.c_str()).IsEmpty())
//            toolTipText += "\n\nAirline: " + AnsiString(route->airlineCode.c_str());
//        if (!AnsiString(route->number.c_str()).IsEmpty())
//            toolTipText += "\nFlight No: " + AnsiString(route->number.c_str());
    } else {
        routeText = "N/A";
        toolTipText = "";
    }

    RouteInfoMemo->Lines->Text = routeText;
    RouteInfoMemo->Hint = toolTipText;
    RouteInfoMemo->ShowHint = !toolTipText.IsEmpty();
}

// ========================
// 3. 항공기 선택 이벤트에서 호출
// ========================

// 항공기 선택(예: HookTrack에서 TrackHook.ICAO_CC가 확정될 때) 호출 예시:
void __fastcall TForm1::OnAircraftSelected(uint32_t icao)
{
    // 항공기 객체 찾기
    TADS_B_Aircraft* ac = FAircraftModel->FindAircraftByICAO(icao);
    const RouteInfo* route = nullptr;
    m_selectedRoutePaths.clear();

	if (ac){
        // **배열을 AnsiString으로 변환한 뒤 처리!**
        AnsiString flightNum = AnsiString(ac->FlightNum).Trim();
        auto it = callSignToRoute.find(flightNum.c_str());
        route = (it != callSignToRoute.end()) ? it->second : nullptr;

        // 경로 점들 계산
        if (route && route->airportCodes.size() >= 2) {
            size_t count = std::min<size_t>(route->airportCodes.size() - 1, 2);
            for (size_t i = 0; i < count; ++i) {
                auto it1 = icaoToAirport.find(route->airportCodes[i]);
                auto it2 = icaoToAirport.find(route->airportCodes[i + 1]);
                if (it1 != icaoToAirport.end() && it2 != icaoToAirport.end()) {
                    auto segs = BuildRouteSegment(it1->second->latitude, it1->second->longitude,
                                                it2->second->latitude, it2->second->longitude);
                    for (auto& s : segs)
                        if (!s.empty()) m_selectedRoutePaths.push_back(std::move(s));
                }
            }
        }
    }

    // 패널 전체 갱신 (기존 Close 라벨 + 경로)
    UpdateCloseControlPanel(ac, route);
    ObjectDisplay->Repaint();
}

void __fastcall TForm1::FilterAirlineEditChange(TObject *Sender)
{
    filterAirline = FilterAirlineEdit->Text.Trim().UpperCase();
    ObjectDisplay->Repaint();
}
void __fastcall TForm1::FilterOriginEditChange(TObject *Sender)
{
    filterOrigin = FilterOriginEdit->Text.Trim().UpperCase();
    LOG_DEBUG_F(LogHandler::CAT_GENERAL, "Filter origin changed: %s", filterOrigin.c_str());
    ObjectDisplay->Repaint();
}
void __fastcall TForm1::FilterDestinationEditChange(TObject *Sender)
{
    filterDestination = FilterDestinationEdit->Text.Trim().UpperCase();
    ObjectDisplay->Repaint();
}

void __fastcall TForm1::FilterPolygonOnlyCheckBoxClick(TObject *Sender)
{
    filterPolygonOnly = FilterPolygonOnlyCheckBox->Checked;
    ObjectDisplay->Repaint();
}

// Helper: Map trackbar position to playback speed (e.g., 1=0.5x, 2=1x, 3=2x, 4=4x)
double TrackBarPosToSpeed(int pos) {
    switch (pos) {
        case 0: return 1.0;
        case 1: return 2.0;
        case 2: return 3.0;
        default: return 1.0;
    }
}

// --- Playback Speed UI Setup ---
void TForm1::SetupPlaybackSpeedUI()
{
    // .dfm에서 생성된 UI를 초기화
    if (PlaybackSpeedComboBox) {
        PlaybackSpeedComboBox->ItemIndex = 0;
        // 이벤트 핸들러 동적 할당
        PlaybackSpeedComboBox->OnChange = PlaybackSpeedComboBoxChange;
    }
    
    // 최초 1회 적용
    if (FRawDataHandler) FRawDataHandler->SetPlaybackSpeed(1.0);
    if (FSBSDataHandler) FSBSDataHandler->SetPlaybackSpeed(1.0);
    
    if (PlaybackSpeedLabel) {
        PlaybackSpeedLabel->Caption = "Playback Speed: 1x";
    }
}

void __fastcall TForm1::PlaybackSpeedComboBoxChange(TObject *Sender)
{
    if (!PlaybackSpeedComboBox) return;
    
    int idx = PlaybackSpeedComboBox->ItemIndex;
    double speed = 1.0;
    if (idx == 1) speed = 2.0;
    else if (idx == 2) speed = 3.0;
    
    if (FRawDataHandler) FRawDataHandler->SetPlaybackSpeed(speed);
    if (FSBSDataHandler) FSBSDataHandler->SetPlaybackSpeed(speed);
    
    if (PlaybackSpeedLabel) {
        PlaybackSpeedLabel->Caption = "Playback Speed: " + FloatToStrF(speed, ffGeneral, 3, 2) + "x";
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AssessmentTimerTimer(TObject *Sender)
{
	std::vector<TADS_B_Aircraft*> snapshot_pointers;
    snapshot_pointers.reserve(FAircraftModel->GetAircraftCount());

    ght_iterator_t iterator;
    const void* key;
    TADS_B_Aircraft* aircraft;
    for (aircraft = FAircraftModel->GetFirstAircraft(&iterator, &key);
         aircraft;
         aircraft = FAircraftModel->GetNextAircraft(&iterator, &key))
    {
        if (aircraft->HaveLatLon) {
			snapshot_pointers.push_back(aircraft);
        }
    }
    
    // 충돌 필터 설정값을 임계값으로 사용
    double horizontalThreshold = m_horizontalMaxDistance;  // NM
    double verticalThreshold = m_verticalMaxDistance;      // feet
    int timeThreshold = (int)m_tcpaMaxThreshold;           // seconds
    double minTimeThreshold = m_tcpaMinThreshold;          // seconds
    
    FProximityAssessor->startAssessment(snapshot_pointers, horizontalThreshold, verticalThreshold, timeThreshold, minTimeThreshold);
}

void __fastcall TForm1::OnAssessmentComplete(TObject *Sender)
{
    FConflictMap.clear();
    FSortedConflictList = FProximityAssessor->SortedListResults;
    FConflictMap = FProximityAssessor->MapResults;

    UpdateConflictList();
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateConflictList()
{
    // 충돌 상태 초기화
    m_hasCriticalConflicts = false;
    m_hasHighConflicts = false;
    
    // 현재 선택된 항목의 ICAO 쌍을 기억
    std::pair<unsigned int, unsigned int> selectedPair = {0, 0};
    if (ConflictListView->Selected && ConflictListView->Selected->Data) {
        __int64 packedICAOs = (__int64)ConflictListView->Selected->Data;
        selectedPair.first = (unsigned int)(packedICAOs >> 32);
        selectedPair.second = (unsigned int)(packedICAOs & 0xFFFFFFFF);
    }
    
    // FSortedConflictList는 이미 백그라운드 스레드에서 위험도 순으로 완벽하게 정렬되어 있습니다.
    ConflictListView->Items->BeginUpdate();
    ConflictListView->Items->Clear();

    // 스레드가 정렬해준 순서 그대로 리스트뷰에 추가합니다.
    for (const auto& conflict : FSortedConflictList)
    {
        // "N/A" 제거 로직: 항목을 추가하기 전에 두 항공기가 모두 존재하는지 확인
        TADS_B_Aircraft* ac1 = FAircraftModel->FindAircraftByICAO(conflict.ICAO1);
        TADS_B_Aircraft* ac2 = FAircraftModel->FindAircraftByICAO(conflict.ICAO2);

        if (ac1 && ac2)
        {
            TListItem* item = ConflictListView->Items->Add();

            item->Caption = ac1->HexAddr;
            item->SubItems->Add(ac2->HexAddr);

            // 정밀한 시간 표시 로직
            AnsiString tcpaStr;
            if (conflict.timeToCPA < 60.0) {
                tcpaStr.sprintf("%.1f s", conflict.timeToCPA);
            } else {
                int minutes = (int)conflict.timeToCPA / 60;
                int seconds = (int)conflict.timeToCPA % 60;
                tcpaStr.sprintf("%02d:%02d", minutes, seconds);
            }
            item->SubItems->Add(tcpaStr);

            item->SubItems->Add(FloatToStrF(conflict.cpaDistance, ffFixed, 4, 1));
            item->SubItems->Add(FloatToStrF(conflict.verticalCPA, ffFixed, 5, 0));

            //item->SubItems->Add(FloatToStrF(conflict.threatScore, ffFixed, 6, 1));

            double score = conflict.threatScore;
            AnsiString levelStr;
            
            if (score > 8.3) {
                levelStr = "Critical";
                m_hasCriticalConflicts = true;
            }
            else if (score > 6.5) {
                levelStr = "High";
                m_hasHighConflicts = true;
            }
            else if (score > 4.0) {
                levelStr = "Medium";
            }
            else {
                levelStr = "Low";
            }

            // 위험도 레벨만 표시 (점수 제거)
            item->SubItems->Add(levelStr);
            
            // 위험도 레벨을 Data에 저장 (나중에 색상 적용을 위해)
            item->Data = (void*)((__int64)conflict.ICAO1 << 32 | conflict.ICAO2);
        }
    }
    ConflictListView->Items->EndUpdate();
    
    // 이전에 선택되었던 항목을 다시 선택
    if (selectedPair.first != 0 && selectedPair.second != 0) {
        for (int i = 0; i < ConflictListView->Items->Count; i++) {
            TListItem* item = ConflictListView->Items->Item[i];
            if (item->Data) {
                __int64 packedICAOs = (__int64)item->Data;
                unsigned int icao1 = (unsigned int)(packedICAOs >> 32);
                unsigned int icao2 = (unsigned int)(packedICAOs & 0xFFFFFFFF);
                
                if ((icao1 == selectedPair.first && icao2 == selectedPair.second) ||
                    (icao1 == selectedPair.second && icao2 == selectedPair.first)) {
                    item->Selected = true;
                    item->Focused = true;
                    ConflictListView->Selected = item;
                    break;
                }
            }
        }
    }
    
    // 깜빡임 상태 업데이트
    UpdateConflictStatusColors();
}

// ConflictListViewSelectItem 함수 (변경 없음, 이전과 동일)
void __fastcall TForm1::ConflictListViewSelectItem(TObject *Sender, TListItem *Item, bool Selected)
{
    if (Selected && Item && Item->Data)
    {
        // Data 포인터에서 두 ICAO 값을 다시 추출
        __int64 packedICAOs = (__int64)Item->Data;
        unsigned int icao1 = (unsigned int)(packedICAOs >> 32);
        unsigned int icao2 = (unsigned int)(packedICAOs & 0xFFFFFFFF);

        // [수정] 새로 추가된 멤버 변수에 선택된 쌍의 정보를 저장
        FSelectedConflictPair = {icao1, icao2};

        // [추가] 두 항공기의 중간 지점으로 지도 중심 이동
        CenterMapOnPair(icao1, icao2);

        // 일반 선택(TrackHook)은 해제하여 중복 하이라이트 방지
        TrackHook.Valid_CC = false;

        ObjectDisplay->Repaint();
        
        // 선택 상태 색상 업데이트
        ConflictListView->Invalidate();
    }
    else
    {
        // 선택이 해제되면, 쌍 선택도 해제
        FSelectedConflictPair = {0, 0};
        ObjectDisplay->Repaint();
        
        // 선택 해제 상태 색상 업데이트
        ConflictListView->Invalidate();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CenterMapOnPair(unsigned int icao1, unsigned int icao2)
{
    TADS_B_Aircraft* ac1 = FAircraftModel->FindAircraftByICAO(icao1);
    TADS_B_Aircraft* ac2 = FAircraftModel->FindAircraftByICAO(icao2);

    if (ac1 && ac2 && ac1->HaveLatLon && ac2->HaveLatLon)
    {
        // 두 항공기의 위도/경도 평균 계산
        double centerLat = (ac1->Latitude + ac2->Latitude) / 2.0;
        double centerLon = (ac1->Longitude + ac2->Longitude) / 2.0;

        // g_EarthView를 직접 조작하여 지도 중심 이동 (기존 코드 방식 활용)
        if(g_EarthView) {
            MapCenterLat = centerLat;
            MapCenterLon = centerLon;
            SetMapCenter(g_EarthView->m_Eye.x, g_EarthView->m_Eye.y);
        }
    }
}

void __fastcall TForm1::ObjectDisplayDblClick(TObject *Sender)
{
  // 오른쪽 마우스 더블클릭 시 폴리곤 완성
  if (AreaTemp && AreaTemp->NumPoints >= 3)
  {
    CompleteClick(NULL);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedFilterTrackBarChange(TObject *Sender)
{
    int minSpeed = SpeedMinTrackBar->Position;
    int maxSpeed = SpeedMaxTrackBar->Position;
    
    // MIN/MAX 값 검증 및 자동 조정
    if (Sender == SpeedMinTrackBar) {
        // MIN 슬라이더가 움직인 경우, MAX보다 크면 MAX를 MIN에 맞춤
        if (minSpeed > maxSpeed) {
            SpeedMaxTrackBar->Position = minSpeed;
            maxSpeed = minSpeed;
        }
    } else if (Sender == SpeedMaxTrackBar) {
        // MAX 슬라이더가 움직인 경우, MIN보다 작으면 MIN을 MAX에 맞춤
        if (maxSpeed < minSpeed) {
            SpeedMinTrackBar->Position = maxSpeed;
            minSpeed = maxSpeed;
        }
    }
    
    SpeedFilterLabel->Caption = "Speed: " + IntToStr(minSpeed) + " ~ " + IntToStr(maxSpeed) + " knots";
    
    // 화면 다시 그리기 (필터 적용)
    ObjectDisplay->Repaint();
}

void __fastcall TForm1::AltitudeFilterTrackBarChange(TObject *Sender)
{
    int minAlt = AltitudeMinTrackBar->Position;
    int maxAlt = AltitudeMaxTrackBar->Position;
    
    // MIN/MAX 값 검증 및 자동 조정
    if (Sender == AltitudeMinTrackBar) {
        // MIN 슬라이더가 움직인 경우, MAX보다 크면 MAX를 MIN에 맞춤
        if (minAlt > maxAlt) {
            AltitudeMaxTrackBar->Position = minAlt;
            maxAlt = minAlt;
        }
    } else if (Sender == AltitudeMaxTrackBar) {
        // MAX 슬라이더가 움직인 경우, MIN보다 작으면 MIN을 MAX에 맞춤
        if (maxAlt < minAlt) {
            AltitudeMinTrackBar->Position = maxAlt;
            minAlt = maxAlt;
        }
    }
    
    AltitudeFilterLabel->Caption = "Altitude: " + IntToStr(minAlt) + " ~ " + IntToStr(maxAlt) + " ft";
    
    // 화면 다시 그리기 (필터 적용)
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AircraftCategoryFilterChange(TObject *Sender)
{
    // 항공기 카테고리 필터 변경 시 화면 다시 그리기
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
// --- 충돌 필터 관련 함수들 ---

//---------------------------------------------------------------------------
void __fastcall TForm1::TCPAFilterTrackBarChange(TObject *Sender)
{
    int minTCPA = TCPAMinTrackBar->Position;
    int maxTCPA = TCPAMaxTrackBar->Position;
    
    // MIN/MAX 값 검증 및 조정
    if (Sender == TCPAMinTrackBar) {
        if (minTCPA > maxTCPA) {
            TCPAMaxTrackBar->Position = minTCPA;
            maxTCPA = minTCPA;
        }
    } else if (Sender == TCPAMaxTrackBar) {
        if (maxTCPA < minTCPA) {
            TCPAMinTrackBar->Position = maxTCPA;
            minTCPA = maxTCPA;
        }
    }
    
    m_tcpaMinThreshold = minTCPA;
    m_tcpaMaxThreshold = maxTCPA;
    
    // 시간 표시 형식 개선 (분:초 형식)
    AnsiString minStr, maxStr;
    if (minTCPA < 60.0) {
        minStr.sprintf("%.1f s", minTCPA);
    } else {
        int minutes = (int)minTCPA / 60;
        int seconds = (int)minTCPA % 60;
        minStr.sprintf("%02d:%02d", minutes, seconds);
    }
    
    if (maxTCPA < 60) {
        maxStr = IntToStr(maxTCPA) + "s";
    } else {
        int minutes = maxTCPA / 60;
        int seconds = maxTCPA % 60;
        maxStr.sprintf("%d:%02d", minutes, seconds);
    }
    
    TCPAThresholdLabel->Caption = "TCPA: " + minStr + " ~ " + maxStr;
    
    // TrackBar 값 변경 시 즉시 새로운 충돌 평가 시작
    AssessmentTimerTimer(NULL);
}

//---------------------------------------------------------------------------
void __fastcall TForm1::HorizontalDistanceFilterTrackBarChange(TObject *Sender)
{
    int maxDist = HorizontalMaxTrackBar->Position;
    
    m_horizontalMinDistance = 0.0;  // 항상 0으로 고정
    m_horizontalMaxDistance = maxDist * 0.1;  // 0.1 NM 단위로 변환
    
    HorizontalDistanceLabel->Caption = "H Dist: 0 ~ " + 
        FormatFloat("0.0", m_horizontalMaxDistance) + " NM";
        
    // TrackBar 값 변경 시 즉시 새로운 충돌 평가 시작
    AssessmentTimerTimer(NULL);
}

//---------------------------------------------------------------------------
void __fastcall TForm1::VerticalDistanceFilterTrackBarChange(TObject *Sender)
{
    int maxDist = VerticalMaxTrackBar->Position;
    
    m_verticalMinDistance = 0.0;  // 항상 0으로 고정
    m_verticalMaxDistance = maxDist;
    
    VerticalDistanceLabel->Caption = "V Dist: 0 ~ " + IntToStr(maxDist) + " ft";
    
    // TrackBar 값 변경 시 즉시 새로운 충돌 평가 시작
    AssessmentTimerTimer(NULL);
}

//---------------------------------------------------------------------------
void __fastcall TForm1::ShowConflictAircraftAlwaysCheckBoxClick(TObject *Sender)
{
    m_showConflictAircraftAlways = ShowConflictAircraftAlwaysCheckBox->Checked;
    
    // "충돌감지된 항공기만 표시"와 배타적 관계
    if (m_showConflictAircraftAlways && m_showOnlyConflictAircraft) {
        m_showOnlyConflictAircraft = false;
        ShowOnlyConflictAircraftCheckBox->Checked = false;
    }
    
    // 화면 새로고침
    ObjectDisplay->Repaint();
}

//---------------------------------------------------------------------------
void __fastcall TForm1::ShowOnlyConflictAircraftCheckBoxClick(TObject *Sender)
{
    m_showOnlyConflictAircraft = ShowOnlyConflictAircraftCheckBox->Checked;
    
    // "충돌감지된 항공기는 필터 관계없이 항상 표시"와 배타적 관계
    if (m_showOnlyConflictAircraft && m_showConflictAircraftAlways) {
        m_showConflictAircraftAlways = false;
        ShowConflictAircraftAlwaysCheckBox->Checked = false;
    }
    
    // 화면 새로고침
    ObjectDisplay->Repaint();
}

//---------------------------------------------------------------------------
void TForm1::UpdateConflictFilterLabels()
{
    // TCPA 라벨 - 분:초 형식으로 표시
    AnsiString minStr, maxStr;
    int minTCPA = (int)m_tcpaMinThreshold;
    int maxTCPA = (int)m_tcpaMaxThreshold;
    
    if (minTCPA < 60) {
        minStr = IntToStr(minTCPA) + "s";
    } else {
        int minutes = minTCPA / 60;
        int seconds = minTCPA % 60;
        minStr.sprintf("%d:%02d", minutes, seconds);
    }
    
    if (maxTCPA < 60) {
        maxStr = IntToStr(maxTCPA) + "s";
    } else {
        int minutes = maxTCPA / 60;
        int seconds = maxTCPA % 60;
        maxStr.sprintf("%d:%02d", minutes, seconds);
    }
    
    TCPAThresholdLabel->Caption = "TCPA: " + minStr + " ~ " + maxStr;
        
    HorizontalDistanceLabel->Caption = "H Dist: 0 ~ " + 
        FormatFloat("0.0", m_horizontalMaxDistance) + " NM";
        
    VerticalDistanceLabel->Caption = "V Dist: 0 ~ " + 
        IntToStr((int)m_verticalMaxDistance) + " ft";
}

//---------------------------------------------------------------------------
// 더 이상 사용하지 않음 - ProximityAssessor에서 직접 임계값 적용
/*
bool TForm1::IsConflictFiltered(double tcpa, double horizontalDist, double verticalDist)
{
    if (!m_conflictFilterEnabled) {
        return false;  // 필터가 비활성화되면 모든 충돌 허용
    }
    
    // TCPA 필터 체크
    if (tcpa < m_tcpaMinThreshold || tcpa > m_tcpaMaxThreshold) {
        return true;  // 필터링됨
    }
    
    // 수평거리 필터 체크 (Min은 항상 0)
    if (horizontalDist > m_horizontalMaxDistance) {
        return true;  // 필터링됨
    }
    
    // 수직거리 필터 체크 (Min은 항상 0)
    if (verticalDist > m_verticalMaxDistance) {
        return true;  // 필터링됨
    }
    
    return false;  // 필터를 통과함
}
*/

//---------------------------------------------------------------------------
// Critical 상태 깜빡임 타이머 이벤트
void __fastcall TForm1::CriticalBlinkTimerTimer(TObject *Sender)
{
    m_criticalBlinkState = !m_criticalBlinkState;
    
    // ListView를 다시 그리도록 강제 업데이트
    ConflictListView->Invalidate();
}

//---------------------------------------------------------------------------
// ListView 서브 항목의 커스텀 색상 그리기
void __fastcall TForm1::ConflictListViewCustomDrawSubItem(TCustomListView *Sender, TListItem *Item,
                                                          int SubItem, TCustomDrawState State, bool &DefaultDraw)
{
    if (!Item) {
        DefaultDraw = true;
        return;
    }
    
    // 선택된 항목인지 확인 (포커스 여부와 관계없이)
    bool isItemSelected = false;
    
    // 더 확실한 선택 상태 확인 방법
    if (ConflictListView->Selected && ConflictListView->Selected == Item) {
        isItemSelected = true;
    }
    
    // 선택된 항목의 모든 서브 아이템은 청록색으로 설정
    if (isItemSelected) {
        Sender->Canvas->Brush->Color = TColor(RGB(0, 255, 255)); // 청록색 (Cyan)
        Sender->Canvas->Font->Color = clBlack; // 청록 배경에는 검은색 텍스트
        DefaultDraw = true;
        return; // 선택된 항목은 다른 색상 로직을 건너뛰기
    }
    
    // 선택되지 않은 항목만 위험도에 따른 색상 적용
    if (Item->SubItems->Count > 4) {
        AnsiString levelStr = Item->SubItems->Strings[4]; // 위험도 레벨
        
        if (levelStr == "Critical") {
            if (m_criticalBlinkTimer->Enabled && m_criticalBlinkState) {
                Sender->Canvas->Brush->Color = clRed;
                Sender->Canvas->Font->Color = clWhite; // 빨간 배경에는 흰색 텍스트
            } else {
                Sender->Canvas->Brush->Color = TColor(RGB(255, 220, 220)); // 연한 빨간색
                Sender->Canvas->Font->Color = clBlack;
            }
        }
        else if (levelStr == "High") {
            Sender->Canvas->Brush->Color = TColor(RGB(255, 165, 0)); // 주황색
            Sender->Canvas->Font->Color = clBlack;
        }
        else if (levelStr == "Medium") {
            Sender->Canvas->Brush->Color = TColor(RGB(255, 255, 0)); // 노란색
            Sender->Canvas->Font->Color = clBlack;
        }
        else if (levelStr == "Low") {
            Sender->Canvas->Brush->Color = TColor(RGB(144, 238, 144)); // 연한 초록색
            Sender->Canvas->Font->Color = clBlack;
        }
        else {
            Sender->Canvas->Brush->Color = clWindow;
            Sender->Canvas->Font->Color = clWindowText;
        }
    } else {
        Sender->Canvas->Brush->Color = clWindow;
        Sender->Canvas->Font->Color = clWindowText;
    }
    
    DefaultDraw = true; // 기본 그리기 허용
}

//---------------------------------------------------------------------------
// ListView 항목의 커스텀 색상 그리기
void __fastcall TForm1::ConflictListViewCustomDrawItem(TCustomListView *Sender, TListItem *Item, 
                                                       TCustomDrawState State, bool &DefaultDraw)
{
    if (!Item || Item->SubItems->Count < 5) {
        DefaultDraw = true;
        return;
    }
    
    // 위험도 레벨 가져오기 (마지막 컬럼)
    AnsiString levelStr = Item->SubItems->Strings[4];
    TColor backgroundColor = clWindow; // 기본 색상
    TColor textColor = clWindowText; // 기본 텍스트 색상
    
    // 위험도에 따른 색상 설정
    if (levelStr == "Critical") {
        // Critical인 경우 항상 깜빡임 적용
        if (m_criticalBlinkTimer->Enabled && m_criticalBlinkState) {
            backgroundColor = clRed;
            textColor = clWhite; // 빨간 배경에는 흰색 텍스트
        } else {
            backgroundColor = TColor(RGB(255, 220, 220)); // 연한 빨간색
            textColor = clBlack;
        }
    }
    else if (levelStr == "High") {
        // High인 경우 주황색
        backgroundColor = TColor(RGB(255, 165, 0)); // 주황색
        textColor = clBlack; // 주황 배경에는 검은색 텍스트
    }
    else if (levelStr == "Medium") {
        // Medium인 경우 노란색
        backgroundColor = TColor(RGB(255, 255, 0)); // 노란색
        textColor = clBlack; // 노란 배경에는 검은색 텍스트
    }
    else if (levelStr == "Low") {
        // Low인 경우 연한 초록색
        backgroundColor = TColor(RGB(144, 238, 144)); // 연한 초록색 (Light Green)
        textColor = clBlack; // 초록 배경에는 검은색 텍스트
    }
    else {
        // 기타의 경우 기본 색상
        backgroundColor = clWindow;
        textColor = clWindowText;
    }
    
    // 선택된 항목인지 확인 (포커스 여부와 관계없이)
    bool isItemSelected = false;
    
    // 더 확실한 선택 상태 확인 방법
    if (ConflictListView->Selected && ConflictListView->Selected == Item) {
        isItemSelected = true;
    }
    
    // 선택된 항목인 경우 청록색으로 덮어쓰기 (선택된 비행기 색과 동일)
    if (isItemSelected) {
        backgroundColor = TColor(RGB(0, 255, 255)); // 청록색 (Cyan)
        textColor = clBlack; // 청록 배경에는 검은색 텍스트
    }
    
    // 배경색 설정
    Sender->Canvas->Brush->Color = backgroundColor;
    Sender->Canvas->Font->Color = textColor;
    
    DefaultDraw = true; // 기본 그리기 허용
}
//---------------------------------------------------------------------------
// 충돌 상태에 따른 색상 및 깜빡임 업데이트
void TForm1::UpdateConflictStatusColors()
{
    // Critical 충돌이 있으면 깜빡임 타이머 활성화
    if (m_hasCriticalConflicts) {
        if (!m_criticalBlinkTimer->Enabled) {
            m_criticalBlinkTimer->Enabled = true;
        }
    } else {
        // Critical 충돌이 없으면 깜빡임 타이머 비활성화
        if (m_criticalBlinkTimer->Enabled) {
            m_criticalBlinkTimer->Enabled = false;
            m_criticalBlinkState = false;
        }
    }
    
    // ListView를 다시 그리도록 강제 업데이트
    ConflictListView->Invalidate();
}

//---------------------------------------------------------------------------
// 이탈감지 항공기 목록 업데이트
void __fastcall TForm1::UpdateDeviationList()
{
    DeviationListView->Items->Clear();
    
    for (const AnsiString& deviationInfo : FDeviationAircraftList) {
        // CSV 형태의 데이터를 파싱 - result.csv 형식: Row, CENTROID_ID, Altitude, Latitude, Longitude, HexIdent, timestamp_utc, distance_from_centroid
        TStringList* parts = new TStringList();
        try {
            parts->CommaText = deviationInfo;
            
            printf("CSV parts count: %d\n", parts->Count);
            
            if (parts->Count >= 7) {
                // HexIdent, Altitude, Latitude, Longitude 정보 표시
                printf("Deviation Aircraft - ICAO: %s, Alt: %s, Lat: %s, Lon: %s\n",
                       AnsiString(parts->Strings[4]).c_str(), AnsiString(parts->Strings[1]).c_str(),
                       AnsiString(parts->Strings[2]).c_str(), AnsiString(parts->Strings[3]).c_str());
                TListItem* item = DeviationListView->Items->Add();
                item->Caption = parts->Strings[4]; // HexIdent (인덱스 5)
                item->SubItems->Add(parts->Strings[1]); // Altitude (인덱스 2)
                item->SubItems->Add(parts->Strings[2]); // Latitude (인덱스 3)
                item->SubItems->Add(parts->Strings[3]); // Longitude (인덱스 4)
                
                // 이탈감지된 항목은 빨간색으로 표시
                item->ImageIndex = -1;
            } else if (parts->Count >= 1 && !deviationInfo.Trim().IsEmpty()) {
                // 단순 텍스트 형태의 경우 (fallback)
                TListItem* item = DeviationListView->Items->Add();
                item->Caption = "N/A";
                item->SubItems->Add("N/A");
                item->SubItems->Add("N/A");
                item->SubItems->Add(deviationInfo);
            }
        } __finally {
            delete parts;
        }
    }
    
    // 이탈감지 항목이 있으면 레이블 색상 변경
    if (FDeviationAircraftList.size() > 0) {
        DeviationLabel->Font->Color = clRed;
        AnsiString countStr = AnsiString(static_cast<int>(FDeviationAircraftList.size()));
        DeviationLabel->Caption = "Route Deviation Alerts (" + countStr + ")";
    } else {
        DeviationLabel->Font->Color = clGreen;
        DeviationLabel->Caption = "Route Deviation Alerts (0)";
    }
}

// 이탈감지 항공기 선택 이벤트 핸들러
void __fastcall TForm1::DeviationListViewSelectItem(TObject *Sender, TListItem *Item, bool Selected)
{
    if (Selected && Item) {
        AnsiString icaoStr = Item->Caption;
        if (icaoStr != "N/A") {
            try {
                // ICAO를 16진수로 변환
                unsigned int icao = 0;
                if (icaoStr.Length() == 6) {
                    icao = StrToInt("0x" + icaoStr);
                } else {
                    icao = StrToInt(icaoStr);
                }
                
                // 해당 항공기로 포커스 이동
                OnAircraftSelected(icao);
                
                // 해당 항공기 정보를 UI에 표시
                TADS_B_Aircraft* ac = FAircraftModel->FindAircraftByICAO(icao);
                if (ac) {
                    // 해당 항공기 중심으로 맵 이동
                    double aircraftLat = ac->Latitude;
                    double aircraftLon = ac->Longitude;
                    
                    MapCenterLat = aircraftLat;
                    MapCenterLon = aircraftLon;
                    SetMapCenter(g_EarthView->m_Eye.x, g_EarthView->m_Eye.y);
                    
                    // 항공기 정보 패널 업데이트
                    UpdateCloseControlPanel(ac, nullptr);
                }
            } catch (...) {
                // ICAO 변환 실패 시 무시
            }
        }
    }
}
//---------------------------------------------------------------------------

