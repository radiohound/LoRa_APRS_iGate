#include "lacf_capability.h"

#include <cstdio>
#include <string>

namespace LACF_CAP {

    namespace {
        const int BITS[7] = { 3, 5, 3, 2, 2, 6, 5 };
        long absl(long v) { return v < 0 ? -v : v; }
        const char* const ROLE_NAMES[8] = { "transmit only", "listens", "digipeater", "digipeater late", "iGate", "iGate and digipeater", "iGate and digipeater late", "reserved" };
    }

    void pack(const Capability& c, uint8_t out[BYTES]) {
        const unsigned f[7] = { (unsigned)(c.role & 7), (unsigned)(c.model & 31), (unsigned)(c.sf & 7), (unsigned)(c.bw & 3), (unsigned)(c.cr & 3), (unsigned)(c.power & 63), (unsigned)(c.preset & 31) };
        uint32_t v = 0;
        for (int i = 0; i < 7; i++) v = (v << BITS[i]) | f[i];
        v <<= 6;                                                  // six spare bits, sent as 0
        out[0] = (uint8_t)(v >> 24); out[1] = (uint8_t)(v >> 16); out[2] = (uint8_t)(v >> 8); out[3] = (uint8_t)v;
    }

    void parse(const uint8_t in[BYTES], Capability& c) {
        uint32_t v = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | in[3];
        v >>= 6;                                                  // the spare bits are ignored
        uint8_t* f[7] = { &c.role, &c.model, &c.sf, &c.bw, &c.cr, &c.power, &c.preset };
        for (int i = 6; i >= 0; i--) { *f[i] = (uint8_t)(v & ((1u << BITS[i]) - 1)); v >>= BITS[i]; }
    }

    uint8_t sfCode(int sf) { return (uint8_t)(sf < 5 ? 0 : (sf > 12 ? 7 : sf - 5)); }
    uint8_t bwCode(long hz) { if (hz > 500000) return 1; return (uint8_t)(hz <= 62500 ? 0 : (hz <= 125000 ? 1 : (hz <= 250000 ? 2 : 3))); }       // a bandwidth that is not one of the four: 125 kHz
    uint8_t crCode(int d) { return (uint8_t)(d < 5 ? 0 : (d > 8 ? 3 : d - 5)); }
    uint8_t powerCode(int dBm) { return (uint8_t)(dBm < 0 ? 0 : (dBm > 63 ? 63 : dBm)); }
    uint8_t presetFor(long hz) {
        if (absl(hz - 433775000L) <= 2000) return 2;
        if (absl(hz - 439912500L) <= 2000) return 3;
        if (absl(hz - 434855000L) <= 2000) return 4;
        return 0;
    }

    std::string describe(const Capability& c) {
        static const char* const BW[4] = { "62.5", "125", "250", "500" };
        char b[160];
        const std::string model = c.model == MODEL_NONE ? std::string("none (plain only)") : std::to_string((int)c.model - 1);
        std::snprintf(b, sizeof b, "%s, model %s, SF%d BW%s CR4/%d, %d dBm, preset %d", ROLE_NAMES[c.role & 7], model.c_str(), (int)c.sf + 5, BW[c.bw & 3], (int)c.cr + 5, (int)c.power, (int)c.preset);
        return b;
    }

    bool Schedule::due(uint32_t now) const {
        if (!everSent_ || changed_) return true;
        const uint32_t since = now - lastMs_;
        if (since >= SIX_HOURS_MS) return true;
        return sinceLast_ >= EVERY && since >= ONE_HOUR_MS;
    }
    void Schedule::sent(uint32_t now) { everSent_ = true; changed_ = false; lastMs_ = now; sinceLast_ = 0; }
    void Schedule::noteFrame() { if (sinceLast_ < 0xFFFF) sinceLast_++; }

    void Neighbors::update(const std::string& call, const Capability& cap, bool direct, uint32_t now) {
        Entry* slot = nullptr;
        for (Entry& e : e_) if (e.used && e.call == call) { slot = &e; break; }
        if (slot == nullptr) for (Entry& e : e_) if (!e.used) { slot = &e; break; }
        if (slot == nullptr) {                                                    // full: replace the one heard longest ago
            slot = &e_[0];
            for (Entry& e : e_) if (now - e.heardMs > now - slot->heardMs) slot = &e;
        }
        const bool keepDirect = slot->used && slot->call == call && slot->direct && !direct && now - slot->heardMs < DIRECT_KEEP_MS;
        slot->call = call; slot->cap = cap; slot->direct = direct || keepDirect; slot->heardMs = now; slot->used = true;
    }
    const Neighbors::Entry* Neighbors::find(const std::string& call, uint32_t now) const {
        for (const Entry& e : e_) if (e.used && e.call == call) return (now - e.heardMs < EXPIRY_MS) ? &e : nullptr;
        return nullptr;
    }
    size_t Neighbors::count(uint32_t now) const {
        size_t n = 0;
        for (const Entry& e : e_) if (e.used && now - e.heardMs < EXPIRY_MS) n++;
        return n;
    }

}
