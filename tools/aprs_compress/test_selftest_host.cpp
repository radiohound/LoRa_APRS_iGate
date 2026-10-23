// Checks the over-the-air test list on a PC: every packet is expanded, compressed, decoded and
// recognised by check().   c++ -std=c++17 -I lib/AprsCompress/src tools/aprs_compress/test_selftest_host.cpp \
//     lib/AprsCompress/src/AprsCompress*.cpp -o test_selftest_host
#include "AprsCompress.h"
#include "AprsCompressSelfTest.h"
#include <cstdio>
#include <cstring>

int main() {
    int bad = 0;
    for (int i = 0; i < AprsCompressSelfTest::count(); i++) {
        char text[300], back[300]; uint8_t frame[300];
        size_t tl = AprsCompressSelfTest::expand(i, "N0CALL-7", text, sizeof text);
        size_t fl = AprsCompress::encodeFrame((const uint8_t*)text, tl, frame, sizeof frame);
        if (fl) {
            size_t bl = AprsCompress::decodeFrame(frame, fl, (uint8_t*)back, sizeof back);
            back[bl] = 0;
            if (bl != tl || memcmp(back, text, tl)) { printf("#%02d DECODE MISMATCH\n", i); bad++; continue; }
        } else { memcpy(back, text, tl + 1); }
        int c = AprsCompressSelfTest::check(back, tl);
        printf("#%02d  plain frame %3zu B -> %s %3zu B   check=%d\n", i, tl + 3, fl ? "compressed" : "plain     ", fl ? fl : tl + 3, c);
        if (c != i) bad++;
    }
    // a corrupted copy must be flagged, not accepted
    char text[300]; size_t tl = AprsCompressSelfTest::expand(2, "N0CALL-7", text, sizeof text);
    text[20] ^= 1;
    if (AprsCompressSelfTest::check(text, tl) != -2) { puts("corruption NOT detected"); bad++; }
    printf("%s\n", bad ? "FAILED" : "all ok");
    return bad != 0;
}
