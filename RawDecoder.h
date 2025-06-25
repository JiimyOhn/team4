// ==== 파일: RawDecoder.h ====

#ifndef RawDecoderH
#define RawDecoderH

#include "ParsedData.h"
#include <System.hpp> // AnsiString

// Raw 메시지를 파싱하여 표준 구조체로 반환하는 순수 함수
ParsedAircraftData DecodeRawMessage(const AnsiString& MsgIn);

#endif