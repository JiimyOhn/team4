#ifndef ParsedDataH
#define ParsedDataH

struct ParsedAircraftData
{
    // 파싱 성공 여부를 나타내는 플래그
    bool IsValid;

    // 필수 데이터
    unsigned int ICAO;
    __int64 Timestamp;

    // 옵셔널 데이터 (파싱 성공 여부 플래그와 함께)
    bool hasFlightNum;
    char FlightNum[9];

    bool hasAltitude;
    double Altitude;

    bool hasSpeed;
    double Speed;

    bool hasHeading;
    double Heading;

    bool hasPosition;
    double Latitude;
    double Longitude;

    bool hasVerticalRate;
    double VerticalRate;

    // CPR 데이터는 Raw 디코더만 채움
    bool isCpr;
    bool isOddCpr;
    int cprLat;
    int cprLon;

    // 생성자에서 모든 값을 기본값으로 초기화
    ParsedAircraftData() :
        IsValid(false), ICAO(0), Timestamp(0),
        hasFlightNum(false), hasAltitude(false), hasSpeed(false),
        hasHeading(false), hasPosition(false), hasVerticalRate(false),
        isCpr(false), isOddCpr(false), cprLat(0), cprLon(0)
    {
        memset(FlightNum, 0, sizeof(FlightNum));
    }
};

#endif