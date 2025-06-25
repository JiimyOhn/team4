#ifndef SbsDecoderH
#define SbsDecoderH

#include "ParsedData.h"

// SBS 메시지를 파싱하여 표준 구조체로 반환하는 순수 함수
ParsedAircraftData DecodeSbsMessage(const char* msg);

#endif