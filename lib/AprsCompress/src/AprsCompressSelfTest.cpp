#include "AprsCompressSelfTest.h"
#include <string.h>

namespace {

    // %C = sender callsign, %N = two-digit index. Fabricated positions; keep APRS-IS gating off.
    // No message numbers ("{nn"), so a test message never asks anyone for an ack.
    const char* const TEMPLATES[] = {
        "%C>APZPPM,WIDE1-1:!3400.00N/11800.00W>PPM test~%N",
        "%C>APZPPM:!3400.00N/11800.00W>~%N",
        "%C>APZPPM,WIDE1-1,WIDE2-1:=3412.34N/11823.45W[Hiking, long comment about the trail conditions and battery 3.9V~%N",
        "%C>APZPPM,WIDE1-1:!/5L!!<*e7>7PCompressed position with a comment~%N",
        "%C>APZPPM:!/5L!!<*e7>7P~%N",
        "%C>APZPPM,TCPIP*:@092345z3400.00N/11800.00W_090/005g010t068r000p000h50b10132~%N",
        "%C>APZPPM,WIDE1-1:>Status: on the air, SF12 BW125~%N",
        "%C>APZPPM:;PPM-OBJ  *092345z3400.00N/11800.00W]Test object~%N",
        "%C>APZPPM,WIDE1-1::TESTMSG  :Hello from the PPM self-test~%N",
        "%C>APZPPM:T#123,100,200,050,010,000,00000000~%N",
        "%C>S32U6T,WIDE1-1:`c5Zl\x1c>/\"4}=\xe9\xf1~%N",                      // Mic-E-like, non-ASCII bytes
        "%C>APZPPM,WIDE1-1:!3355.10N/11755.20WhBalloon payload 12.3km 45.2C 5.1V~%N",
        "%C>APZPPM,WIDE1-1:!3400.00N/11800.00W>"
            "Long comment: close to the largest packet the format carries, to check the length handling "
            "and the range coder on a long run of ordinary text. 0123456789 0123456789 0123456789~%N",
        "%C>APZPPM:>q7Zk#Xw9@Lm2$Rt5&Vb8*Hn4^Cd1!Jp6Ys3Fg0Qz7Mx2Nc5Bv8Lk4Hj1~%N",    // high entropy: may stay plain
    };
    const int N = (int)(sizeof(TEMPLATES) / sizeof(TEMPLATES[0]));

    int twoDigits(const char* p) {
        if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9') return -1;
        return (p[0] - '0') * 10 + (p[1] - '0');
    }
}

namespace AprsCompressSelfTest {

    int count() { return N; }

    size_t expand(int idx, const char* callsign, char* out, size_t cap) {
        if (idx < 0 || idx >= N || cap == 0) return 0;
        size_t n = 0;
        const char* t = TEMPLATES[idx];
        for (size_t i = 0; t[i]; i++) {
            const char* piece = nullptr;
            char num[3] = { (char)('0' + idx / 10), (char)('0' + idx % 10), 0 };
            if (t[i] == '%' && t[i + 1] == 'C') { piece = callsign; i++; }
            else if (t[i] == '%' && t[i + 1] == 'N') { piece = num; i++; }
            if (piece) {
                size_t l = strlen(piece);
                if (n + l >= cap) return 0;
                memcpy(out + n, piece, l); n += l;
            } else {
                if (n + 1 >= cap) return 0;
                out[n++] = t[i];
            }
        }
        out[n] = 0;
        return n;
    }

    int check(const char* text, size_t len) {
        if (len < 4 || text[len - 3] != '~') return -1;
        int idx = twoDigits(text + len - 2);
        if (idx < 0 || idx >= N) return -1;
        const char* gt = (const char*)memchr(text, '>', len);
        if (!gt || gt - text > 9) return -2;
        char call[16];
        size_t cl = (size_t)(gt - text);
        memcpy(call, text, cl); call[cl] = 0;
        char want[300];
        size_t wl = expand(idx, call, want, sizeof want);
        if (wl != len || memcmp(want, text, len) != 0) return -2;
        return idx;
    }
}
