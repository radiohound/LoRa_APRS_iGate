#ifndef PPM_SELFTEST_H_
#define PPM_SELFTEST_H_

#include <Arduino.h>

// Optional over-the-air check of the compressed-frame code. Only built with -DAPRS_PPM_SELFTEST
// (see tools/aprs_compress/README.md). A normal build contains none of this.
//
//   -DAPRS_PPM_SELFTEST            check every received test packet and keep it away from APRS-IS/digi
//   -DAPRS_PPM_SELFTEST_TX=20      also send one test packet every 20 s (needs lora.txActive)

namespace PPM_SelfTest {
    void begin();
    void loop();
    bool consume(const String& packet);     // true = it was a test packet, handled here
}

#endif
