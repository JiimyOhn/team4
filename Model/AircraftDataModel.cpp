// ==== 파일: AircraftDataModel.cpp (최종 완전판) ====

#pragma hdrstop
#include "AircraftDataModel.h"
#include "TimeFunctions.h"
#include "RawDecoder.h"
#include "SbsDecoder.h"

#pragma link "HashTable\\Lib\\Win64\\Release\\HashTableLib.a"

// ==========================================================================
// [이동] CPR 위치 계산을 위한 헬퍼 함수들을 Aircraft.cpp에서 이곳으로 이동
// ==========================================================================
int cprModFunction(int a, int b)
{
    int res = a % b;
    if (res < 0) res += b;
    return res;
}

int AircraftDataModel::cprNLFunction(double lat)
{
    if (lat < 0) lat = -lat; /* Table is simmetric about the equator. */
    if (lat < 10.47047130) return 59;
    if (lat < 14.82817437) return 58;
    if (lat < 18.18626357) return 57;
	if (lat < 21.02939493) return 56;
    if (lat < 23.54504487) return 55;
    if (lat < 25.82924707) return 54;
    if (lat < 27.93898710) return 53;
    if (lat < 29.91135686) return 52;
    if (lat < 31.77209708) return 51;
    if (lat < 33.53993436) return 50;
    if (lat < 35.22899598) return 49;
    if (lat < 36.85025108) return 48;
    if (lat < 38.41241892) return 47;
    if (lat < 39.92256684) return 46;
    if (lat < 41.38651832) return 45;
    if (lat < 42.80914012) return 44;
    if (lat < 44.19454951) return 43;
    if (lat < 45.54626723) return 42;
    if (lat < 46.86733252) return 41;
    if (lat < 48.16039128) return 40;
    if (lat < 49.42776439) return 39;
    if (lat < 50.67150166) return 38;
    if (lat < 51.89342469) return 37;
    if (lat < 53.09516153) return 36;
    if (lat < 54.27817472) return 35;
    if (lat < 55.44378444) return 34;
    if (lat < 56.59318756) return 33;
    if (lat < 57.72747354) return 32;
    if (lat < 58.84763776) return 31;
    if (lat < 59.95459277) return 30;
    if (lat < 61.04917774) return 29;
    if (lat < 62.13216659) return 28;
    if (lat < 63.20427479) return 27;
    if (lat < 64.26616523) return 26;
    if (lat < 65.31845310) return 25;
    if (lat < 66.36171008) return 24;
    if (lat < 67.39646774) return 23;
    if (lat < 68.42322022) return 22;
    if (lat < 69.44242631) return 21;
    if (lat < 70.45451075) return 20;
    if (lat < 71.45986473) return 19;
    if (lat < 72.45884545) return 18;
    if (lat < 73.45177442) return 17;
    if (lat < 74.43893416) return 16;
    if (lat < 75.42056257) return 15;
    if (lat < 76.39684391) return 14;
    if (lat < 77.36789461) return 13;
    if (lat < 78.33374083) return 12;
    if (lat < 79.29428225) return 11;
    if (lat < 80.24923213) return 10;
    if (lat < 81.19801349) return 9;
    if (lat < 82.13956981) return 8;
    if (lat < 83.07199445) return 7;
    if (lat < 83.99173563) return 6;
    if (lat < 84.89166191) return 5;
    if (lat < 85.75541621) return 4;
    if (lat < 86.53536998) return 3;
    if (lat < 87.00000000) return 2;
    else return 1;
}

int AircraftDataModel::cprNFunction(double lat, int isodd)
{
    int nl = cprNLFunction(lat) - isodd;
    if (nl < 1) nl = 1;
    return nl;
}

double AircraftDataModel::cprDlonFunction(double lat, int isodd)
{
    return 360.0 / cprNFunction(lat, isodd);
}

