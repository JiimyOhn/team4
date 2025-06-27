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
#include <Vcl.ComCtrls.hpp> // For TTrackBar
#include <unordered_map>

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
 m_planeBatch.reserve(5000);
  m_lineBatch.reserve(5000);
  m_textBatch.reserve(5000 * 6);
  SetHexTextScale(1.0f);
  SetHexTextBold(true);
  printf("init complete\n");

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
}

void __fastcall TForm1::ApiCallTimerTimer(TObject *Sender)
{
    try {
        new TLoadApiDataThread();
        printf("ApiCallTimer: LoadAllData() OK\n");
    } catch (const std::exception& e) {
        printf("ApiCallTimer EXCEPTION: %s\n", e.what());
    } catch (...) {
        printf("ApiCallTimer UNKNOWN EXCEPTION\n");
    }
}

//---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
{
 Timer1->Enabled=false;
 Timer2->Enabled=false;
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
    printf("OpenGL Version %s\n",glGetString(GL_VERSION));
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
 DrawObjects();
 DrawAirportMarkers();      // 공항 위치 점 그리기
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
void __fastcall TForm1::DrawObjects(void)
{
  double ScrX, ScrY, ScrX2, ScrY2;
  int    ViewableAircraft=0;

  glEnable( GL_LINE_SMOOTH );
  glEnable( GL_POINT_SMOOTH );
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
  glLineWidth(3.0);
  glPointSize(4.0);
  glColor4f(1.0, 1.0, 1.0, 1.0);

  LatLon2XY(MapCenterLat,MapCenterLon, ScrX, ScrY);

  glBegin(GL_LINE_STRIP);
  glVertex2f(ScrX-20.0,ScrY);
  glVertex2f(ScrX+20.0,ScrY);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex2f(ScrX,ScrY-20.0);
  glVertex2f(ScrX,ScrY+20.0);
  glEnd();


  const void *Key;
  ght_iterator_t iterator;
  TADS_B_Aircraft* Data,*DataCPA;

  DWORD i,j,Count;

  if (AreaTemp)
  {
   glPointSize(3.0);
	for (DWORD i = 0; i <AreaTemp->NumPoints ; i++)
		LatLon2XY(AreaTemp->Points[i][1],AreaTemp->Points[i][0],
				  AreaTemp->PointsAdj[i][0],AreaTemp->PointsAdj[i][1]);

   glBegin(GL_POINTS);
   for (DWORD i = 0; i <AreaTemp->NumPoints ; i++)
	{
	glVertex2f(AreaTemp->PointsAdj[i][0],
			   AreaTemp->PointsAdj[i][1]);
	}
	glEnd();
   glBegin(GL_LINE_STRIP);
   for (DWORD i = 0; i <AreaTemp->NumPoints ; i++)
	{
	glVertex2f(AreaTemp->PointsAdj[i][0],
			   AreaTemp->PointsAdj[i][1]);
	}
	glEnd();
  }
	Count=Areas->Count;
	for (i = 0; i < Count; i++)
	 {
	   TArea *Area = (TArea *)Areas->Items[i];
	   TMultiColor MC;

	   MC.Rgb=ColorToRGB(Area->Color);
	   if (Area->Selected)
	   {
		  glLineWidth(4.0);
		  glPushAttrib (GL_LINE_BIT);
		  glLineStipple (3, 0xAAAA);
	   }


	   glColor4f(MC.Red/255.0, MC.Green/255.0, MC.Blue/255.0, 1.0);
	   glBegin(GL_LINE_LOOP);
	   for (j = 0; j <Area->NumPoints; j++)
	   {
		LatLon2XY(Area->Points[j][1],Area->Points[j][0], ScrX, ScrY);
		glVertex2f(ScrX,ScrY);
	   }
	  glEnd();
	   if (Area->Selected)
	   {
		glPopAttrib ();
		glLineWidth(2.0);
	   }

	   glColor4f(MC.Red/255.0, MC.Green/255.0, MC.Blue/255.0, 0.4);

	   for (j = 0; j <Area->NumPoints; j++)
	   {
		LatLon2XY(Area->Points[j][1],Area->Points[j][0],
				  Area->PointsAdj[j][0],Area->PointsAdj[j][1]);
	   }
	  TTriangles *Tri=Area->Triangles;

	  while(Tri)
	  {
		glBegin(GL_TRIANGLES);
		glVertex2f(Area->PointsAdj[Tri->indexList[0]][0],
				   Area->PointsAdj[Tri->indexList[0]][1]);
		glVertex2f(Area->PointsAdj[Tri->indexList[1]][0],
				   Area->PointsAdj[Tri->indexList[1]][1]);
		glVertex2f(Area->PointsAdj[Tri->indexList[2]][0],
				   Area->PointsAdj[Tri->indexList[2]][1]);
		glEnd();
		Tri=Tri->next;
	  }
	 }

	AircraftCountLabel->Caption= IntToStr(FAircraftModel->GetAircraftCount());
        m_planeBatch.clear();
        m_lineBatch.clear();
        m_textBatch.clear();
        for(Data = FAircraftModel->GetFirstAircraft(&iterator, &Key);
                          Data; Data = FAircraftModel->GetNextAircraft(&iterator, &Key))
        {
          if (Data->HaveLatLon)
          {
            // [1] RouteInfo 얻기
            //const RouteInfo* route = FindRouteByCallsign(AnsiString(Data->FlightNum).c_str());

            const RouteInfo* route = nullptr;
            if (!filterAirline.IsEmpty() || !filterOrigin.IsEmpty() || !filterDestination.IsEmpty()) {
                auto it = callSignToRoute.find(AnsiString(Data->FlightNum).c_str());
                route = (it != callSignToRoute.end()) ? it->second : nullptr;
                // (필터가 한 개라도 있을 때만 route lookup)
                if (!IsRouteMatched(route)) continue;
            }

            // ① [추가] 필터가 지정된 경우에만 route==nullptr 항공기를 스킵
            if ((!filterAirline.IsEmpty() || !filterOrigin.IsEmpty() || !filterDestination.IsEmpty()) && route == nullptr)
                continue;

            // [2] 항공사 필터
            if (!filterAirline.IsEmpty() && route) {
                AnsiString code = AnsiString(route->airlineCode.c_str()).UpperCase();
				AnsiString filter = filterAirline.UpperCase();
				if (code.SubString(1, filter.Length()) != filter)
					continue;
            }

            // [3] 출발지 필터 (route->airportCodes[0]이 출발지)
            if (!filterOrigin.IsEmpty() && route && !route->airportCodes.empty()) {
                AnsiString origin = AnsiString(route->airportCodes.front().c_str()).UpperCase();
                if (AnsiString(route->airportCodes.front().c_str()).UpperCase() != filterOrigin.UpperCase())
                    continue;
            }

            // [4] 도착지 필터 (route->airportCodes.back()이 도착지)
            if (!filterDestination.IsEmpty() && route && !route->airportCodes.empty()) {
                if (AnsiString(route->airportCodes.back().c_str()).UpperCase() != filterDestination.UpperCase())
                    continue;
            }
            ViewableAircraft++;

           LatLon2XY(Data->Latitude,Data->Longitude, ScrX, ScrY);
           ScrX2 = ScrX;
           ScrY2 = ScrY;
           //DrawPoint(ScrX,ScrY);
           float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 기본 흰색
           
           // 항공기 타입에 따른 색상 설정
           if (aircraft_is_helicopter(Data->ICAO, NULL)) {
               color[0] = 0.0f; color[1] = 1.0f; color[2] = 0.0f; // 헬리콥터: 녹색
           } else if (aircraft_is_military(Data->ICAO, NULL)) {
               color[0] = 1.0f; color[1] = 0.0f; color[2] = 0.0f; // 군용기: 빨간색
           } else {
               color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; // 민간기: 흰색
           }
           
           if (Data->HaveSpeedAndHeading)
           {
                 // 속도와 방향이 있는 경우 보라색으로 강조
                 color[0]=1.0f; color[1]=0.0f; color[2]=1.0f; color[3]=1.0f;
				 if (TimeToGoCheckBox->State==cbChecked)
				 {
					double lat2,lon2,az2;
					VDirect(Data->Latitude,Data->Longitude,
							Data->Heading,Data->Speed*(double)TimeToGoTrackBar->Position/3600.0,&lat2,&lon2,&az2);
					LatLon2XY(lat2,lon2, ScrX2, ScrY2);
					}
			}
			else
				{
					Data->Heading=0.0;
					// 속도와 방향이 없는 경우 빨간색으로 표시
					color[0]=1.0f; color[1]=0.0f; color[2]=0.0f; color[3]=1.0f;
				}


				AirplaneInstance inst;
				inst.x = ScrX;
				inst.y = ScrY;
				inst.scale = 1.5f; // 기본 크기
				
				// 고도에 따른 크기 조정
				if (Data->HaveAltitude) {
					if (Data->Altitude > 30000) {
						inst.scale = 1.0f; // 고고도는 작게 (멀리 있어서)
					} else if (Data->Altitude < 10000) {
						inst.scale = 2.0f; // 저고도는 크게 (가까이 있어서)
					} else {
						inst.scale = 1.5f; // 중간 고도는 기본 크기
					}
				}
				
				inst.heading = Data->Heading;
				inst.imageNum = SelectAircraftIcon(Data);
				inst.color[0] = color[0];
				inst.color[1] = color[1];
				inst.color[2] = color[2];
				inst.color[3] = color[3];
				m_planeBatch.push_back(inst);

				AirplaneLineInstance line;
				line.x1 = ScrX;
				line.y1 = ScrY;
				line.x2 = ScrX2;
				line.y2 = ScrY2;
				m_lineBatch.push_back(line);

				for(int i=0; i<6 && Data->HexAddr[i]; ++i){
					HexCharInstance tc;
					tc.x = ScrX + 40 + i * (HEX_FONT_WIDTH - 15) * GetHexTextScale();
					tc.y = ScrY - 10;
					char c = Data->HexAddr[i];
					if(c >= '0' && c <= '9') tc.glyph = c - '0';
					else if(c >= 'A' && c <= 'F') tc.glyph = 10 + (c - 'A');
					else tc.glyph = 0;
					tc.color[0] = color[0];
					tc.color[1] = color[1];
					tc.color[2] = color[2];
					tc.color[3] = color[3];
					m_textBatch.push_back(tc);
				}
        }
	   }
               // 기존 배치 렌더링 대신 개별 스타일 적용
               for (const auto& line : m_lineBatch) {
                   // 항공기 타입에 따른 다른 리더 스타일 적용
                   TADS_B_Aircraft* aircraft = FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CC);
                   if (aircraft && aircraft_is_military(aircraft->ICAO, NULL)) {
                       // 군용기는 두꺼운 선
                       DrawLeaderThick(line.x1, line.y1, line.x2, line.y2, 4.0f);
                   } else if (aircraft && aircraft_is_helicopter(aircraft->ICAO, NULL)) {
                       // 헬리콥터는 점선
                       DrawLeaderDashed(line.x1, line.y1, line.x2, line.y2);
                   } else {
                       // 일반 항공기는 화살표
                       DrawLeaderArrow(line.x1, line.y1, line.x2, line.y2, 8.0f);
                   }
               }
               
               // 기존 배치 렌더링 (백업용)
               // DrawAirplaneLinesInstanced(m_lineBatch);
               DrawAirplaneImagesInstanced(m_planeBatch);
               DrawHexTextInstanced(m_textBatch);

 ViewableAircraftCountLabel->Caption=ViewableAircraft;
 if (TrackHook.Valid_CC)
 {
		Data= FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CC);
		if (Data)
		{
		ICAOLabel->Caption=Data->HexAddr;
        if (Data->HaveFlightNum)
          FlightNumLabel->Caption=Data->FlightNum;
        else FlightNumLabel->Caption="N/A";
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
		 SpdLabel->Caption=FloatToStrF(Data->Speed, ffFixed,12,2)+" KTS  VRATE:"+FloatToStrF(Data->VerticalRate,ffFixed,12,2);
		 HdgLabel->Caption=FloatToStrF(Data->Heading, ffFixed,12,2)+" DEG";
        }
        else
        {
 		 SpdLabel->Caption="N/A";
		 HdgLabel->Caption="N/A";
        }
        if (Data->Altitude)
		 AltLabel->Caption= FloatToStrF(Data->Altitude, ffFixed,12,2)+" FT";
		else AltLabel->Caption="N/A";

		MsgCntLabel->Caption="Raw: "+IntToStr((int)Data->NumMessagesRaw)+" SBS: "+IntToStr((int)Data->NumMessagesSBS);
		TrkLastUpdateTimeLabel->Caption=TimeToChar(Data->LastSeen);

        glColor4f(1.0, 0.0, 0.0, 1.0);
        LatLon2XY(Data->Latitude,Data->Longitude, ScrX, ScrY);
        DrawTrackHook(ScrX, ScrY);
        }

		else
        {
		 TrackHook.Valid_CC=false;
		 ICAOLabel->Caption="N/A";
		 FlightNumLabel->Caption="N/A";
         CLatLabel->Caption="N/A";
		 CLonLabel->Caption="N/A";
         SpdLabel->Caption="N/A";
		 HdgLabel->Caption="N/A";
		 AltLabel->Caption="N/A";
		 MsgCntLabel->Caption="N/A";
         TrkLastUpdateTimeLabel->Caption="N/A";
        }
 }
 if (TrackHook.Valid_CPA)
 {
  bool CpaDataIsValid=false;
  DataCPA= FAircraftModel->FindAircraftByICAO(TrackHook.ICAO_CPA);
  if ((DataCPA) && (TrackHook.Valid_CC))
	{

	  double tcpa,cpa_distance_nm, vertical_cpa;
	  double lat1, lon1,lat2, lon2, junk;
	  if (computeCPA(Data->Latitude, Data->Longitude, Data->Altitude,
					 Data->Speed,Data->Heading,
					 DataCPA->Latitude, DataCPA->Longitude, DataCPA->Altitude,
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
}

bool TForm1::IsRouteMatched(const RouteInfo* route) const {
    if (!route) return false;
    if (!filterAirline.IsEmpty() && AnsiString(route->airlineCode.c_str()).UpperCase() != filterAirline.UpperCase())
        return false;
    if (!filterOrigin.IsEmpty() && route->airportCodes.size() &&
        AnsiString(route->airportCodes.front().c_str()).UpperCase() != filterOrigin.UpperCase())
        return false;
    if (!filterDestination.IsEmpty() && route->airportCodes.size() &&
        AnsiString(route->airportCodes.back().c_str()).UpperCase() != filterDestination.UpperCase())
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
		 printf("%s\n\n",GetAircraftDBInfo(ADS_B_Aircraft->ICAO));
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

    // Model에게 "오래된 항공기 삭제" 작업을 위임
    FAircraftModel->PurgeStaleAircraft(CSpinStaleTime->Value);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PurgeButtonClick(TObject *Sender)
{
    // 1. Model에게 "모든 항공기 삭제" 작업을 위임
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
        FRawDataHandler->Connect(RawIpAddress->Text, 30002);
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
        FSBSDataHandler->Connect(SBSIpAddress->Text, 5002);
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

	if (!provider) throw Sysutils::Exception("Unknown map type");

    // 2. 캐시 디렉터리는  생성
    std::string cachedir = provider->GetCacheDir();
     if (mkdir(cachedir.c_str()) != 0 && errno != EEXIST)
	    throw Sysutils::Exception("Can not create cache directory");

    g_Storage = new FilesystemStorage(cachedir, true);
	g_Keyhole = new KeyholeConnection(provider->GetURI());
	g_Keyhole->SetFetchTileCallback([p = provider.get()](TilePtr tile, KeyholeConnection* conn) {
		if (!p) {
			printf("[Callback] provider is nullptr!\n");
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
			printf("Load ERROR ID %s\n",LastArea);
		   }
		 else printf("Loaded ID %s\n",LastArea);
		 strcpy(LastArea,Area);
		 }
	   if (Form1->AreaTemp==NULL)
		   {
			Form1->AreaTemp= new TArea;
			Form1->AreaTemp->NumPoints=0;
			Form1->AreaTemp->Name=Area;
			Form1->AreaTemp->Selected=false;
			Form1->AreaTemp->Triangles=NULL;
			 printf("Loading ID %s\n",Area);
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
	  printf("Parsing of \"%s\" failed: %s\n", FileName.c_str(), strerror(errno));
      return (false);
	}
   if ((Form1->AreaTemp!=NULL) && (Form1->AreaTemp->NumPoints>0))
   {
     char Area[512];
     strcpy(Area,Form1->AreaTemp->Name.c_str());
     if (FinshARTCCBoundary())
	    {
        printf("Loaded ERROR ID %s\n",Area);
	    }
        else printf("Loaded ID %s\n",Area);
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
   printf("Duplicate Area Name %s\n",Form1->AreaTemp->Name.c_str());;
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

// ICAO 코드 uint32 → 6자리 대문자 16진 문자열로 변환
std::string ICAO_to_string(uint32_t icao) {
    char buf[7] = {0};
    snprintf(buf, sizeof(buf), "%06X", icao);
    return std::string(buf);
}

// ========================
// 2. 메인 패널 갱신 함수
// ========================

void __fastcall TForm1::UpdateCloseControlPanel(TADS_B_Aircraft* ac, const RouteInfo* route)
{
    // -------- 기존 항공기 정보 갱신 --------
    if (!ac) {
        // 패널 초기화 (None 처리)
        ICAOLabel->Caption      = "N/A";
        FlightNumLabel->Caption = "N/A";
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
    if (!ac) {
        UpdateCloseControlPanel(nullptr, nullptr);
        return;
    }

    // **배열을 AnsiString으로 변환한 뒤 처리!**
    AnsiString flightNum = AnsiString(ac->FlightNum).Trim().UpperCase();
    auto it = callSignToRoute.find(flightNum.c_str());
    const RouteInfo* route = (it != callSignToRoute.end()) ? it->second : nullptr;

    // 패널 전체 갱신 (기존 Close 라벨 + 경로)
    UpdateCloseControlPanel(ac, route);
}

void __fastcall TForm1::FilterAirlineEditChange(TObject *Sender)
{
    filterAirline = FilterAirlineEdit->Text.Trim();
    ObjectDisplay->Repaint();
}
void __fastcall TForm1::FilterOriginEditChange(TObject *Sender)
{
    filterOrigin = FilterOriginEdit->Text.Trim();
    printf("[EditChange] filterOrigin: %s\n", filterOrigin.c_str());
    ObjectDisplay->Repaint();
}
void __fastcall TForm1::FilterDestinationEditChange(TObject *Sender)
{
    filterDestination = FilterDestinationEdit->Text.Trim();
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
    // --- 기존 트랙바 삭제 및 드롭다운(ComboBox)으로 대체 ---
    if (PlaybackSpeedTrackBar) {
        delete PlaybackSpeedTrackBar;
        PlaybackSpeedTrackBar = nullptr;
    }
    if (PlaybackSpeedLabel) {
        delete PlaybackSpeedLabel;
        PlaybackSpeedLabel = nullptr;
    }
    if (PlaybackSpeedComboBox) {
        delete PlaybackSpeedComboBox;
        PlaybackSpeedComboBox = nullptr;
    }
    
    // ComboBox 생성
    PlaybackSpeedComboBox = new TComboBox(this);
    PlaybackSpeedComboBox->Parent = this;
    PlaybackSpeedComboBox->Left = 20;
    PlaybackSpeedComboBox->Top = 80;
    PlaybackSpeedComboBox->Width = 80;
    PlaybackSpeedComboBox->Style = csDropDownList;
    PlaybackSpeedComboBox->Items->Add("1x");
    PlaybackSpeedComboBox->Items->Add("2x");
    PlaybackSpeedComboBox->Items->Add("3x");
    PlaybackSpeedComboBox->ItemIndex = 0;
    
    // 라벨
    PlaybackSpeedLabel = new TLabel(this);
    PlaybackSpeedLabel->Parent = this;
    PlaybackSpeedLabel->Left = PlaybackSpeedComboBox->Left + PlaybackSpeedComboBox->Width + 10;
    PlaybackSpeedLabel->Top = PlaybackSpeedComboBox->Top + 2;
    PlaybackSpeedLabel->Caption = "재생 속도";
    PlaybackSpeedLabel->Font->Size = 10;
    PlaybackSpeedLabel->Font->Color = clBlack;
    PlaybackSpeedLabel->Show();
    
    // 이벤트 핸들러 할당
    PlaybackSpeedComboBox->OnChange = PlaybackSpeedComboBoxChange;
    
    // 최초 1회 적용
    if (FRawDataHandler) FRawDataHandler->SetPlaybackSpeed(1.0);
    if (FSBSDataHandler) FSBSDataHandler->SetPlaybackSpeed(1.0);
    PlaybackSpeedLabel->Caption = "재생 속도: 1x";
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
        PlaybackSpeedLabel->Caption = "재생 속도: " + FloatToStrF(speed, ffGeneral, 3, 2) + "x";
    }
}
//---------------------------------------------------------------------------

