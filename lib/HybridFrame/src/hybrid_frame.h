// LACF, the LoRa APRS Compact Frame (spec 11 of the APRS-PPM proposal): the native frame 3C FF 02: raw (kinds 0, 1) or PPM-coded (kinds 2, 3) content, each with a CRC-16.
// The frame code (hb.c and the two table files) is the C reference of that spec, unchanged. A relay (relay() below) keeps the tuples of the earlier hops
// and adds its own; a frame that is not relayed carries none.
#ifndef HYBRID_FRAME_H_
#define HYBRID_FRAME_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace HYBRID_FRAME {

    // True for 3C FF 02 (the tested 2.0.0 frame) and for 3C FF 03 (the experimental 3.0.0-dev.1 frame, spec 12; a second decoder, local experiment).
    bool isHybrid(const uint8_t* data, size_t size);

    // TNC2 packet ("SRC>DST,PATH:info", no 3C FF 01 prefix) to an LACF frame. False when the packet cannot be encoded (the caller then sends it as text)
    // or its path holds a non-compliant WIDEn-N (the caller must then DROP the packet, see pathOk()).
    bool encode(const std::string& tnc2, std::vector<uint8_t>& out);

    // TNC2 packet to a 3C FF 03 frame (3.0.0-dev.1, no capability bytes). Same contract as encode().
    bool encode3(const std::string& tnc2, std::vector<uint8_t>& out);

    // The front of a 3C FF 03 frame whatever its model: the structure and the CRC (over the bytes as sent) are checked, the content is not decoded. frontText is
    // "SRC>DST,PATH:" (DST is "?" when its code is not assigned), so a digipeater can apply its own path rule to a frame it cannot decode; relay() then forwards
    // it without the model. key, if given, gets the frame's CRC-16, to recognise a repeat. False for any other frame.
    bool front(const uint8_t* data, size_t size, std::string& frontText, unsigned* key = nullptr);

    // LACF frame to TNC2. False when the receiver rules drop the frame. rxtTuples, if given, gets the number of RXT tuples the frame carried.
    bool decode(const uint8_t* data, size_t size, std::string& tnc2, int* rxtTuples = nullptr);

    // The outgoing LACF frame of a relay (spec 11, "Relay"): the received frame rxFrame with the new path newTnc2 (the digipeater's own rule has already been
    // applied to it, so it holds ownCall + "*" as the element it used), the tuples of the earlier hops kept (moved with their path elements), and the relay's
    // own 4-byte tuple (ownTuple, normally with a placeholder time to hop) added at its path element. The offset of that tuple in out is returned in
    // ownTupleOffset, or -1 when none was added (the relay's element is the eighth or later). False when the frame must not be relayed as LACF: it does not
    // decode, it carries a tuple on the relay's own element or a later one (spec 11, Relay, step 3), the own element is not in the new path, or the new frame
    // cannot be encoded. The caller then falls back to encode() (a frame without RXT) or drops the packet.
    bool relay(const uint8_t* rxFrame, size_t rxLen, const std::string& newTnc2, const std::string& ownCall, const uint8_t ownTuple[4],
               std::vector<uint8_t>& out, int* ownTupleOffset);

    // False if the path holds a WIDEn-N that breaks the APRS rule (n 1 to 7, N 1 to n). Such a packet is dropped and not repeated on RF in any form.
    bool pathOk(const std::string& tnc2);

    // Gives back the 16 KB stack of the worker task until the next call needs it again.
    void shutdown();

}

#endif
