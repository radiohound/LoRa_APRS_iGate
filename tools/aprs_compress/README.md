# APRS-PPM: order-3 context compression for LoRa APRS packets (experimental)

Not a new idea: this is the prediction-by-partial-matching (PPM) family of text compressors
(order-3 context, PPM "method C" escape weights) feeding a range coder, with the context
statistics **trained ahead of time on real APRS packets and shipped as a fixed table**. A fixed
table is what makes it work on 40-100 byte packets, where adaptive compressors (zlib, LZMA)
have nothing to learn from and expand the data.

## On-air format

    0x3C 0xFF (0x80 | model version)  <length>  <range-coded bytes>

* `<length>` is the length of the original text (1..255), so no end-of-message symbol is needed.
* Plain LoRa APRS stays `0x3C 0xFF 0x01`. Firmware that does not know this format ignores the
  frame (it checks for exactly `0x3C 0xFF 0x01`), so it is safe to leave on in a mixed network:
  those stations just do not hear compressed senders.
* Model v1 is `0x81`. Sender and receiver must hold the **identical** table; any retrain gets a new
  version number.
* The coder is integer-only (no floats), so every device decodes exactly what was encoded.
* A frame is only sent compressed when it is smaller than the plain frame.

## In this firmware

* Receive: always on. A compressed frame is expanded to the plain `<\xff\x01` + text form right
  after the radio read, so APRS-IS gating, digipeating, KISS, MQTT and the map all see ordinary text.
* Transmit: off by default. Set `"compressTx": true` in the `lora` block of `igate_conf.json`.
  (No web-UI checkbox yet: the UI is shipped as pre-compressed assets.)
* Flash cost: about 130 KB for the model tables. RAM: about 2 KB of stack while coding.

## Measured (host test, held-out stations the model never saw)

Training: real LoRa APRS-IS packets plus RF-heard VHF APRS packets (14,186 unique), trained on
70% of stations and tested on the other 30% (4,514 packets). Mean frame including the 3-byte
header; every packet round-trips bit-exactly, and ~4,700 mutated/truncated real packets per run
were fuzzed with no failures. `--min-count` prunes rare contexts: the table size/ratio dial.

| `--min-count` | Table size (70% data) | Saved |
|---|---|---|
| 4 | 179 KB | 52% |
| 6 | 121 KB | 50% |
| 8 | 94 KB | 48% |
| 12 | 64 KB | 45% |
| 20 | 39 KB | 42% |

The shipped table (v1) is trained on all data at min count 8: 125 KB of flash, so expect about
48-50% on stations it has not seen (the in-sample figure is higher and not representative).
Whole-firmware flash on the ttgo-lora32-v21 build goes from 81.5% to 88.1%.

## Retrain

    python3 tools/aprs_compress/train_model.py --lora lora.txt --vhf vhf.txt \
        --out lib/AprsCompress/src/AprsCompressModel.cpp --version 2

Inputs are raw APRS-IS text, one packet per line. For an honest ratio use `--holdout 0.3 --test-out test.txt`
and run the host test on `test.txt` (the shipped table is trained on everything).

## Host test (no hardware needed)

    c++ -std=c++17 -O2 -I lib/AprsCompress/src tools/aprs_compress/test_host.cpp \
        lib/AprsCompress/src/AprsCompress.cpp lib/AprsCompress/src/AprsCompressModel.cpp -o test_host
    ./test_host test.txt

## Not yet tested

Over-the-air operation on real hardware: the firmware builds for `ttgo-lora32-v21`, and the codec
is round-trip tested on a PC, but no two radios have exchanged a compressed frame yet.
