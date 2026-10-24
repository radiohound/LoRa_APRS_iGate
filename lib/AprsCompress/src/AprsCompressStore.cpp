#include "AprsCompressStore.h"
#include <string.h>

namespace {
    size_t sourceLen(const uint8_t* text, size_t len) {
        size_t n = 0;
        while (n < len && text[n] != '>') n++;
        return n;
    }
}

namespace AprsCompress {

    PacketStore::PacketStore() : tick_(0) { clear(); }

    void PacketStore::clear() {
        for (int i = 0; i < ENTRIES; i++) entries_[i].used = false;
        tick_ = 0;
    }

    PacketStore::Entry* PacketStore::findBySource(const uint8_t* text, size_t len) {
        size_t sl = sourceLen(text, len);
        if (sl == 0 || sl == len) return nullptr;          // no '>' : not a packet we can key
        for (int i = 0; i < ENTRIES; i++) {
            Entry& e = entries_[i];
            if (e.used && sourceLen(e.text, e.len) == sl && memcmp(e.text, text, sl) == 0) return &e;
        }
        return nullptr;
    }

    PacketStore::Entry* PacketStore::findForFrame(uint8_t id, uint8_t refCheck) {
        for (int i = 0; i < ENTRIES; i++) {
            Entry& e = entries_[i];
            if (e.used && stationId(e.text, e.len) == id && crc8(e.text, e.len) == refCheck) {
                e.stamp = ++tick_;
                return &e;
            }
        }
        return nullptr;
    }

    void PacketStore::put(const uint8_t* text, size_t len, bool isFull) {
        if (len == 0 || len > MAX_TEXT) return;
        size_t sl = sourceLen(text, len);
        if (sl == 0 || sl == len) return;
        Entry* e = findBySource(text, len);
        uint8_t since = 0;
        if (e != nullptr) {
            if (!isFull) {                                  // a primed frame does not replace the reference
                if (e->sinceFull < 250) e->sinceFull++;
                e->stamp = ++tick_;
                return;
            }
        } else if (!isFull) {
            return;                                         // nothing to prime from
        } else {
            Entry* oldest = &entries_[0];
            for (int i = 0; i < ENTRIES; i++) {
                if (!entries_[i].used) { oldest = &entries_[i]; break; }
                if (entries_[i].stamp < oldest->stamp) oldest = &entries_[i];
            }
            e = oldest;
            since = 0;
        }
        e->used = true;
        e->len = (uint8_t)len;
        e->sinceFull = since;
        e->stamp = ++tick_;
        memcpy(e->text, text, len);
    }
}
