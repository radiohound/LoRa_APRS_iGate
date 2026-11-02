# LACF, the LoRa APRS Compact Frame (experimental)

LACF is the native frame `3C FF 02` with a PPM-coded content field, as written down in spec 11 of the APRS-PPM proposal
(`radiohound/aprs-compression-proposal`, a draft in early development and still being reviewed for change). The frame code in
`lib/HybridFrame` is the C reference of that spec, unchanged; `hybrid_frame.cpp` is the glue.

The same format runs on the T-Deck tracker (branch `hybrid` of the tracker fork) and Dire Wolf (branch `feature/lora-hybrid`).

**Credit.** LACF is developed together with Fabrice Morel, F4MLV (moricef): it combines his native LoRa APRS frame and code with K6ATV's PPM compression, and the two are working on it together (issue 7 of moricef/LoRa_APRS_Tracker). RXT was created by Jon, N7UV. This firmware is a fork of Ricardo Guzman's (CA2RXU) LoRa APRS iGate.

## The setting

On the web page, **Digipeating**, last row: **Tx frame format**, with the same two choices as on the T-Deck:

* **Text (standard)**: what the station sends today.
* **Hybrid RXT Compression (LACF, test only)**: every frame the station sends, beacons, digipeated packets and the rest, goes out as an LACF frame
  when the packet can be encoded, otherwise as text. Other firmware ignores these frames.

Stored as `lora.txFormat` in the config (0 or 3; any other value reads as 0).

## Receiving

LACF frames are always decoded, whatever the setting, and expanded to the text packet the rest of the firmware already handles (so they are gated to
APRS-IS and digipeated like any other). The serial log says `LACF frame: N B on air, M B expanded`, and how many RXT tuples the frame carried. A frame that
fails the receiver rules of the spec is dropped.

## Digipeating (relay)

When the digipeater relays an LACF frame and the Tx frame format is LACF, it follows the relay rule of spec 11: the new frame is built from the received bytes, so the source,
destination, position block and content are copied as received, the tuples of the earlier hops are kept, and the iGate adds its own RXT tuple: the RSSI, SNR and frequency offset
of the reception, and the time to hop, which runs from the end of the reception to the end of this transmission (queue, CAD and backoff included) and is filled in just before the
frame is sent. The digipeater's own path rules (digipeater modes 1 to 3, cross-frequency) decide the new path, as for any packet. A relay adds 5 bytes to the first hop and 4 to each
further one. A frame that would be longer than 255 bytes with its tuple goes out without it. Only a packet that is digipeated gets a tuple: not the iGate's own beacons, not packets
from APRS-IS or MQTT. A standard LoRa APRS frame that is digipeated is sent as before, and with the Tx frame format on Text nothing changes. The serial log shows
`LACF frame: N B instead of M B (relayed, RXT tuple added, time to hop T ms)`.

## Non-compliant WIDE paths

A path element shaped like `WIDEn-N` must follow the APRS rule (n 1 to 7, N 1 to n). A packet whose path holds one that does not (`WIDE1-2`,
`WIDE2-3`) is dropped and **not repeated on RF in any form**, in any Tx frame format. See spec 11, "Non-compliant WIDE paths", for the reasons.

## Cost

The published build for the TTGO LoRa32 V2.1 (commit 88964d7, which still carried the earlier whole-text codec) is 1,804,336 bytes of the 1,966,080-byte app slot (91.8%). Ricardo's `main` is 1,602,737 bytes (81.5%),
so LACF, the MQTT feed and the other additions of this branch cost about 330 KB. That fits because `btStop()` was removed from `wifi_utils.cpp`: nothing here uses
Bluetooth, and the call alone makes the Arduino core link about 128 KB of Bluetooth support (found by F4MLV; measured again on 1 November 2026 on `main`: 1,602,737
bytes (81.5%) became 1,474,561 (75.0%), and RAM 17.2% became 16.0%). The frame code runs in a task of its own with a 16 KB stack, created at the first use.

The earlier whole-text codec (`3C FF 81` and `3C FF C1`) has since been removed, because no station sends it any more. Built from the commit that removes it, the LoRa32 build is 1,664,817 bytes (84.7% of the slot; was 1,797,625, 91.4%) and uses 53,860 bytes of RAM (was 68,588); the ESP32-C3 build is 1,654,510 bytes (84.2%; was 1,782,746) and 46,468 bytes of RAM (was 61,252). That build is not yet the published firmware.

## Tests

Host tests (no board needed):

* `sh test/lacf/run.sh`: the wrapper and the frame code against 40 spec vectors, 35 frames with RXT tuples, 28 rejection vectors, the WIDE rule, a sample of the spec relay vectors (first and later hops, edge cases) and the RXT scales (604 checks).
* `sh test/events/run.sh`: the collector event lines, including the RFONLY/NOGATE privacy rule.
* `sh test/configstore/run.sh`: safe saving of the settings file (123 checks).
* `sh test/mqttdefaults/run.sh`: the pre-filled collector settings (10 cases).

Both `ttgo-lora32-v21` and `ESP32C3_DIY_1W_LoRa_915_E22_900M30S` build.

## Status

Flashed to a LoRa32 V2.1 and used on the air: the iGate sends and receives LACF frames next to ordinary LoRa APRS frames, and the T-Deck tracker and the Dire Wolf
station on the hill (both on the `hybrid` code) decode its frames and it decodes theirs. The browser **Update** on the published firmware page was tested on this board
and keeps the settings. LACF is still an experimental draft in early development, being worked out together with F4MLV; see the firmware page for how to try it.

## Not done

* The relay with RXT was tested on the host (the spec's relay vectors) and run on the air on 2 November 2026 with a T-Deck: the iGate relayed T-Deck beacons of 58 and 25 bytes as 67 and 34 bytes, and decoded a T-Deck relay of its own beacon (29 to 38 bytes, one tuple). Not yet: a frame through two relays, and a packet near the 255-byte limit on the air.
* Frames of kind 0 and 1 (plain text frames of the earlier drafts) are not handled; they are dropped.
* The earlier whole-text compressed frames (`3C FF 81`, `3C FF C1`) are no longer decoded or sent. They are dropped like any other `3C FF` frame of an unknown type, and the collector counts them as `parse_failed`. The `compressTx` and `compressFullEvery` settings are still read and written so that settings files do not change, but they do nothing.
