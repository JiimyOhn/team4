//---------------------------------------------------------------------------

#ifndef AircraftDBH
#define AircraftDBH
#include "ght_hash_table.h"

#define  AC_DB_NUM_FIELDS          27
#define  AC_DB_ICAO                 0
#define  AC_DB_Registration         1
#define  AC_DB_ManufacturerICAO     2
#define  AC_DB_ManufacturerName     3
#define  AC_DB_Model                4
#define  AC_DB_TypeCode             5
#define  AC_DB_SerialNumber         6
#define  AC_DB_LineNumber           7
#define  AC_DB_ICAOAircraftType     8
#define  AC_DB_Operator             9
#define  AC_DB_OperatorCallSign    10
#define  AC_DB_OperatorICAO        11
#define  AC_DB_OperatorIATA        12
#define  AC_DB_Owner               13
#define  AC_DB_TestReg             14
#define  AC_DB_Registered          15
#define  AC_DB_RegUntil            16
#define  AC_DB_Status              17
#define  AC_DB_Built               18
#define  AC_DB_FirstFlightDate     19
#define  AC_DB_Seatconfiguration   20
#define  AC_DB_Engines             21
#define  AC_DB_Modes               22
#define  AC_DB_ADSB                23
#define  AC_DB_ACARS               24
#define  AC_DB_Notes               25
#define  AC_DB_CategoryDescription 26

typedef struct
{
  uint32_t    ICAO24;
  AnsiString  Fields[AC_DB_NUM_FIELDS];
} TAircraftData;

bool InitAircraftDB(AnsiString FileName);
const char * GetAircraftDBInfo(uint32_t addr);

// 추가된 함수 선언들
const char *aircraft_get_country(uint32_t addr, bool get_short);
bool aircraft_is_helicopter(uint32_t addr, const char **type_ptr);
const char *aircraft_get_military(uint32_t addr);
bool aircraft_is_military(uint32_t addr, const char **country);

// 항공기 카테고리 분류 함수들
enum AircraftCategory {
    CATEGORY_COMMERCIAL = 0,      // 대형 상업용 항공기 (B737, A320 등)
    CATEGORY_CARGO = 16,           // 화물기
    CATEGORY_HELICOPTER = 72,      // 헬리콥터
    CATEGORY_MILITARY = 29,        // 군용기
    CATEGORY_BUSINESS_JET = 7,    // 비즈니스 제트
    CATEGORY_GENERAL_AVIATION = 3, // 일반 항공기 (소형 개인용)
    CATEGORY_GLIDER = 39,          // 글라이더
    CATEGORY_ULTRALIGHT = 45,      // 초경량 항공기
    CATEGORY_UNKNOWN = 68
};

AircraftCategory aircraft_get_category(uint32_t addr);
bool aircraft_is_commercial(uint32_t addr);
bool aircraft_is_cargo(uint32_t addr);
bool aircraft_is_business_jet(uint32_t addr);
bool aircraft_is_glider(uint32_t addr);
bool aircraft_is_ultralight(uint32_t addr);

//---------------------------------------------------------------------------
#endif
