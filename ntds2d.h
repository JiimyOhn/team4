//---------------------------------------------------------------------------

#ifndef NTDS2DH
#define NTDS2DH
#include <vector>
#include <windows.h>
#include "Aircraft.h"
#include "AircraftInfo.h"
int MakeAirplaneImages(void);
int MakeAirportImages(void);
void MakeAirTrackFriend(void);
void MakeAirTrackHostile(void);
void MakeAirTrackUnknown(void);
void MakePoint(void);
void MakeTrackHook(void);
void DrawAirTrackFriend(float x,float y);
void DrawAirTrackHostile(float x,float y);
void DrawAirTrackUnknown(float x,float y);
void DrawPoint(float x,float y);
void DrawAirplaneImage(float x, float y,float scale,float heading,int imageNum);
struct AirplaneInstance {
    float x;
    float y;
    float scale;
    float heading;
    int   imageNum;
    float color[4];
};

struct AirplaneLineInstance {
    float x1;
    float y1;
    float x2;
    float y2;
};
void InitAirplaneInstancing();
void DrawAirplaneImagesInstanced(const std::vector<AirplaneInstance>& instances);
void InitAirplaneLinesInstancing();
void DrawAirplaneLinesInstanced(const std::vector<AirplaneLineInstance>& instances);
void DrawTrackHook(float x, float y);
void DrawRadarCoverage(float xc, float yc, float major, float minor);
void DrawLeader(float x1, float y1, float x2, float y2);

// 다양한 리더 스타일 함수들
void DrawLeaderArrow(float x1, float y1, float x2, float y2, float arrowSize = 8.0f);
void DrawLeaderDashed(float x1, float y1, float x2, float y2);
void DrawLeaderThick(float x1, float y1, float x2, float y2, float thickness = 3.0f);

void ComputeTimeToGoPosition(float  TimeToGo,
                                                         float  xs, float  ys,
                                                         float  xv, float  yv,
                                                         float &xe, float &ye);
void DrawLines(DWORD resolution, double xpts[],double ypts[]);

struct HexCharInstance {
    float x;
    float y;
    int   glyph;
    float color[4];
};

void InitHexTextInstancing();
void DrawHexTextInstanced(const std::vector<HexCharInstance>& instances);
void SetHexTextScale(float scale);
void SetHexTextBold(bool bold);
float GetHexTextScale();

// 공항 마커 인스턴싱용 구조체
struct AirportInstance {
    float x;
    float y;
    int   type;
};

void InitAirportInstancing();

// 항공기 타입별 아이콘 선택 함수
int SelectAircraftIcon(const TADS_B_Aircraft* aircraft);

// --- Airport marker atlas ---
enum AirportType { Civil = 0, Military = 1, Helipad = 2 };

// 공항 타입 판별
AirportType GetAirportType(const AirportInfo& airport);

struct AtlasRect { int x, y, w, h; };

extern const int iconW, iconH, atlasW, atlasH;
extern AtlasRect airportAtlasRects[3];
extern GLuint atlasTexId;

void DrawAtlasIcon(double x, double y, const AtlasRect& rect, GLuint texId,
                   int iconDrawSize = 32);
void DrawAirportMarkers();

//---------------------------------------------------------------------------
#endif


