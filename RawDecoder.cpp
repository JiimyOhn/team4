// ==== 파일: RawDecoder.cpp (최종 완전판) ====

#pragma hdrstop
#include "RawDecoder.h"
#include "TimeFunctions.h"

#include <cstring>
#include <cmath>
#include <vcl.h> // for tolower, isspace

// ==========================================================================
// 기존 DecodeRawADS_B.h의 모든 내용을 여기에 직접 정의하여 캡슐화
// ==========================================================================

#define MODES_LONG_MSG_BITS 112
#define MODES_SHORT_MSG_BITS 56
#define MODES_LONG_MSG_BYTES (MODES_LONG_MSG_BITS/8)
#define MODES_SHORT_MSG_BYTES (MODES_SHORT_MSG_BITS/8)
#define MODES_ICAO_CACHE_LEN 1024
#define MODES_ICAO_CACHE_TTL 60
#define TWO_PI 6.28318530717958647692

// Raw 디코더 내부에서만 사용되는 임시 데이터 구조체
typedef struct {
    unsigned char msg[MODES_LONG_MSG_BYTES];
    int msg_bits;
    int msg_type;
    int error_bit;
    int CRC_ok;
    uint32_t CRC;
    int ca;
    unsigned char AA[3];
    int flight_status;
    int DR_status;
    int UM_status;
    int identity;
    int ME_type;
    int ME_subtype;
    int aircraft_type;
    char flight[9];
    int odd_flag;
    int UTC_flag;
    int altitude;
    int raw_latitude;
    int raw_longitude;
    int EW_dir;
    int EW_velocity;
    int NS_dir;
    int NS_velocity;
    int vert_rate_source;
    int vert_rate_sign;
    int vert_rate;
    int velocity;
    int heading;
    bool heading_is_valid;
    bool phase_corrected;
    enum metric_unit_t { MODES_UNIT_FEET, MODES_UNIT_METERS } unit;
} modeS_message;

const bool error_correct_1 = true;
const bool error_correct_2 = false;

// -- Static Function Prototypes and Variables --
static int hex_digit_val (int c);
static void decode_modeS_message (modeS_message *mm, const uint8_t *_msg);
static int modeS_message_len_by_type (int type);
static uint32_t CRC_get (const uint8_t *msg, int bits);
static uint32_t CRC_check (const uint8_t *msg, int bits);
static int fix_single_bit_errors (uint8_t *msg, int bits);
static int fix_two_bits_errors (uint8_t *msg, int bits);
static bool brute_force_AP (const uint8_t *msg, modeS_message *mm);
static int decode_AC12_field (uint8_t *msg, modeS_message::metric_unit_t *unit);
static int decode_AC13_field (const uint8_t *msg, modeS_message::metric_unit_t *unit);
static uint32_t aircraft_get_addr (uint8_t a0, uint8_t a1, uint8_t a2);
static void ICAO_cache_add_address (uint32_t addr);
static bool ICAO_address_recently_seen (uint32_t addr);
static uint32_t ICAO_cache_hash_address (uint32_t a);

static uint32_t *ICAO_cache = NULL; // 실제 ICAO 캐시 데이터
static const uint32_t checksum_table [MODES_LONG_MSG_BITS] = {
    0x3935EA, 0x1C9AF5, 0xF1B77E, 0x78DBBF, 0xC397DB, 0x9E31E9, 0xB0E2F0, 0x587178,
    0x2C38BC, 0x161C5E, 0x0B0E2F, 0xFA7D13, 0x82C48D, 0xBE9842, 0x5F4C21, 0xD05C14,
    0x682E0A, 0x341705, 0xE5F186, 0x72F8C3, 0xC68665, 0x9CB936, 0x4E5C9B, 0xD8D449,
    0x939020, 0x49C810, 0x24E408, 0x127204, 0x093902, 0x049C81, 0xFDB444, 0x7EDA22,
    0x3F6D11, 0xE04C8C, 0x702646, 0x381323, 0xE3F395, 0x8E03CE, 0x4701E7, 0xDC7AF7,
    0x91C77F, 0xB719BB, 0xA476D9, 0xADC168, 0x56E0B4, 0x2B705A, 0x15B82D, 0xF52612,
    0x7A9309, 0xC2B380, 0x6159C0, 0x30ACE0, 0x185670, 0x0C2B38, 0x06159C, 0x030ACE,
    0x018567, 0xFF38B7, 0x80665F, 0xBFC92B, 0xA01E91, 0xAFF54C, 0x57FAA6, 0x2BFD53,
    0xEA04AD, 0x8AF852, 0x457C29, 0xDD4410, 0x6EA208, 0x375104, 0x1BA882, 0x0DD441,
    0xF91024, 0x7C8812, 0x3E4409, 0xE0D800, 0x706C00, 0x383600, 0x1C1B00, 0x0E0D80,
    0x0706C0, 0x038360, 0x01C1B0, 0x00E0D8, 0x00706C, 0x003836, 0x001C1B, 0xFFF409,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};

