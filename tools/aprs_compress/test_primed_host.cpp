// Host test for primed frames: checks the firmware codec against the reference vectors
// (lora-aprs-ideas/test/vectors_primed_v1.txt), then runs a sender/receiver packet-store simulation
// with a lost frame.
//   c++ -std=c++17 -O2 -I lib/AprsCompress/src tools/aprs_compress/test_primed_host.cpp \
//       lib/AprsCompress/src/AprsCompress.cpp lib/AprsCompress/src/AprsCompressModel.cpp \
//       lib/AprsCompress/src/AprsCompressStore.cpp -o test_primed_host && ./test_primed_host <vectors file>
#include "AprsCompress.h"
#include "AprsCompressStore.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

static bool unhex(const std::string& s, std::vector<uint8_t>& out) {
    out.clear();
    for (size_t i = 0; i + 1 < s.size(); i += 2) out.push_back((uint8_t)std::stoi(s.substr(i, 2), nullptr, 16));
    return true;
}

// What LoRa_Utils::transmitText does, without the radio.
static size_t makeFrame(AprsCompress::PacketStore& tx, int fullEvery, const std::string& text, uint8_t* out, size_t, const char** kind) {
    const uint8_t* t = (const uint8_t*)text.data(); size_t tl = text.size();
    uint8_t full[256], primed[256];
    size_t nf = AprsCompress::encodeFrame(t, tl, full, sizeof full), np = 0;
    AprsCompress::PacketStore::Entry* ref = tx.findBySource(t, tl);
    if (ref && fullEvery > 1 && (int)ref->sinceFull + 1 < fullEvery)
        np = AprsCompress::encodePrimed(ref->text, ref->len, t, tl, primed, sizeof primed);
    if (np > 0 && (nf == 0 || np < nf)) { tx.put(t, tl, false); memcpy(out, primed, np); *kind = "primed"; return np; }
    tx.put(t, tl, true);
    if (nf > 0) { memcpy(out, full, nf); *kind = "full"; return nf; }
    out[0] = 0x3C; out[1] = 0xFF; out[2] = 0x01; memcpy(out + 3, t, tl); *kind = "plain"; return tl + 3;
}

// What LoRa_Utils::readFrame does for the three frame types.
static bool takeFrame(AprsCompress::PacketStore& rx, const uint8_t* f, size_t n, std::string& text) {
    uint8_t buf[300];
    if (AprsCompress::isCompressedHeader(f, n)) {
        size_t k = AprsCompress::decodeFrame(f, n, buf, 255);
        if (!k) return false;
        rx.put(buf, k, true); text.assign((char*)buf, k); return true;
    }
    if (AprsCompress::isPrimedHeader(f, n)) {
        uint8_t id, chk; AprsCompress::primedHeader(f, n, id, chk);
        AprsCompress::PacketStore::Entry* r = rx.findForFrame(id, chk);
        if (!r) return false;
        size_t k = AprsCompress::decodePrimed(r->text, r->len, f, n, buf, 255);
        if (!k) return false;
        rx.put(buf, k, false); text.assign((char*)buf, k); return true;
    }
    if (n > 3 && f[0] == 0x3C && f[1] == 0xFF && f[2] == 0x01) { rx.put(f + 3, n - 3, true); text.assign((const char*)f + 3, n - 3); return true; }
    return false;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <vectors_primed_v1.txt>\n", argv[0]); return 2; }
    int bad = 0, n = 0, rejected = 0;
    std::ifstream in(argv[1]); std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line); std::string rh, fh, th; ss >> rh >> fh >> th;
        std::vector<uint8_t> ref, frame, text, enc(400), back(400);
        unhex(rh, ref); unhex(th, text); if (fh != "-") unhex(fh, frame);
        size_t el = AprsCompress::encodePrimed(ref.data(), ref.size(), text.data(), text.size(), enc.data(), enc.size());
        n++;
        if (el != frame.size() || (el && memcmp(enc.data(), frame.data(), el))) { printf("ENCODE MISMATCH vector %d\n", n); bad++; continue; }
        if (el) {
            size_t bl = AprsCompress::decodePrimed(ref.data(), ref.size(), frame.data(), frame.size(), back.data(), back.size());
            if (bl != text.size() || memcmp(back.data(), text.data(), bl)) { printf("DECODE MISMATCH vector %d\n", n); bad++; continue; }
            ref[ref.size() / 2] ^= 1;
            if (AprsCompress::decodePrimed(ref.data(), ref.size(), frame.data(), frame.size(), back.data(), back.size()) != 0) { printf("WRONG REFERENCE ACCEPTED vector %d\n", n); bad++; } else rejected++;
        }
    }
    printf("reference vectors: %d, failures %d, wrong-reference frames rejected %d\n", n, bad, rejected);

    // Sender/receiver simulation: two stations interleaved, full frame every 3rd; once with a full frame
    // lost and once with a primed frame lost.
    const char* seq[] = {
        "TEST-1>APLRT1,WIDE1-1:!3400.00N/11800.00W>LoRa Tracker 5.2V",
        "TEST-2>APLRG1:!3412.34N/11823.45W-LoRa iGate 433.775 MHz",
        "TEST-1>APLRT1,WIDE1-1:!3400.10N/11800.05W>LoRa Tracker 5.2V",
        "TEST-2>APLRG1:!3412.34N/11823.45W-LoRa iGate 433.775 MHz",
        "TEST-1>APLRT1,WIDE1-1:!3400.20N/11800.10W>LoRa Tracker 5.1V",
        "TEST-1>APLRT1,WIDE1-1:!3400.30N/11800.15W>LoRa Tracker 5.1V",
        "TEST-1>APLRT1,WIDE1-1:!3400.40N/11800.20W>LoRa Tracker 5.0V",
        "TEST-1>APLRT1,WIDE1-1:!3400.50N/11800.25W>LoRa Tracker 5.0V",
        "TEST-2>APLRG1:!3412.34N/11823.45W-LoRa iGate 433.775 MHz",
        "TEST-1>APLRT1,WIDE1-1:!3400.60N/11800.30W>LoRa Tracker 4.9V",
    };
    int seqBad = 0;
    const int lostCases[2] = { 5, 2 };
    const char* lostName[2] = { "a full frame (#5)", "a primed frame (#2)" };
    for (int c = 0; c < 2; c++) {
        AprsCompress::PacketStore tx, rx;
        int dropped = 0, primed = 0, full = 0;
        size_t bytes = 0, plainBytes = 0;
        printf("simulation, losing %s:\n", lostName[c]);
        for (int i = 0; i < (int)(sizeof seq / sizeof *seq); i++) {
            uint8_t frame[256]; const char* kind = "";
            size_t fl = makeFrame(tx, 3, seq[i], frame, sizeof frame, &kind);
            bytes += fl; plainBytes += strlen(seq[i]) + 3;
            if (!strcmp(kind, "primed")) primed++; else full++;
            std::string got;
            if (i == lostCases[c]) { printf("  #%d %-6s %3zu B  (lost on the air)\n", i, kind, fl); continue; }
            bool ok = takeFrame(rx, frame, fl, got);
            printf("  #%d %-6s %3zu B  %s\n", i, kind, fl, ok ? (got == seq[i] ? "decoded" : "WRONG TEXT") : "dropped (no reference)");
            if (ok && got != seq[i]) seqBad++;
            if (!ok) dropped++;
        }
        printf("  %d primed, %d full/plain, %zu B sent against %zu B plain, wrong texts %d, dropped %d\n",
               primed, full, bytes, plainBytes, seqBad, dropped);
    }
    return (bad || seqBad) ? 1 : 0;
}
