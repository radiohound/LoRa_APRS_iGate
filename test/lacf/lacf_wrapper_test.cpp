// Host test of the LACF wrapper (lib/HybridFrame) against the spec 11 vectors.   sh test/lacf/run.sh
#include "hybrid_frame.h"
#include "vectors.h"
#include "relay_vectors.h"
#include "vectors3.h"
#include "rxt_scale.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

static int fails = 0, checks = 0;
#define EXPECT(name, cond) do { checks++; if (!(cond)) { fails++; printf("FAIL: %s\n", name); } } while (0)

static std::vector<uint8_t> hexBytes(const char* h) {
    std::vector<uint8_t> b;
    for (size_t i = 0; h[i] && h[i + 1]; i += 2) b.push_back((uint8_t)std::stoi(std::string(h + i, 2), nullptr, 16));
    return b;
}
static std::string hexText(const char* h) { auto b = hexBytes(h); return std::string(b.begin(), b.end()); }

int main() {
    for (const HbVec& v : HB_VECTORS) {
        std::string tnc2 = hexText(v.tnc2), back;
        std::vector<uint8_t> frame;
        EXPECT("encode", HYBRID_FRAME::encode(tnc2, frame));
        EXPECT("encode bytes", frame == hexBytes(v.frame));
        EXPECT("decode", HYBRID_FRAME::decode(frame.data(), frame.size(), back) && back == tnc2);
        EXPECT("is hybrid", HYBRID_FRAME::isHybrid(frame.data(), frame.size()));
    }
    for (const HbVec& v : HB_RELAYED) {                                // frames that carry RXT tuples: decoded, tuples counted and dropped
        std::vector<uint8_t> frame = hexBytes(v.frame);
        std::string back; int tuples = 0;
        EXPECT("relayed decode", HYBRID_FRAME::decode(frame.data(), frame.size(), back, &tuples) && back == hexText(v.tnc2));
        EXPECT("relayed carries tuples", tuples >= 1);
    }
    for (const char* h : HB_REJECT) {
        std::vector<uint8_t> frame = hexBytes(h); std::string back;
        EXPECT("reject", !HYBRID_FRAME::decode(frame.data(), frame.size(), back));
    }
    for (const char* h : HB_NOENC) {                                   // packets that must not encode (non-compliant WIDE path, ':' in the source, no hybrid form)
        std::string tnc2 = hexText(h); std::vector<uint8_t> f;
        EXPECT("not encodable", !HYBRID_FRAME::encode(tnc2, f));
    }
    // non-compliant WIDE paths are dropped: not encoded, and pathOk says no
    const char* bad[] = { "N0CALL>APLRG1,WIDE1-2:>x", "N0CALL>APLRG1,WIDE2-3:>x", "N0CALL>APLRG1,WIDE8-1:>x", "N0CALL>APLRG1,WIDE1-0:>x" };
    for (const char* p : bad) { std::vector<uint8_t> f; EXPECT("bad path not encoded", !HYBRID_FRAME::encode(p, f)); EXPECT("bad path", !HYBRID_FRAME::pathOk(p)); }
    const char* good[] = { "N0CALL>APLRG1,WIDE1-1:>x", "N0CALL>APLRG1,WIDE2:>x", "N0CALL>APLRG1,WIDE3-3*:>x", "N0CALL>APLRG1:>x" };
    for (const char* p : good) { std::vector<uint8_t> f; EXPECT("good path encodes", HYBRID_FRAME::encode(p, f)); EXPECT("good path", HYBRID_FRAME::pathOk(p)); }
    uint8_t k1[6] = { 0x3C, 0xFF, 0x02, 0x01, 0, 0 }, k3[6] = { 0x3C, 0xFF, 0x02, 0x03, 0, 0 }, t[6] = { 0x3C, 0xFF, 0x01, 0x02, 0, 0 };
    EXPECT("kind 1 (raw with a position block) is LACF-family", HYBRID_FRAME::isHybrid(k1, 6));
    EXPECT("kind 3 is LACF", HYBRID_FRAME::isHybrid(k3, 6));
    EXPECT("plain is not LACF", !HYBRID_FRAME::isHybrid(t, 6));
    // the relay (spec 11, "Relay"): the received frame with the new path, the earlier hops' tuples kept, the relay's own tuple added
    int relayed = 0;
    for (const RelayVec& v : RELAY_VECTORS) {
        std::vector<uint8_t> rx = hexBytes(v.rx), want = hexBytes(v.relayed), got;
        std::string back; int k = 0;
        EXPECT("relay input decodes", HYBRID_FRAME::decode(rx.data(), rx.size(), back, &k));
        char call[16]; std::snprintf(call, sizeof call, "N0RLY-%d", k + 1);
        const uint8_t tuple[4] = { (uint8_t)(50 + k), (uint8_t)(20 + k), (uint8_t)(45 + k), (uint8_t)(10 + k) };
        int off = -2;
        EXPECT("relay", HYBRID_FRAME::relay(rx.data(), rx.size(), hexText(v.tnc2), call, tuple, got, &off));
        EXPECT("relay bytes equal the reference relay", got == want);
        if (got == want) {
            EXPECT("relay tuple offset", off < 0 || (off + 5 == (int)got.size() && std::equal(tuple, tuple + 4, got.begin() + off)));
            if (off < 0) EXPECT("no own tuple: the frame has no tuple of this relay", !(got.size() > 4 && (got[3] & 0x08) && std::equal(tuple, tuple + 4, got.end() - 5)));
            std::string text; int tuples = 0;
            EXPECT("relayed frame decodes", HYBRID_FRAME::decode(got.data(), got.size(), text, &tuples) && text == hexText(v.tnc2) && tuples >= k);
            relayed++;
        }
    }
    EXPECT("relay vectors ran", relayed == (int)(sizeof RELAY_VECTORS / sizeof RELAY_VECTORS[0]));
    {   // refusals: the new path must hold the relay's own element, and only the path may differ
        std::vector<uint8_t> rx = hexBytes(RELAY_VECTORS[0].rx), out; const uint8_t tuple[4] = { 50, 20, 45, 10 };
        std::string tnc2 = hexText(RELAY_VECTORS[0].tnc2);
        EXPECT("relay refuses a path without the relay's element", !HYBRID_FRAME::relay(rx.data(), rx.size(), tnc2, "N0OTHR-9", tuple, out, nullptr));
        std::string other = tnc2; other[other.size() - 1] ^= 1;                          // the information field changed
        EXPECT("relay refuses a changed information field", !HYBRID_FRAME::relay(rx.data(), rx.size(), other, "N0RLY-1", tuple, out, nullptr));
    }
    {   // filling the tuple: the placeholder time to hop is replaced, the frame stays valid
        std::vector<uint8_t> rx = hexBytes(RELAY_VECTORS[0].rx), out; int off = -1;
        const uint8_t tuple[4] = { RXT_Scale::rssi(-89), RXT_Scale::snr(12.5f), RXT_Scale::frequencyOffset(-3000), 0 };
        EXPECT("relay with measured values", HYBRID_FRAME::relay(rx.data(), rx.size(), hexText(RELAY_VECTORS[0].tnc2), "N0RLY-1", tuple, out, &off) && off >= 0);
        for (unsigned long ms : { 0UL, 1000UL, 5000UL, 600000UL }) {
            std::vector<uint8_t> f = out; f[off + 3] = RXT_Scale::timeToHop(ms, RXT_Scale::tthScale(12, 125000));
            std::string text; int tuples = 0;
            EXPECT("frame with the time to hop filled in decodes", HYBRID_FRAME::decode(f.data(), f.size(), text, &tuples) && tuples == 1);
        }
    }
    // the RXT scales (F4MLV's): the spec's example tuple 32 14 2D 0A is RSSI -80 dBm, SNR -14 dB, 0 Hz, 10 steps of time to hop
    EXPECT("rssi -80", RXT_Scale::rssi(-80) == 0x32);
    EXPECT("snr -14", RXT_Scale::snr(-14.0f) == 0x14);
    EXPECT("frequency offset 0", RXT_Scale::frequencyOffset(0) == 0x2D);
    EXPECT("time to hop 10 steps", RXT_Scale::timeToHop(380, RXT_Scale::tthScale(12, 125000)) == 0x0A);
    EXPECT("rssi clamps", RXT_Scale::rssi(-200) == 0 && RXT_Scale::rssi(0) == 89);
    EXPECT("snr clamps", RXT_Scale::snr(-40.0f) == 0 && RXT_Scale::snr(40.0f) == 88);
    EXPECT("frequency offset clamps", RXT_Scale::frequencyOffset(-9999) == 0 && RXT_Scale::frequencyOffset(9999) == 89);
    EXPECT("frequency offset is symmetric", RXT_Scale::frequencyOffset(1000) + RXT_Scale::frequencyOffset(-1000) == 90);
    EXPECT("time to hop clamps", RXT_Scale::timeToHop(0, 327.68f) == 0 && RXT_Scale::timeToHop(100000000UL, 327.68f) == 89);
    EXPECT("every scale stays within 0 to 89", RXT_Scale::rssi(-100.4f) <= 89 && RXT_Scale::snr(0.3f) <= 88 && RXT_Scale::frequencyOffset(2499) <= 89);
    // ---- 3.0.0-dev.1 (frame type 3C FF 03, spec 12): a second decoder next to the 2.0.0 one
    for (const HbVec& v : HB3_VECTORS) {
        std::string tnc2 = hexText(v.tnc2), back;
        std::vector<uint8_t> frame;
        EXPECT("type03 encode", HYBRID_FRAME::encode3(tnc2, frame));
        EXPECT("type03 encode bytes", frame == hexBytes(v.frame));
        EXPECT("type03 decode", HYBRID_FRAME::decode(frame.data(), frame.size(), back) && back == tnc2);
        EXPECT("type03 is hybrid", HYBRID_FRAME::isHybrid(frame.data(), frame.size()));
    }
    for (const RelayVec& v : RELAY3_VECTORS) {
        std::vector<uint8_t> rx = hexBytes(v.rx), want = hexBytes(v.relayed), got;
        const uint8_t tuple[4] = { 50, 20, 45, 10 };
        int off = -2;
        EXPECT("type03 relay", HYBRID_FRAME::relay(rx.data(), rx.size(), hexText(v.tnc2), "N0RLY-1", tuple, got, &off) && got == want);
    }
    {   // the same bytes with the other type byte are read by neither decoder
        std::vector<uint8_t> f03 = hexBytes(HB3_VECTORS[0].frame), f02 = hexBytes(HB_VECTORS[0].frame), a = f03, b = f02;
        std::string out;
        a[2] = 0x02; b[2] = 0x03;
        EXPECT("type 02 marker on a 03 frame", !HYBRID_FRAME::decode(a.data(), a.size(), out));
        EXPECT("type 03 marker on a 02 frame", !HYBRID_FRAME::decode(b.data(), b.size(), out));
        EXPECT("front() is for type 03 only", !HYBRID_FRAME::front(f02.data(), f02.size(), out));
    }
    // relay without the model, with the digipeater's own path: a frame this station cannot decode is relayed from its front
    for (const PassVec& v : PASS3_VECTORS) {
        std::vector<uint8_t> rx = hexBytes(v.rx), want = hexBytes(v.relayed), got;
        std::string back, front, newFront;
        unsigned key = 0, key2 = 0;
        EXPECT("pass: not decodable here", !HYBRID_FRAME::decode(rx.data(), rx.size(), back));
        EXPECT("pass: front", HYBRID_FRAME::front(rx.data(), rx.size(), front, &key) && front.find('>') != std::string::npos && front.back() == ':');
        EXPECT("pass: front of the relayed frame", HYBRID_FRAME::front(want.data(), want.size(), newFront, &key2));   // the new path: what the digipeater's rule would give
        EXPECT("pass: the CRC is the frame's identity", key == key2);
        const uint8_t tuple[4] = { 50, 20, 45, 10 };
        int off = -2;
        EXPECT("pass: relayed with the caller's path", HYBRID_FRAME::relay(rx.data(), rx.size(), newFront, "N0RLY-1", tuple, got, &off) && got == want);
    }
    for (const char* h : PASS3_DAMAGED) {                                // damaged in the content: the CRC is over the bytes as sent, so even a relay without the model refuses it
        std::vector<uint8_t> rx = hexBytes(h); std::string front;
        EXPECT("pass: a damaged frame has no front", !HYBRID_FRAME::front(rx.data(), rx.size(), front));
    }

    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
