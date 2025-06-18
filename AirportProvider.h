#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>

struct AirportInfo {
    std::string Code;
    std::string Name;
    std::string ICAO;
    std::string IATA;
    std::string Location;
    std::string CountryISO2;
    std::string Latitude;
    std::string Longitude;
    std::string AltitudeFeet;
};

class AirportProvider {
public:
    AirportProvider();
    ~AirportProvider();

    // airports.csv를 지정 경로로 다운로드
    bool DownloadAirportsCSV(const std::string& savePath);

    // 콜백: (파싱된 벡터, finished)
    void DownloadAndParseAirportsCSV(
        std::function<void(const std::vector<AirportInfo>&, bool finished)> onBatchParsed
    );

private:
    std::string buffer;
    std::mutex mtx;
    std::condition_variable cv;
    bool downloadFinished = false;
    bool downloadSuccess = false;
};