void AircraftDataModel::decodeCPR(TADS_B_Aircraft *a)
{
    const double AirDlat0 = 360.0 / 60;
    const double AirDlat1 = 360.0 / 59;
    double lat0 = a->even_cprlat;
    double lat1 = a->odd_cprlat;
    double lon0 = a->even_cprlon;
    double lon1 = a->odd_cprlon;

    int j = floor(((59*lat0 - 60*lat1) / 131072) + 0.5);
    double rlat0 = AirDlat0 * (cprModFunction(j,60) + lat0 / 131072);
    double rlat1 = AirDlat1 * (cprModFunction(j,59) + lat1 / 131072);

    if (rlat0 >= 270) rlat0 -= 360;
    if (rlat1 >= 270) rlat1 -= 360;

    if (cprNLFunction(rlat0) != cprNLFunction(rlat1)) return;

    if (a->even_cprtime > a->odd_cprtime) {
        int ni = cprNFunction(rlat0,0);
        int m = floor((((lon0 * (cprNLFunction(rlat0)-1)) - (lon1 * cprNLFunction(rlat0))) / 131072) + 0.5);
        a->Longitude = cprDlonFunction(rlat0,0) * (cprModFunction(m,ni)+lon0/131072);
        a->Latitude = rlat0;
    } else {
        int ni = cprNFunction(rlat1,1);
        int m = floor((((lon0 * (cprNLFunction(rlat1)-1)) - (lon1 * cprNLFunction(rlat1))) / 131072.0) + 0.5);
        a->Longitude = cprDlonFunction(rlat1,1) * (cprModFunction(m,ni)+lon1/131072);
        a->Latitude = rlat1;
    }
    if (a->Longitude > 180) a->Longitude -= 360;
}


// ==========================================================================
// 클래스 멤버 함수 구현
// ==========================================================================

__fastcall AircraftDataModel::AircraftDataModel()
{
    FCurrentSpriteImage = 0;
    FHashTable = ght_create(50000);
    if (!FHashTable) {
        throw Sysutils::Exception("Create Hash Failed");
    }
    ght_set_rehash(FHashTable, TRUE);
}

__fastcall AircraftDataModel::~AircraftDataModel()
{
    PurgeAllAircraft();
    ght_finalize(FHashTable);
}

void AircraftDataModel::ProcessRawMessage(const AnsiString& data, bool ACycleImages, int ANumSpriteImages)
{
    ParsedAircraftData parsed = DecodeRawMessage(data);
    if (parsed.IsValid) {
        ApplyUpdate(parsed, ACycleImages, ANumSpriteImages);
    }
}

void AircraftDataModel::ProcessSbsMessage(const AnsiString& data, bool ACycleImages, int ANumSpriteImages)
{
    ParsedAircraftData parsed = DecodeSbsMessage(data.c_str());
    if (parsed.IsValid) {
        ApplyUpdate(parsed, ACycleImages, ANumSpriteImages);
    }
}

void AircraftDataModel::ApplyUpdate(const ParsedAircraftData& parsedData, bool ACycleImages, int ANumSpriteImages)
{
    TADS_B_Aircraft* aircraft = FindOrCreateAircraft(parsedData.ICAO, ACycleImages, ANumSpriteImages);
    if (!aircraft) return;

    aircraft->LastSeen = parsedData.Timestamp;

    if (parsedData.isCpr) {
        aircraft->NumMessagesRaw++;
        if (parsedData.isOddCpr) {
            aircraft->odd_cprlat = parsedData.cprLat;
            aircraft->odd_cprlon = parsedData.cprLon;
            aircraft->odd_cprtime = parsedData.Timestamp;
        } else {
            aircraft->even_cprlat = parsedData.cprLat;
            aircraft->even_cprlon = parsedData.cprLon;
            aircraft->even_cprtime = parsedData.Timestamp;
        }

        if (aircraft->odd_cprtime > 0 && aircraft->even_cprtime > 0 &&
            llabs(aircraft->even_cprtime - aircraft->odd_cprtime) <= 10000)
        {
            decodeCPR(aircraft);
            aircraft->HaveLatLon = true;
        }
    } else {
        aircraft->NumMessagesSBS++;
    }

    if (parsedData.hasFlightNum) {
        aircraft->HaveFlightNum = true;
        strncpy(aircraft->FlightNum, parsedData.FlightNum, 9);
        aircraft->FlightNum[8] = '\0';
    }
    if (parsedData.hasAltitude) {
        aircraft->HaveAltitude = true;
        aircraft->Altitude = parsedData.Altitude;
    }
    if (parsedData.hasPosition) {
        aircraft->HaveLatLon = true;
        aircraft->Latitude = parsedData.Latitude;
        aircraft->Longitude = parsedData.Longitude;
    }
    if (parsedData.hasSpeed && parsedData.hasHeading) {
        aircraft->HaveSpeedAndHeading = true;
        aircraft->Speed = parsedData.Speed;
        aircraft->Heading = parsedData.Heading;
    }
    if (parsedData.hasVerticalRate) {
        aircraft->VerticalRate = parsedData.VerticalRate;
    }
}


