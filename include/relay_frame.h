#ifndef RELAY_FRAME_H_
#define RELAY_FRAME_H_

#include <cstdint>
#include <vector>

// The LACF frame a digipeater sends for a packet it relays (spec 11, "Relay"): built when the packet is queued, from the frame that was received, so that it keeps the
// source, destination, position block and content as received and the tuples of the earlier hops, and carries the relay's own tuple. Only the time to hop is still
// open then: it runs from the end of reception to the end of this transmission, so it is filled in just before the frame is sent.
struct RelayFrame {
    std::vector<uint8_t>    frame;              // the frame to send; its own tuple (at ownOffset) holds a placeholder time to hop
    int                     ownOffset = -1;     // offset of the relay's 4-byte tuple in frame, -1 if it has none
    uint32_t                rxDoneMs  = 0;      // millis() when the relayed frame had been received completely
};

#endif
