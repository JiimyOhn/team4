#include "AirportProvider.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>

// #pragma comment(lib, "wininet.lib")
#pragma comment(lib, "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.19041.0\\um\\x86\\WinInet.Lib")

AirportProvider::AirportProvider() {}
AirportProvider::~AirportProvider() {}

bool AirportProvider::DownloadAirportsCSV(const std::string& savePath) {
    const char* url = "https://vrs-standing-data.adsb.lol/airports.csv";
    HINTERNET hInternet = InternetOpenA("AirportProvider", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile.is_open()) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    BOOL bResult = TRUE;
    while (bResult) {
        bResult = InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead);
        if (bytesRead == 0) break;
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return true;
}

// 비동기 다운로드 및 실시간 파싱
void AirportProvider::DownloadAndParseAirportsCSV(
    std::function<void(const std::vector<AirportInfo>&, bool finished)> onBatchParsed)
{
    buffer.clear();
    downloadFinished = false;
    downloadSuccess = false;

    // 파싱 스레드
    std::thread parsingThread([this, onBatchParsed]() {
        std::vector<AirportInfo> batch;
        std::string incompleteLine;
        bool isHeader = true;
        std::vector<int> colIdx(9, -1);
        auto lastBatchTime = std::chrono::steady_clock::now();

        while (true) {
            std::string localBuffer;
            bool localDownloadFinished = false;

            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait_for(lock, std::chrono::milliseconds(100), [this] { return !buffer.empty() || downloadFinished; });
                localBuffer.swap(buffer);
                localDownloadFinished = downloadFinished;
            }

            // incompleteLine에서 이어서 붙임
            if (!incompleteLine.empty()) {
                localBuffer = incompleteLine + localBuffer;
                incompleteLine.clear();
            }

            std::istringstream ss(localBuffer);
            std::string line;
            while (std::getline(ss, line)) {
                if (ss.eof() && localBuffer.back() != '\n' && localBuffer.back() != '\r') {
                    // 마지막 줄이 완전하지 않음
                    incompleteLine = line;
                    break;
                }
                std::vector<std::string> cols;
                std::string cell;
                std::istringstream ls(line);
                while (std::getline(ls, cell, ',')) {
                    cols.push_back(cell);
                }
                if (isHeader) {
                    for (size_t i = 0; i < cols.size(); ++i) {
                        if (cols[i] == "Code") colIdx[0] = i;
                        else if (cols[i] == "Name") colIdx[1] = i;
                        else if (cols[i] == "ICAO") colIdx[2] = i;
                        else if (cols[i] == "IATA") colIdx[3] = i;
                        else if (cols[i] == "Location") colIdx[4] = i;
                        else if (cols[i] == "CountryISO2") colIdx[5] = i;
                        else if (cols[i] == "Latitude") colIdx[6] = i;
                        else if (cols[i] == "Longitude") colIdx[7] = i;
                        else if (cols[i] == "AltitudeFeet") colIdx[8] = i;
                    }
                    isHeader = false;
                    continue;
                }
                if (cols.size() < 9) continue;
                AirportInfo info;
                if (colIdx[0] >= 0 && colIdx[0] < (int)cols.size()) info.Code = cols[colIdx[0]];
                if (colIdx[1] >= 0 && colIdx[1] < (int)cols.size()) info.Name = cols[colIdx[1]];
                if (colIdx[2] >= 0 && colIdx[2] < (int)cols.size()) info.ICAO = cols[colIdx[2]];
                if (colIdx[3] >= 0 && colIdx[3] < (int)cols.size()) info.IATA = cols[colIdx[3]];
                if (colIdx[4] >= 0 && colIdx[4] < (int)cols.size()) info.Location = cols[colIdx[4]];
                if (colIdx[5] >= 0 && colIdx[5] < (int)cols.size()) info.CountryISO2 = cols[colIdx[5]];
                if (colIdx[6] >= 0 && colIdx[6] < (int)cols.size()) info.Latitude = cols[colIdx[6]];
                if (colIdx[7] >= 0 && colIdx[7] < (int)cols.size()) info.Longitude = cols[colIdx[7]];
                if (colIdx[8] >= 0 && colIdx[8] < (int)cols.size()) info.AltitudeFeet = cols[colIdx[8]];
                batch.push_back(info);

                // 100개마다 콜백
                if (batch.size() >= 100) {
                    // 파싱 결과 출력
                    std::cout << "[Batch Parsed] size=" << batch.size() << std::endl;
                    for (const auto& a : batch) {
                        std::cout << a.Code << ", " << a.Name << ", " << a.ICAO << ", " << a.IATA << ", "
                                  << a.Location << ", " << a.CountryISO2 << ", " << a.Latitude << ", "
                                  << a.Longitude << ", " << a.AltitudeFeet << std::endl;
                    }
                    onBatchParsed(batch, false);
                    batch.clear();
                    lastBatchTime = std::chrono::steady_clock::now();
                }
            }

            // 100개가 안되더라도 0.5초마다 콜백
            auto now = std::chrono::steady_clock::now();
            if (!batch.empty() && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBatchTime).count() >= 500) {
                // 파싱 결과 출력
                std::cout << "[Batch Parsed] size=" << batch.size() << std::endl;
                for (const auto& a : batch) {
                    std::cout << a.Code << ", " << a.Name << ", " << a.ICAO << ", " << a.IATA << ", "
                              << a.Location << ", " << a.CountryISO2 << ", " << a.Latitude << ", "
                              << a.Longitude << ", " << a.AltitudeFeet << std::endl;
                }
                onBatchParsed(batch, false);
                batch.clear();
                lastBatchTime = now;
            }

            if (localDownloadFinished) {
                // 남은 incompleteLine 처리
                if (!incompleteLine.empty()) {
                    std::string lastLine = incompleteLine;
                    incompleteLine.clear();
                    std::vector<std::string> cols;
                    std::string cell;
                    std::istringstream ls(lastLine);
                    while (std::getline(ls, cell, ',')) {
                        cols.push_back(cell);
                    }
                    if (!isHeader && cols.size() >= 9) {
                        AirportInfo info;
                        if (colIdx[0] >= 0 && colIdx[0] < (int)cols.size()) info.Code = cols[colIdx[0]];
                        if (colIdx[1] >= 0 && colIdx[1] < (int)cols.size()) info.Name = cols[colIdx[1]];
                        if (colIdx[2] >= 0 && colIdx[2] < (int)cols.size()) info.ICAO = cols[colIdx[2]];
                        if (colIdx[3] >= 0 && colIdx[3] < (int)cols.size()) info.IATA = cols[colIdx[3]];
                        if (colIdx[4] >= 0 && colIdx[4] < (int)cols.size()) info.Location = cols[colIdx[4]];
                        if (colIdx[5] >= 0 && colIdx[5] < (int)cols.size()) info.CountryISO2 = cols[colIdx[5]];
                        if (colIdx[6] >= 0 && colIdx[6] < (int)cols.size()) info.Latitude = cols[colIdx[6]];
                        if (colIdx[7] >= 0 && colIdx[7] < (int)cols.size()) info.Longitude = cols[colIdx[7]];
                        if (colIdx[8] >= 0 && colIdx[8] < (int)cols.size()) info.AltitudeFeet = cols[colIdx[8]];
                        batch.push_back(info);
                    }
                }
                // 남은 데이터도 콜백
                if (!batch.empty()) {
                    std::cout << "[Batch Parsed] size=" << batch.size() << std::endl;
                    for (const auto& a : batch) {
                        std::cout << a.Code << ", " << a.Name << ", " << a.ICAO << ", " << a.IATA << ", "
                                  << a.Location << ", " << a.CountryISO2 << ", " << a.Latitude << ", "
                                  << a.Longitude << ", " << a.AltitudeFeet << std::endl;
                    }
                    onBatchParsed(batch, false);
                    batch.clear();
                }
                // 파싱 완료 콜백
                onBatchParsed({}, true);
                break;
            }
        }
    });

    // 다운로드 스레드
    std::thread downloadThread([this]() {
        const char* url = "https://vrs-standing-data.adsb.lol/airports.csv";
        HINTERNET hInternet = InternetOpenA("AirportProvider", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) {
            std::lock_guard<std::mutex> lock(mtx);
            downloadFinished = true;
            downloadSuccess = false;
            cv.notify_all();
            return;
        }

        HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hFile) {
            InternetCloseHandle(hInternet);
            std::lock_guard<std::mutex> lock(mtx);
            downloadFinished = true;
            downloadSuccess = false;
            cv.notify_all();
            return;
        }

        char bufferChunk[4096];
        DWORD bytesRead = 0;
        BOOL bResult = TRUE;
        while (bResult) {
            bResult = InternetReadFile(hFile, bufferChunk, sizeof(bufferChunk), &bytesRead);
            if (bytesRead == 0) break;
            {
                std::lock_guard<std::mutex> lock(mtx);
                buffer.append(bufferChunk, bytesRead);
                cv.notify_all();
            }
        }

        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);

        std::lock_guard<std::mutex> lock(mtx);
        downloadFinished = true;
        downloadSuccess = true;
        cv.notify_all();
    });

    downloadThread.detach();
    parsingThread.detach();
}