// The four fields of an RXT tuple (spec 11, "RXT block"), from a measurement to the tuple byte (0 to 89, the RXT character minus 33).
// The scales are F4MLV's (moricef/LoRa_APRS_iGate, branch rxt, src/lora_utils.cpp): RSSI -130 dBm in 1 dB steps, SNR -24 dB in 0.5 dB steps,
// frequency offset +-2500 Hz on a quadratic scale around index 45, time to hop exponential (base 1.08) in units of ten symbol times.
#ifndef RXT_SCALE_H_
#define RXT_SCALE_H_

#include <cmath>
#include <cstdint>

namespace RXT_Scale {

    constexpr int   MAX_INDEX  = 89;
    constexpr float RSSI_MIN   = -130.0f;
    constexpr float SNR_MIN    = -24.0f;
    constexpr float SNR_MAX    = 20.0f;
    constexpr float SNR_RES    = 0.5f;
    constexpr float FO_MAX_HZ  = 2500.0f;
    constexpr int   FO_CENTER  = 45;
    constexpr float TTH_BASE   = 1.08f;
    constexpr float TTH_FACTOR = 10.0f;

    inline int clampIndex(int v) { return v < 0 ? 0 : (v > MAX_INDEX ? MAX_INDEX : v); }

    inline uint8_t rssi(float dBm) {
        return (uint8_t)clampIndex((int)std::round(dBm - RSSI_MIN));
    }

    inline uint8_t snr(float dB) {
        float c = dB < SNR_MIN ? SNR_MIN : (dB > SNR_MAX ? SNR_MAX : dB);
        return (uint8_t)clampIndex((int)std::round((c - SNR_MIN) / SNR_RES));
    }

    inline uint8_t frequencyOffset(int hz) {
        float c = (float)hz;
        if (c < -FO_MAX_HZ) c = -FO_MAX_HZ;
        if (c > FO_MAX_HZ) c = FO_MAX_HZ;
        float n = c / FO_MAX_HZ;
        float u = (n >= 0 ? 1.0f : -1.0f) * std::sqrt(std::fabs(n));
        return (uint8_t)clampIndex((int)std::round(u * FO_CENTER + FO_CENTER));
    }

    // The unit of the time-to-hop scale, in ms: ten symbol times of the active modem.
    inline float tthScale(int spreadingFactor, long bandwidthHz) {
        return (float)(1UL << spreadingFactor) / ((float)bandwidthHz / 1000.0f) * TTH_FACTOR;
    }

    inline uint8_t timeToHop(unsigned long ms, float scale) {
        float target = (float)ms / scale + 1.0f;
        if (target < 1.0f) target = 1.0f;
        return (uint8_t)clampIndex((int)std::round(std::log(target) / std::log(TTH_BASE)));
    }

}

#endif
