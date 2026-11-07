// Prints a few event lines (NDJSON) made by EventsFormat; check.py feeds them to the collector.
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "events_format.h"

static int check_private() {
    int bad = 0;
    // The rule: RFONLY or NOGATE anywhere before the first ':', in any case; a header that cannot be read counts as private.
    struct { const char* t; bool want; } c[] = {
        { "K6ATV-4>APLRT1,RFONLY:=x", true },            { "K6ATV-4>APLRT1,NOGATE:=x", true },
        { "K6ATV-4>APLRT1,WIDE1-1,RFONLY*:=x", true },   { "K6ATV-4>APLRT1,nogate:=x", true },
        { "K6ATV-4>APLRT1,RFONLY-1:=x", true },          { "K6ATV-4>APLRT1,NOGATE-2*:=x", true },       // the iGate's own gating rule matches these too
        { "K6ATV-4>APLRT1,RFONLYX:=x", true },
        { "K6ATV-4>APLRT1,WIDE1-1,W2,W3,W4,W5,W6,W7,W8,W9,W10,W11,RFONLY,W13,W14,W15:=x", true },       // far down a long path
        { "K6ATV-4>APLRT1,WIDE1-1:=x", false },          { "K6ATV-4>APLRT1:=NOGATE in the text", false },     // only the header counts
        { "K6ATV-4>APLRT1:=hello: RFONLY", false },
        { "garbage", true },                              { "no colon K6ATV-4>APLRT1,WIDE1-1", true },
        { "K6ATV-4 APLRT1,WIDE1-1:=x", true },            { ":K6ATV-4>APLRT1,WIDE1-1", true },                 // no '>' before the ':'
        { "", true },
    };
    for (auto& k : c) if (EventsFormat::privatePath(k.t) != k.want) { printf("privatePath wrong for: %s\n", k.t); bad++; }
    printf("privatePath: %d cases, %d wrong\n", (int)(sizeof c / sizeof c[0]), bad);
    return bad;
}

// The class a frame is reported as, from its first bytes (used for frames that cannot be decoded and for CRC errors).
static int check_classify() {
    using namespace EventsFormat;
    int bad = 0;
    struct { const char* what; std::vector<uint8_t> f; int want; } c[] = {
        { "plain 3C FF 01",                        { 0x3C, 0xFF, 0x01, 'A' },             PLAIN },
        { "whole-text PPM 3C FF 81",               { 0x3C, 0xFF, 0x81, 0x00 },            PPM_FULL },
        { "primed PPM 3C FF C1",                   { 0x3C, 0xFF, 0xC1, 0x00 },            PPM_PRIMED },
        { "LACF 2.0.0, kind 2 (LACF content)",     { 0x3C, 0xFF, 0x02, 0x22, 0x00 },      HYBRID },
        { "LACF 2.0.0, kind 0 (plain content)",    { 0x3C, 0xFF, 0x02, 0x20, 0x00 },      NATIVE },
        { "LACF 3.0.0-dev.1, kind 2",              { 0x3C, 0xFF, 0x03, 0x22, 0x00 },      HYBRID },
        { "LACF 3.0.0-dev.1, kind 0",              { 0x3C, 0xFF, 0x03, 0x20, 0x00 },      HYBRID },      // every kind of type 03 is hybrid, as 2.0.0 is for the collector
        { "LACF 3.0.0-dev.1, three bytes only",    { 0x3C, 0xFF, 0x03 },                  HYBRID },
        { "another type byte",                     { 0x3C, 0xFF, 0x04, 0x00 },            UNKNOWN },
        { "not a LoRa APRS frame",                 { 0x00, 0x01, 0x02 },                  UNKNOWN },
        { "too short",                             { 0x3C, 0xFF },                        UNKNOWN },
    };
    for (auto& k : c) if (classify(k.f.data(), k.f.size()) != k.want) { printf("classify wrong for: %s\n", k.what); bad++; }
    printf("classify: %d cases, %d wrong\n", (int)(sizeof c / sizeof c[0]), bad);
    return bad;
}

int main(int argc, char** argv) {
    (void)argv;
    if (argc > 1) return check_private() + check_classify();
    EventsFormat::Head h = { "a1b2c3d4", 1, 1793545200123LL, "K6ATV-14" };
    EventsFormat::Radio r = { 433775000, 125000, 12, 5 };
    EventsFormat::Counts c = { { 5, 2, 1, 0, 0, 3 }, 0, 0, 4 };

    printf("%s\n", EventsFormat::rx(h, r, -107, -19.75f, -4331, EventsFormat::PLAIN, 57, 57, "K6ATV-10>APDW19:!3701.16NL12137.08W&K6ATV-10 LoRa APRS").c_str());
    h.sequence = 2;
    printf("%s\n", EventsFormat::rx(h, r, -90, 5.5f, 120, EventsFormat::HYBRID, 39, 57, "K6ATV-14>APLRT1,N0RLY-1*,WIDE2-1:=3701.16N/12137.08W-quote\" back\\ \x01 \xe9").c_str());
    h.sequence = 3;
    printf("%s\n", EventsFormat::rx(h, r, -100, 0.0f, 0, EventsFormat::HYBRID, 30, 45, "K6ATV-4>APLRT1,WIDE1-1:=hello").c_str());
    h.sequence = 4;
    printf("%s\n", EventsFormat::tx(h, r, EventsFormat::HYBRID, 58, true).c_str());
    h.sequence = 5;
    EventsFormat::Noise n = { 59, -107.3, -111, -98 };
    printf("%s\n", EventsFormat::status(h, 3600, c, n).c_str());
    h.sequence = 6;
    const uint8_t bad[] = { 0x3C, 0xFF, 0x02, 0x22, 0x00, 0xAB };
    printf("%s\n", EventsFormat::rxUndecoded(h, r, -118, -17.5f, -3600, EventsFormat::classify(bad, sizeof bad), EventsFormat::CRC_ERROR, sizeof bad, bad, sizeof bad).c_str());
    h.sequence = 7;
    printf("%s\n", EventsFormat::rxUndecoded(h, r, -100, 2.0f, 50, EventsFormat::PPM_PRIMED, EventsFormat::DROPPED_NO_REFERENCE, 20, nullptr, 0).c_str());
    h.sequence = 8;
    printf("%s\n", EventsFormat::status(h, 3660, c, EventsFormat::Noise{ 0, 0, 0, 0 }).c_str());
    return 0;
}