TADS_B_Aircraft* AircraftDataModel::FindOrCreateAircraft(unsigned int Addr, bool ACycleImages, int ANumSpriteImages)
{
    TADS_B_Aircraft* aircraft = (TADS_B_Aircraft*)ght_get(FHashTable, sizeof(Addr), &Addr);

    if (!aircraft) {
        aircraft = new TADS_B_Aircraft;
        memset(aircraft, 0, sizeof(TADS_B_Aircraft)); // 모든 필드를 0으로 초기화
        aircraft->ICAO = Addr;
        snprintf(aircraft->HexAddr, sizeof(aircraft->HexAddr), "%06X", (int)Addr);
        aircraft->SpriteImage = FCurrentSpriteImage;
        if (ACycleImages) {
            FCurrentSpriteImage = (FCurrentSpriteImage + 1) % ANumSpriteImages;
        }
        if (ght_insert(FHashTable, aircraft, sizeof(Addr), &Addr) < 0) {
            delete aircraft;
            return NULL;
        }
    }
    return aircraft;
}


void AircraftDataModel::PurgeStaleAircraft(int AStaleTimeSec)
{
    __int64 CurrentTime = GetCurrentTimeInMsec();
    __int64 StaleTimeInMs = AStaleTimeSec * 1000;
    
    ght_iterator_t iterator;
    const void* key;
    TADS_B_Aircraft* pData;

    for (pData = (TADS_B_Aircraft*)ght_first(FHashTable, &iterator, &key);
         pData;
         pData = (TADS_B_Aircraft*)ght_next(FHashTable, &iterator, &key))
    {
        if ((CurrentTime - pData->LastSeen) >= StaleTimeInMs)
        {
            void* pRemoved = ght_remove(FHashTable, sizeof(uint32_t), key);
            if(pRemoved)
            {
                delete static_cast<TADS_B_Aircraft*>(pRemoved);
            }
        }
    }
}


void AircraftDataModel::PurgeAllAircraft()
{
    ght_iterator_t iterator;
    const void* key;
    void* pData;

    // for 루프와 ght_remove 호출이 모두 Model 클래스 내부에 존재
    // 안전한 순회를 위해 ght_next를 먼저 호출 후 ght_remove를 수행하도록 수정 가능
    for (pData = ght_first(FHashTable, &iterator, &key);
         pData;
         /* pData는 루프 안에서 다음 값으로 갱신됨 */)
    {
        TADS_B_Aircraft* currentAircraft = static_cast<TADS_B_Aircraft*>(pData);
        pData = ght_next(FHashTable, &iterator, &key); 
        
        void* pRemoved = ght_remove(FHashTable, sizeof(uint32_t), &currentAircraft->ICAO);
        if(pRemoved)
        {
            delete static_cast<TADS_B_Aircraft*>(pRemoved);
        }
    }
}


int AircraftDataModel::GetAircraftCount()
{
    return ght_size(FHashTable);
}


TADS_B_Aircraft* AircraftDataModel::GetFirstAircraft(ght_iterator_t* iterator, const void** key)
{
    return (TADS_B_Aircraft*)ght_first(FHashTable, iterator, key);
}


TADS_B_Aircraft* AircraftDataModel::GetNextAircraft(ght_iterator_t* iterator, const void** key)
{
    return (TADS_B_Aircraft*)ght_next(FHashTable, iterator, key);
}


TADS_B_Aircraft* AircraftDataModel::FindAircraftByICAO(unsigned int Addr)
{
    return (TADS_B_Aircraft*)ght_get(FHashTable, sizeof(Addr), &Addr);
}