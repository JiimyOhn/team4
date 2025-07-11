﻿#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "IAviationProvider.h"
#include "RouteInfo.h"

class RouteProvider : public IAviationProvider<RouteInfo> {
public:
    RouteProvider();
    ~RouteProvider();

    // routes.csv를 지정 경로로 다운로드
    bool DownloadAndParseAviationData(const std::string& savePath) override;

    // 실시간 파싱: batch 단위로 콜백 (finished==true면 전체 완료)
    void DownloadAndParseAviationDataAsync(
        std::function<void(const std::vector<RouteInfo>&, bool finished)> onBatchParsed
    ) override;

private:
    std::string buffer;
    std::mutex mtx;
    std::condition_variable cv;
    bool downloadFinished = false;
    bool downloadSuccess = false;
};