// ==========================================================================
// 모든 static helper 함수들의 전체 구현부
// ==========================================================================
static int hex_digit_val(int c) {
    c = tolower(c);
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int modeS_message_len_by_type (int type) {
    if (type == 16 || type == 17 || type == 19 || type == 20 || type == 21)
        return (MODES_LONG_MSG_BITS);
    return (MODES_SHORT_MSG_BITS);
}

static uint32_t CRC_get (const uint8_t *msg, int bits) {
    uint32_t CRC = ((uint32_t) msg [(bits / 8) - 3] << 16) |
                   ((uint32_t) msg [(bits / 8) - 2] << 8) |
                   (uint32_t) msg [(bits / 8) - 1];
    return (CRC);
}

static uint32_t CRC_check (const uint8_t *msg, int bits) {
    uint32_t crc = 0;
    int offset = (bits == MODES_LONG_MSG_BITS) ? 0 : (MODES_LONG_MSG_BITS - MODES_SHORT_MSG_BITS);
    for (int j = 0; j < bits; j++) {
        int byte = j / 8;
        int bit  = j % 8;
        int mask = 1 << (7 - bit);
        if (msg[byte] & mask)
            crc ^= checksum_table[j + offset];
    }
    return (crc);
}

static int fix_single_bit_errors (uint8_t *msg, int bits) {
    uint8_t aux[MODES_LONG_MSG_BYTES];
    for (int i = 0; i < bits; i++) {
        int byte = i / 8;
        int mask = 1 << (7 - (i % 8));
        memcpy(aux, msg, bits / 8);
        aux[byte] ^= mask;
        if (CRC_get(aux, bits) == CRC_check(aux, bits)) {
            memcpy(msg, aux, bits / 8);
            return i;
        }
    }
    return -1;
}

static int decode_AC12_field (uint8_t *msg, modeS_message::metric_unit_t *unit) {
    int q_bit = msg[5] & 1;
    if (q_bit) {
        *unit = modeS_message::MODES_UNIT_FEET;
        int n = ((msg[5] >> 1) << 4) | ((msg[6] & 0xF0) >> 4);
        int ret = 25 * n - 1000;
        return (ret < 0) ? 0 : ret;
    }
    return 0;
}

static int decode_AC13_field (const uint8_t *msg, modeS_message::metric_unit_t *unit) {
    int m_bit = msg[3] & (1 << 6);
    int q_bit = msg[3] & (1 << 4);
    if (!m_bit) {
        *unit = modeS_message::MODES_UNIT_FEET;
        if (q_bit) {
            int n = ((msg[2] & 31) << 6) | ((msg[3] & 0x80) >> 2) | ((msg[3] & 0x20) >> 1) | (msg[3] & 15);
            int ret = 25 * n - 1000;
            return (ret < 0) ? 0 : ret;
        }
    }
    return 0;
}


// ==========================================================================
// [중요] decode_modeS_message 함수의 전체 구현부
// ==========================================================================
static void decode_modeS_message (modeS_message *mm, const uint8_t *_msg)
{
    uint32_t CRC;
    const char *AIS_charset = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";
    uint8_t *msg;

    memset(mm, 0, sizeof(*mm));
    memcpy(mm->msg, _msg, sizeof(mm->msg));
    msg = mm->msg;

    mm->msg_type = msg[0] >> 3;
    mm->msg_bits = modeS_message_len_by_type(mm->msg_type);
    mm->CRC = CRC_get(msg, mm->msg_bits);
    CRC = CRC_check(msg, mm->msg_bits);

    mm->CRC_ok = (mm->CRC == CRC);
    mm->error_bit = -1;

    if (!mm->CRC_ok && error_correct_1 && (mm->msg_type == 11 || mm->msg_type == 17)) {
        mm->error_bit = fix_single_bit_errors(msg, mm->msg_bits);
        if (mm->error_bit != -1) {
            mm->CRC_ok = true;
        }
    }

    // --- 지금부터 생략되었던 모든 디코딩 로직 ---
    mm->ca = msg[0] & 7;

    mm->AA[0] = msg[1];
    mm->AA[1] = msg[2];
    mm->AA[2] = msg[3];

    // DF 17 (Extended Squitter)
    if (mm->msg_type == 17) {
        mm->ME_type = msg[4] >> 3;
        mm->ME_subtype = msg[4] & 7;

        // Aircraft Identification
        if (mm->ME_type >= 1 && mm->ME_type <= 4) {
            mm->aircraft_type = mm->ME_type - 1;
            mm->flight[0] = AIS_charset[msg[5] >> 2];
            mm->flight[1] = AIS_charset[((msg[5] & 3) << 4) | (msg[6] >> 4)];
            mm->flight[2] = AIS_charset[((msg[6] & 15) << 2) | (msg[7] >> 6)];
            mm->flight[3] = AIS_charset[msg[7] & 63];
            mm->flight[4] = AIS_charset[msg[8] >> 2];
            mm->flight[5] = AIS_charset[((msg[8] & 3) << 4) | (msg[9] >> 4)];
            mm->flight[6] = AIS_charset[((msg[9] & 15) << 2) | (msg[10] >> 6)];
            mm->flight[7] = AIS_charset[msg[10] & 63];
            mm->flight[8] = '\0';
            char *p = mm->flight + 7;
            while (*p == ' ') *p-- = '\0'; // Remove trailing spaces
        }
        // Airborne Position
        else if (mm->ME_type >= 9 && mm->ME_type <= 18) {
            mm->odd_flag = msg[6] & (1 << 2);
            mm->UTC_flag = msg[6] & (1 << 3);
            mm->altitude = decode_AC12_field(msg, &mm->unit);
            mm->raw_latitude = ((msg[6] & 3) << 15) | (msg[7] << 7) | (msg[8] >> 1);
            mm->raw_longitude = ((msg[8] & 1) << 16) | (msg[9] << 8) | msg[10];
        }
        // Airborne Velocity
        else if (mm->ME_type == 19 && mm->ME_subtype >= 1 && mm->ME_subtype <= 4) {
            if (mm->ME_subtype == 1 || mm->ME_subtype == 2) {
                mm->EW_dir = (msg[5] & 4) >> 2;
                mm->EW_velocity = ((msg[5] & 3) << 8) | msg[6];
                mm->NS_dir = (msg[7] & 0x80) >> 7;
                mm->NS_velocity = ((msg[7] & 0x7F) << 3) | ((msg[8] & 0xE0) >> 5);
                mm->vert_rate_source = (msg[8] & 0x10) >> 4;
                mm->vert_rate_sign = (msg[8] & 0x08) >> 3;
                mm->vert_rate = ((msg[8] & 7) << 6) | ((msg[9] & 0xFC) >> 2);
                mm->velocity = (int)hypot((double)mm->NS_velocity, (double)mm->EW_velocity);
                if (mm->velocity) {
                    int ewV = (mm->EW_dir ? -1 : 1) * mm->EW_velocity;
                    int nsV = (mm->NS_dir ? -1 : 1) * mm->NS_velocity;
                    double heading = atan2(ewV, nsV);
                    mm->heading = (int)(heading * 360 / TWO_PI);
                    if (mm->heading < 0) mm->heading += 360;
                    mm->heading_is_valid = true;
                }
            }
        }
    }
    // DF 0, 4, 16, 20
    else if (mm->msg_type == 0 || mm->msg_type == 4 || mm->msg_type == 16 || mm->msg_type == 20) {
        mm->flight_status = msg[0] & 7;
        mm->altitude = decode_AC13_field(msg, &mm->unit);
    }
    // DF 5, 21
    else if (mm->msg_type == 5 || mm->msg_type == 21) {
        mm->flight_status = msg[0] & 7;
        int a = ((msg[3] & 0x80) >> 5) | ((msg[2] & 0x02) >> 0) | ((msg[2] & 0x08) >> 3);
        int b = ((msg[3] & 0x02) << 1) | ((msg[3] & 0x08) >> 2) | ((msg[3] & 0x20) >> 5);
        int c = ((msg[2] & 0x01) << 2) | ((msg[2] & 0x04) >> 1) | ((msg[2] & 0x10) >> 4);
        int d = ((msg[3] & 0x01) << 2) | ((msg[3] & 0x04) >> 1) | ((msg[3] & 0x10) >> 4);
        mm->identity = a*1000 + b*100 + c*10 + d;
    }
}


// `modeS_message`를 `ParsedAircraftData`로 변환하는 헬퍼 함수
static void fillParsedDataFromModeS(const modeS_message& mm, ParsedAircraftData& result) {
    result.IsValid = true;
    result.ICAO = (mm.AA[0] << 16) | (mm.AA[1] << 8) | mm.AA[2];
    result.Timestamp = GetCurrentTimeInMsec();

    if (mm.msg_type == 0 || mm.msg_type == 4 || mm.msg_type == 20 || mm.msg_type == 16) {
        result.hasAltitude = true;
        result.Altitude = mm.altitude;
    } else if (mm.msg_type == 17) {
        if (mm.ME_type >= 1 && mm.ME_type <= 4) {
            result.hasFlightNum = true;
            memcpy(result.FlightNum, mm.flight, 8);
            result.FlightNum[8] = '\0';
        } else if (mm.ME_type >= 9 && mm.ME_type <= 18) {
            result.hasAltitude = true;
            result.Altitude = mm.altitude;
            result.isCpr = true;
            result.isOddCpr = mm.odd_flag;
            result.cprLat = mm.raw_latitude;
            result.cprLon = mm.raw_longitude;
        } else if (mm.ME_type == 19 && (mm.ME_subtype == 1 || mm.ME_subtype == 2)) {
            if (mm.velocity > 0) {
                result.hasSpeed = true;
                result.Speed = mm.velocity;
            }
            if (mm.heading_is_valid) {
                result.hasHeading = true;
                result.Heading = mm.heading;
            }
            result.hasVerticalRate = true;
            result.VerticalRate = (mm.vert_rate_sign == 0 ? 1 : -1) * (mm.vert_rate - 1) * 64;
        }
    }
}


// ==========================================================================
// 공개 함수 (Public Function) 구현
// ==========================================================================
ParsedAircraftData DecodeRawMessage(const AnsiString& MsgIn)
{
    if (ICAO_cache == NULL) {
        // calloc을 사용하여 메모리를 할당하고 0으로 초기화합니다.
        ICAO_cache = (uint32_t *)calloc(2 * sizeof(uint32_t) * MODES_ICAO_CACHE_LEN, 1);
        // 만약 할당에 실패하면 이후 로직이 안전하게 실패하도록 처리할 수 있습니다.
        if (ICAO_cache == NULL) {
             // 적절한 오류 처리 (예: 예외 발생 또는 빈 데이터 반환)
             return ParsedAircraftData();
        }
    }

    ParsedAircraftData result;

    uint8_t bin_msg[MODES_LONG_MSG_BYTES];
    int len = MsgIn.Length();
    const char* hex = MsgIn.c_str();

    if (len < 3 || hex[0] != '*' || hex[len-1] != ';') {
        return result;
    }

    hex++;
    len -= 2;

    if (len > 2 * MODES_LONG_MSG_BYTES || len % 2 != 0) {
        return result;
    }

    for (int j = 0; j < len; j += 2) {
        int high = hex_digit_val(hex[j]);
        int low  = hex_digit_val(hex[j+1]);
        if (high == -1 || low == -1) {
            return result;
        }
        bin_msg[j/2] = (high << 4) | low;
    }

    modeS_message mm;
    decode_modeS_message(&mm, bin_msg);

    if (mm.CRC_ok) {
        fillParsedDataFromModeS(mm, result);
    }

    return result;
}