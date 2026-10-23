#ifndef APRS_COMPRESS_SELFTEST_H_
#define APRS_COMPRESS_SELFTEST_H_

#include <stddef.h>

// Fixed list of test packets for over-the-air checks of AprsCompress. Plain C++, no Arduino,
// so the same list is checked on a PC (tools/aprs_compress/test_selftest_host.cpp) and in firmware.
//
// Every packet ends with "~NN" (its index), which lets a receiver recognise a test packet and
// rebuild exactly what the sender meant to send, using the sender's callsign from the packet.

namespace AprsCompressSelfTest {

    int     count();

    // Writes template idx with the callsign filled in. Returns the length, or 0 if it does not fit.
    size_t  expand(int idx, const char* callsign, char* out, size_t cap);

    // If text is a test packet that was built by expand(), returns its index; otherwise -1.
    // Returns -2 if it looks like a test packet (ends in "~NN") but differs from what expand()
    // makes for that index and sender, i.e. it was corrupted or decoded wrongly.
    int     check(const char* text, size_t len);
}

#endif
