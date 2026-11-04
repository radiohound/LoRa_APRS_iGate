# LACF, the LoRa APRS Compact Frame (experimental)

> **In early development and still being reviewed for change. Not fully tested.** The frame format may change; do not build deployed software from it.

This is a branch of a **fork** of [richonguzman/LoRa_APRS_iGate](https://github.com/richonguzman/LoRa_APRS_iGate)
(the original project and its own README are there). It is an experiment, **not a pull request** and not part of that project.
It adds an optional frame format that carries the same APRS text in fewer bytes, plus an optional event feed to a collector.
Pull requests against the original project are opened separately, only for individual fixes and only by the fork's owner.

## What LACF is

LACF is the native LoRa APRS frame `3C FF 02` with a PPM-coded content field and a CRC-16. It is developed together with
Fabrice Morel, F4MLV (moricef): a hybrid of his native LoRa APRS frame and code and K6ATV's PPM compression. The RXT block that relays
may add was created by Jon, N7UV. This firmware is built on Ricardo Guzman's (CA2RXU) iGate.

* [How this firmware uses it: `docs/LACF.md`](../docs/LACF.md)
* [The frame format, spec 11](https://github.com/radiohound/aprs-compression-proposal/blob/main/spec/11-native-ppm-frame-byte-plan.md) and the
  [proposal repository](https://github.com/radiohound/aprs-compression-proposal) (tests, results, questions and answers)
* [The collector feed: `docs/COLLECTOR.md`](../docs/COLLECTOR.md)

## In short

* Plain LoRa APRS frames are unchanged. An LACF frame is a different frame type that firmware which does not know it ignores.
* **Transmit is off by default.** On the web page, Digipeating, last row, **Tx frame format** chooses Text (standard) or Hybrid RXT Compression (LACF).
  Receiving LACF frames is always on, and only has an effect if someone sends them.
* **Size:** 43.8% fewer bytes on 14,305 real packets (86.6 B to 48.7 B on average), and 41.9% on 93,145 held-out packets from a 24-hour APRS-IS capture,
  with the current frame (CRC-16 included). Single frames sent by this iGate: 29 B instead of 50 B.
* **Flash use:** the earlier whole-text compressed format (`3C FF 81`, `3C FF C1`) has been removed from this branch, because no station sends it any more. Frames of that kind are dropped and counted by the
  collector. The LoRa32 build now uses 85.0% of the app slot and 53,876 bytes of RAM.
* Tested on the air between a LoRa32 iGate, a T-Deck tracker and a Dire Wolf station on a hill, on 31 October and 1 November 2026. The test results are in
  [`docs/lacf-test-results-2026-11-01.md`](https://github.com/radiohound/aprs-compression-proposal/blob/main/docs/lacf-test-results-2026-11-01.md).
  Not tested yet: a busy channel, other spreading factors, how other firmware reacts to an LACF frame, and power draw. Please use spare boards only.

## Try it

The firmware files and how to install them (browser flashing, and the Update button for a board that already runs this code) are on
[tiles.aivu.com/lacf](https://tiles.aivu.com/lacf). The build there may be older than this branch.

## More

* Questions, results and problems: please open an [issue on this fork](https://github.com/radiohound/LoRa_APRS_iGate/issues).

Licence: GPL-3.0, as in the original project.
