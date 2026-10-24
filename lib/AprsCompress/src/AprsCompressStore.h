#ifndef APRS_COMPRESS_STORE_H_
#define APRS_COMPRESS_STORE_H_

#include <stddef.h>
#include <stdint.h>
#include "AprsCompress.h"

// The last full or plain packet text seen (or sent) for each source callsign: the reference for primed
// frames. A primed frame never replaces it, so losing a primed frame does not break the next one; only
// losing the full frame does. A fixed number of entries; when full, the least recently used is replaced.
//
// Receiver: put(..., true) for every plain or full packet received without error; put(..., false) for a
//           primed one (it only counts); look up the reference for a primed frame with findForFrame().
// Sender:   put(..., true) for every full or plain packet transmitted, put(..., false) for a primed one;
//           findBySource() gives the reference for the next packet.

namespace AprsCompress {

    class PacketStore {
    public:
        static constexpr int ENTRIES = 24;

        struct Entry {
            bool     used;
            uint8_t  len;
            uint8_t  sinceFull;       // primed frames since the last full or plain one
            uint32_t stamp;           // for least-recently-used replacement
            uint8_t  text[MAX_TEXT];
        };

        PacketStore();

        // Remember this packet text. isFull = it went out (or came in) as a full or plain frame.
        void   put(const uint8_t* text, size_t len, bool isFull);

        // The entry whose source callsign (text before the first '>') equals this text's.
        Entry* findBySource(const uint8_t* text, size_t len);

        // The entry a primed frame refers to: station id and reference check both match.
        Entry* findForFrame(uint8_t id, uint8_t refCheck);

        void   clear();

    private:
        Entry    entries_[ENTRIES];
        uint32_t tick_;
    };
}

#endif
