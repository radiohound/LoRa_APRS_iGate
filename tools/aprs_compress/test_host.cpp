// Host-side round-trip test and size report for lib/AprsCompress.
//   c++ -std=c++17 -O2 -I lib/AprsCompress/src tools/aprs_compress/test_host.cpp \
//       lib/AprsCompress/src/AprsCompress.cpp lib/AprsCompress/src/AprsCompressModel.cpp -o test_host
//   ./test_host packets.txt        (one TNC2 packet per line)
#include "AprsCompress.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <random>

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s packets.txt\n", argv[0]); return 2; }
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<std::string> pk;
    std::string line;
    while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); if (!line.empty()) pk.push_back(line); }

    size_t n = 0, plain = 0, sent = 0, compressed = 0, bad = 0;
    for (auto& p : pk) {
        uint8_t frame[300], back[300];
        size_t fl = AprsCompress::encodeFrame((const uint8_t*)p.data(), p.size(), frame, sizeof frame);
        n++; plain += 3 + p.size();
        if (fl == 0) { sent += 3 + p.size(); continue; }
        compressed++; sent += fl;
        size_t bl = AprsCompress::decodeFrame(frame, fl, back, sizeof back);
        if (bl != p.size() || memcmp(back, p.data(), bl) != 0) { bad++; fprintf(stderr, "MISMATCH: %s\n", p.c_str()); }
    }
    // random binary / edge cases must round-trip or be refused, never corrupt
    std::mt19937 rng(1);
    size_t fuzz = 0, fuzzBad = 0;
    for (int i = 0; i < 20000; i++) {
        std::string s;
        int mode = rng() % 4;
        if (mode == 3 && !pk.empty()) {         // real packet with a few bytes mutated, sometimes truncated
            s = pk[rng() % pk.size()];
            for (int m = rng() % 4; m > 0; m--) s[rng() % s.size()] = (char)(rng() & 0xFF);
            if (rng() & 1) s.resize(1 + rng() % s.size());
        } else {
            s.assign(1 + rng() % 255, 0);
            for (auto& c : s) c = mode == 0 ? (char)(rng() & 0xFF) : mode == 1 ? (char)(32 + rng() % 95) : (char)"ABC123>,:!/-"[rng() % 12];
        }
        uint8_t frame[300], back[300];
        size_t fl = AprsCompress::encodeFrame((const uint8_t*)s.data(), s.size(), frame, sizeof frame);
        if (!fl) continue;
        fuzz++;
        size_t bl = AprsCompress::decodeFrame(frame, fl, back, sizeof back);
        if (bl != s.size() || memcmp(back, s.data(), bl) != 0) fuzzBad++;
    }
    printf("packets %zu, compressed %zu, round-trip failures %zu\n", n, compressed, bad);
    printf("mean plain frame %.1f B -> sent %.1f B (%.0f%% saved)\n", (double)plain / n, (double)sent / n, 100.0 * (1 - (double)sent / plain));
    printf("fuzz: %zu compressed, %zu failures\n", fuzz, fuzzBad);
    return (bad || fuzzBad) ? 1 : 0;
}
