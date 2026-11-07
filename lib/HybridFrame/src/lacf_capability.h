// The capability bytes of LACF 3.0.0-dev.1 (spec 12, "Capability bytes"): what a station is and what it decodes, four bytes after V when V bit 7 is set.
// role 3 | model decoded 5 | SF 3 | BW 2 | CR 2 | power 6 | preset 5 | spare 6, 32 bits, most significant byte first.
// This module has no hardware dependency: packing, how often to send them, and a small table of what neighbours have announced (host-testable).
#ifndef LACF_CAPABILITY_H_
#define LACF_CAPABILITY_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace LACF_CAP {

    constexpr size_t BYTES = 4;
    enum Role : uint8_t { ROLE_TX_ONLY = 0, ROLE_LISTENS, ROLE_DIGIPEATER, ROLE_DIGIPEATER_LATE, ROLE_IGATE, ROLE_IGATE_DIGIPEATER, ROLE_IGATE_DIGIPEATER_LATE, ROLE_RESERVED };
    constexpr uint8_t MODEL_NONE = 0;             // decodes plain content only
    constexpr uint8_t MODEL_0_HELD = 1;           // the newest model it decodes is model 0 (the field is the model plus one)

    struct Capability {
        uint8_t role = ROLE_LISTENS;
        uint8_t model = MODEL_0_HELD;             // 0 = plain only, else the newest model decoded plus one
        uint8_t sf = 7;                           // 0 to 7 = SF5 to SF12
        uint8_t bw = 1;                           // 0 to 3 = 62.5, 125, 250, 500 kHz
        uint8_t cr = 0;                           // 0 to 3 = 4/5 to 4/8
        uint8_t power = 0;                        // dBm, 0 = unknown, 1 to 63
        uint8_t preset = 0;                       // 0 the channel heard on, 1 = 144.390 AFSK, 2 = 433.775, 3 = 439.9125 (UK), 4 = 434.855 (Poland)
    };

    void pack(const Capability& c, uint8_t out[BYTES]);
    void parse(const uint8_t in[BYTES], Capability& c);

    uint8_t sfCode(int spreadingFactor);          // 5 to 12 -> 0 to 7 (anything else is clamped)
    uint8_t bwCode(long hertz);                   // 62500, 125000, 250000, 500000 -> 0 to 3 (anything else: 125 kHz)
    uint8_t crCode(int denominator);              // the 5 of 4/5 ... 8 -> 0 to 3
    uint8_t powerCode(int dBm);                   // clamped to 0 to 63
    uint8_t presetFor(long hertz);                // the channel presets of spec 12, within 2 kHz; 0 otherwise
    std::string describe(const Capability& c);    // "digipeater, model 0, SF12 BW125 CR4/5, 22 dBm, 433.775 MHz"

    // When to send them (spec 12, Schedule, SHOULD): at start-up and after a change of settings, every 8th transmission but at most once an hour, and at least every 6 hours.
    class Schedule {
    public:
        static constexpr uint32_t ONE_HOUR_MS = 3600UL * 1000UL, SIX_HOURS_MS = 6UL * ONE_HOUR_MS;
        static constexpr uint16_t EVERY = 8;
        bool due(uint32_t nowMs) const;
        void sent(uint32_t nowMs);                // a frame with the bytes went out
        void noteFrame();                         // a frame without them went out
        void settingsChanged() { changed_ = true; }
    private:
        bool everSent_ = false, changed_ = false;
        uint32_t lastMs_ = 0;
        uint16_t sinceLast_ = 0;
    };

    // What neighbours announced: the last bytes of each station heard, and whether they came direct (no used path element) or through a relay (that station may not hear us).
    // An entry expires after about 24 hours. Until a station's bytes have been heard its capabilities are unknown, and unknown means the older model or plain.
    class Neighbors {
    public:
        static constexpr size_t SIZE = 24;
        static constexpr uint32_t EXPIRY_MS = 24UL * 3600UL * 1000UL, DIRECT_KEEP_MS = 3600UL * 1000UL;
        struct Entry { std::string call; Capability cap; bool direct = false; uint32_t heardMs = 0; bool used = false; };
        void update(const std::string& call, const Capability& cap, bool direct, uint32_t nowMs);
        const Entry* find(const std::string& call, uint32_t nowMs) const;
        size_t count(uint32_t nowMs) const;
    private:
        Entry e_[SIZE];
    };

}

#endif
