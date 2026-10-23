#ifdef APRS_PPM_SELFTEST

#include "AprsCompressSelfTest.h"
#include "configuration.h"
#include "ppm_selftest.h"
#include "lora_utils.h"
#include "utils.h"

extern Configuration    Config;

namespace PPM_SelfTest {

    static uint32_t okCount = 0, badCount = 0, compressedCount = 0, plainCount = 0;

    #ifdef APRS_PPM_SELFTEST_TX
        static uint32_t lastTxMs = 0;
        static uint32_t txNext   = 0;
    #endif

    void begin() {
        Utils::println("SELFTEST: compressed-frame self-test active.");
        #ifdef APRS_PPM_SELFTEST_TX
            Utils::println("SELFTEST: sending one test packet every " + String(APRS_PPM_SELFTEST_TX) + " s (first pass compressed, second pass plain).");
        #endif
        Utils::println("SELFTEST: received test packets are checked here and not passed on to APRS-IS, digipeater, KISS or MQTT.");
    }

    void loop() {
        #ifdef APRS_PPM_SELFTEST_TX
            if (!Config.loramodule.txActive) return;
            uint32_t now = millis();
            if (lastTxMs != 0 && now - lastTxMs < (uint32_t)APRS_PPM_SELFTEST_TX * 1000UL) return;
            lastTxMs = now;
            if (lastTxMs == 0) lastTxMs = 1;

            int n = AprsCompressSelfTest::count();
            int idx = txNext % n;
            bool compressed = ((txNext / n) % 2) == 0;
            txNext++;

            char text[300];
            String call = Config.tacticalCallsign == "" ? Config.callsign : Config.tacticalCallsign;
            if (AprsCompressSelfTest::expand(idx, call.c_str(), text, sizeof(text)) == 0) return;
            Utils::println("SELFTEST TX #" + String(idx) + (compressed ? " (compressed)" : " (plain)"));
            LoRa_Utils::setForceCompress(compressed ? 1 : 0);
            LoRa_Utils::sendNewPacket(String(text));
            LoRa_Utils::setForceCompress(-1);
        #endif
    }

    bool consume(const String& packet) {
        if (packet.length() < 8) return false;
        String text = packet.substring(3);                  // drop "<\xff\x01"
        int idx = AprsCompressSelfTest::check(text.c_str(), text.length());
        if (idx == -1) return false;                        // ordinary traffic
        String how = LoRa_Utils::lastRxWasCompressed() ? "compressed" : "plain";
        if (idx >= 0) {
            okCount++;
            if (LoRa_Utils::lastRxWasCompressed()) compressedCount++; else plainCount++;
            Utils::println("SELFTEST RX OK #" + String(idx) + " " + how + ": " + String(LoRa_Utils::lastRxAirBytes()) +
                           " B on air, " + String(text.length() + 3) + " B as plain frame | ok " + String(okCount) +
                           " (compressed " + String(compressedCount) + ", plain " + String(plainCount) + "), bad " + String(badCount));
        } else {
            badCount++;
            Utils::println("SELFTEST RX MISMATCH (" + how + ", " + String(LoRa_Utils::lastRxAirBytes()) + " B on air): " + text);
            Utils::println("SELFTEST totals: ok " + String(okCount) + ", bad " + String(badCount));
        }
        return true;
    }
}

#endif
