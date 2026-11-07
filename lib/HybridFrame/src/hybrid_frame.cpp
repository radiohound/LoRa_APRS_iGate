#include "hybrid_frame.h"

#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "hb.h"
#include "hb3.h"
}
#include "set1.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#define HYBRID_FRAME_WORKER 1
#endif

namespace HYBRID_FRAME {

    namespace {

        // The frame code keeps its working buffers on the stack (about 10 KB at the deepest, measured on an ESP32), more than the 8 KB of the Arduino
        // loop task. On the board the calls therefore run in a task of their own with a 16 KB stack, one call at a time.
        enum Op { OP_ENCODE, OP_DECODE, OP_RELAY, OP_ENCODE3, OP_FRONT };
        struct Job {
            Op op;
            const char* tnc2; size_t tnc2Len;
            const uint8_t* frame; size_t frameLen;
            uint8_t* out; size_t outCap;
            char* text; size_t textCap;
            hb_info_t info;
            int rxtCount;                                            // OP_DECODE: the RXT tuples of the frame, either type
            unsigned key;                                            // OP_FRONT: the frame's CRC-16
            const uint8_t* cap; uint8_t capOut[4]; bool hasCap;      // OP_ENCODE3: the capability bytes to send; OP_DECODE: those the frame carried
            long result;
            const char* own; size_t ownLen; uint8_t ownTuple[4]; int ownOffset;      // OP_RELAY: the relay's callsign, its tuple, and where its tuple ended up
        };

        // The elements of the path of "SRC>DST,E1,E2:info" (after the destination, up to the first ':').
        std::vector<std::string> pathElements(const std::string& t) {
            std::vector<std::string> v;
            size_t colon = t.find(':'), gt = t.find('>');
            if (colon == std::string::npos || gt == std::string::npos || gt > colon) return v;
            size_t comma = t.find(',', gt);
            if (comma == std::string::npos || comma > colon) return v;
            size_t p = comma + 1;
            while (p <= colon) {
                size_t q = t.find_first_of(",:", p);
                v.push_back(t.substr(p, q - p));
                if (t[q] == ':') break;
                p = q + 1;
            }
            return v;
        }

        // OP_RELAY, see relay() in the header: the reference relay with the caller's path (hb_relay_to), then where the relay's own tuple is.
        long relayRun(Job& j) {
            std::string own(j.own, j.ownLen);
            bool type3 = j.frame[2] == 0x03;
            size_t n = type3 ? hb3_relay_to(j.frame, j.frameLen, j.tnc2, j.tnc2Len, own.c_str(), j.ownTuple, j.out, j.outCap)
                             : hb_relay_to(j.frame, j.frameLen, j.tnc2, j.tnc2Len, own.c_str(), j.ownTuple, j.out, j.outCap);
            // A type 03 frame whose model (or destination code) the relay does not hold is forwarded without the model: the structure and the CRC are checked, the content is not decoded,
            // and the path is the one the digipeater's own rule gave (it was applied to the text front() produced).
            if (n == 0 && type3) n = hb3_relay_to_passthrough(j.frame, j.frameLen, j.tnc2, j.tnc2Len, own.c_str(), j.ownTuple, j.out, j.outCap);
            if (n == 0) return -1;
            std::vector<std::string> path = pathElements(std::string(j.tnc2, j.tnc2Len));
            int ownIdx = -1;
            for (size_t i = 0; i < path.size(); i++) if (path[i] == own + "*") { ownIdx = (int)i; break; }
            // The block ends with the tuples, then the mask byte; the relay's tuple is the last one, if it is there: the reference leaves it out of a frame that would be
            // longer than 255 bytes with it, and a relay at the eighth element or later has none. Its mask bit says.
            bool hasOwn = ownIdx >= 0 && ownIdx <= 6 && (j.out[3] & 0x08) && (j.out[n - 1] & (1u << ownIdx));
            j.ownOffset = hasOwn ? (int)n - 5 : -1;
            return (long)n;
        }

        void run(Job& j) {
            static bool listSet = false;
            if (!listSet) { hb_set_list(SET1, SET1_N); hb3_set_list(SET1, SET1_N); listSet = true; }
            if (j.op == OP_ENCODE) j.result = (long)hb_encode(j.tnc2, j.tnc2Len, nullptr, j.out, j.outCap, &j.info);
            else if (j.op == OP_ENCODE3) { hb3_info_t i3; j.result = (long)hb3_encode_cap(j.tnc2, j.tnc2Len, nullptr, j.cap, j.out, j.outCap, &i3); }
            else if (j.op == OP_RELAY) j.result = relayRun(j);
            else if (j.op == OP_FRONT) {
                hb3_info_t i3;
                j.result = (long)hb3_decode_front(j.frame, j.frameLen, j.text, j.textCap, &i3);
                if (j.result > 0) {                                                       // the CRC-16 bytes: after V, the capability bytes if any, and L (LACF content only)
                    size_t cs = i3.content_start, off = cs + 1 + ((j.frame[cs] & 0x80) ? 4 : 0) + (((j.frame[3] & 3) < 2) ? 0 : 1);
                    j.key = (unsigned)(j.frame[off] | (j.frame[off + 1] << 8));
                }
            }
            else if (j.frame[2] == 0x03) { hb3_info_t i3; j.result = (long)hb3_decode(j.frame, j.frameLen, j.text, j.textCap, &i3); j.rxtCount = i3.has_rxt ? i3.rxt.n : 0; j.hasCap = i3.has_cap != 0; std::memcpy(j.capOut, i3.cap, 4); }
            else { j.result = (long)hb_decode(j.frame, j.frameLen, j.text, j.textCap, &j.info); j.rxtCount = j.info.has_rxt ? j.info.rxt.n : 0; }
        }

#ifdef HYBRID_FRAME_WORKER
        SemaphoreHandle_t lockMutex = nullptr, startSem = nullptr, doneSem = nullptr;
        TaskHandle_t workerHandle = nullptr;
        Job shared;

