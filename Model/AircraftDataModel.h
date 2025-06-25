// ==== 파일: AircraftDataModel.h (최종) ====

#ifndef AircraftDataModelH
#define AircraftDataModelH

#include <System.hpp>
#include "Aircraft.h" // TADS_B_Aircraft 구조체 정의
#include "ght_hash_table.h"
#include "ParsedData.h" // 디코더가 반환하는 표준 데이터 구조

class TStringGrid; // Forward declaration

class AircraftDataModel
{
private:
    ght_hash_table_t* FHashTable;
    int FCurrentSpriteImage;

    // [신규] 모든 데이터 업데이트를 처리하는 중앙화된 private 메서드
    void ApplyUpdate(const ParsedAircraftData& parsedData, bool ACycleImages, int ANumSpriteImages);

    // [이동] CPR 위치 계산을 위한 헬퍼 함수들을 private으로 이동
    static int cprNLFunction(double lat);
    static int cprNFunction(double lat, int isodd);
    static double cprDlonFunction(double lat, int isodd);
    static void decodeCPR(TADS_B_Aircraft *a);

public:
    __fastcall AircraftDataModel();
    __fastcall ~AircraftDataModel();

    // 외부 인터페이스는 그대로 유지
    void ProcessRawMessage(const AnsiString& data, bool ACycleImages, int ANumSpriteImages);
    void ProcessSbsMessage(const AnsiString& data, bool ACycleImages, int ANumSpriteImages);

    TADS_B_Aircraft* FindOrCreateAircraft(unsigned int Addr, bool ACycleImages, int ANumSpriteImages);
    void PurgeStaleAircraft(int AStaleTimeSec);
    void PurgeAllAircraft();

    // 데이터 접근을 위한 public 메서드들
    int GetAircraftCount();
    TADS_B_Aircraft* GetFirstAircraft(ght_iterator_t* iterator, const void** key);
    TADS_B_Aircraft* GetNextAircraft(ght_iterator_t* iterator, const void** key);
    TADS_B_Aircraft* FindAircraftByICAO(unsigned int Addr);
};

#endif