        void workerTask(void*) {
            for (;;) {
                xSemaphoreTake(startSem, portMAX_DELAY);
                run(shared);
                xSemaphoreGive(doneSem);
            }
        }

        bool call(Job& j) {
            if (lockMutex == nullptr) {                       // first use
                lockMutex = xSemaphoreCreateMutex();
                startSem = xSemaphoreCreateBinary();
                doneSem = xSemaphoreCreateBinary();
                if (lockMutex == nullptr || startSem == nullptr || doneSem == nullptr) {
                    lockMutex = nullptr;
                    return false;
                }
            }
            xSemaphoreTake(lockMutex, portMAX_DELAY);
            if (workerHandle == nullptr && xTaskCreate(workerTask, "lacf", 16384, nullptr, 1, &workerHandle) != pdPASS) {
                workerHandle = nullptr;
                xSemaphoreGive(lockMutex);
                return false;
            }
            shared = j;
            xSemaphoreGive(startSem);
            xSemaphoreTake(doneSem, portMAX_DELAY);
            j = shared;
            xSemaphoreGive(lockMutex);
            return true;
        }
#else
        bool call(Job& j) { run(j); return true; }
#endif

    }

    void shutdown() {
#ifdef HYBRID_FRAME_WORKER
        if (lockMutex == nullptr) return;
        xSemaphoreTake(lockMutex, portMAX_DELAY);
        if (workerHandle != nullptr) { vTaskDelete(workerHandle); workerHandle = nullptr; }
        xSemaphoreGive(lockMutex);
#endif
    }

    bool isHybrid(const uint8_t* data, size_t size) {
        return size >= 5 && data[0] == 0x3C && data[1] == 0xFF && (data[2] == 0x02 || data[2] == 0x03);   // all four kinds: LACF (2, 3) and raw (0, 1); the receiver checks the rest
    }

    bool pathOk(const std::string& tnc2) {
        return hb_path_ok(tnc2.data(), tnc2.size()) != 0;
    }

    bool encode(const std::string& tnc2, std::vector<uint8_t>& out) {
        Job j;
        std::memset(&j, 0, sizeof j);
        uint8_t frame[256];
        j.op = OP_ENCODE; j.tnc2 = tnc2.data(); j.tnc2Len = tnc2.size(); j.out = frame; j.outCap = sizeof frame;
        if (!call(j) || j.result <= 0 || j.result > 255) return false;
        out.assign(frame, frame + j.result);
        return true;
    }

    bool encode3(const std::string& tnc2, std::vector<uint8_t>& out, const uint8_t* cap) {
        Job j;
        std::memset(&j, 0, sizeof j);
        uint8_t frame[256];
        j.op = OP_ENCODE3; j.tnc2 = tnc2.data(); j.tnc2Len = tnc2.size(); j.cap = cap; j.out = frame; j.outCap = sizeof frame;
        if (!call(j) || j.result <= 0 || j.result > 255) return false;
        out.assign(frame, frame + j.result);
        return true;
    }

    bool front(const uint8_t* data, size_t size, std::string& frontText, unsigned* key) {
        if (size < 5 || data[0] != 0x3C || data[1] != 0xFF || data[2] != 0x03) return false;
        Job j;
        std::memset(&j, 0, sizeof j);
        std::vector<char> text(700);
        j.op = OP_FRONT; j.frame = data; j.frameLen = size; j.text = text.data(); j.textCap = text.size();
        if (!call(j) || j.result <= 0) return false;
        frontText.assign(text.data(), (size_t)j.result);
        if (key) *key = j.key;
        return true;
    }

    bool relay(const uint8_t* rxFrame, size_t rxLen, const std::string& newTnc2, const std::string& ownCall, const uint8_t ownTuple[4],
               std::vector<uint8_t>& out, int* ownTupleOffset) {
        if (ownTupleOffset) *ownTupleOffset = -1;
        Job j;
        std::memset(&j, 0, sizeof j);
        uint8_t frame[256];
        j.op = OP_RELAY; j.frame = rxFrame; j.frameLen = rxLen; j.tnc2 = newTnc2.data(); j.tnc2Len = newTnc2.size();
        j.own = ownCall.data(); j.ownLen = ownCall.size(); std::memcpy(j.ownTuple, ownTuple, 4);
        j.out = frame; j.outCap = sizeof frame; j.ownOffset = -1;
        if (!call(j) || j.result <= 0 || j.result > 255) return false;
        out.assign(frame, frame + j.result);
        if (ownTupleOffset) *ownTupleOffset = j.ownOffset;
        return true;
    }

    bool decode(const uint8_t* data, size_t size, std::string& tnc2, int* rxtTuples, uint8_t* capOut, bool* hasCap) {
        Job j;
        std::memset(&j, 0, sizeof j);
        std::vector<char> text(700);
        j.op = OP_DECODE; j.frame = data; j.frameLen = size; j.text = text.data(); j.textCap = text.size();
        if (!call(j) || j.result <= 0) return false;
        tnc2.assign(text.data(), (size_t)j.result);
        if (rxtTuples) *rxtTuples = j.rxtCount;
        if (hasCap) *hasCap = j.hasCap;
        if (capOut && j.hasCap) std::memcpy(capOut, j.capOut, 4);
        return true;
    }

}
