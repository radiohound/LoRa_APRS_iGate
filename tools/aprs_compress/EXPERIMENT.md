# APRS-PPM on LoRa APRS: what this branch is, why, and how to try it

This is an **experimental branch**, not a pull request and not a request to change this project. It exists so
that anyone interested can build it, try it on spare hardware, and tell us what they find. Nothing here changes
how a stock station behaves unless you switch it on.

## What it does

A LoRa APRS packet is plain text, typically 50 to 80 bytes on the air. This branch adds an optional second frame
format that carries the same text **compressed**, using a small fixed table of statistics learned from real
APRS packets (an order-3 PPM model feeding a range coder, integer arithmetic only). Plain frames
(`0x3C 0xFF 0x01`) are untouched. Compressed frames begin `0x3C 0xFF 0x81` (full) or `0x3C 0xFF 0xC1` (primed:
coded against the same station's previous packet, so a repeated beacon costs only a few bytes).
The exact format is in [README.md](README.md).

## Why

At SF12 and 125 kHz a 57-byte frame is on the air for about 2.6 seconds, so the channel fills up quickly and a
small saving buys real airtime. The comment field costs more than the position, and a tracker repeats the same
comment every time. A shorter frame also has a better chance of getting through a marginal link. The aim is a
change that costs a legacy network nothing and gives adopters the saving.

## What has been measured

Over the air, 24 October 2026, 433.775 MHz, SF12, BW 125 kHz, CR 4/5, about 2 miles line of sight:

| Test | Result |
|---|---|
| ESP32 (this firmware) to a Raspberry Pi running a Direwolf branch | 12 of 12 average-sized position packets decoded to exactly the text sent |
| Same, size: 75-byte plain frame | full frame 38.0 B (49 % smaller), primed frame 13.8 B (82 % smaller), alternating 25.9 B (65 %) |
| Direwolf transmit to this ESP32 firmware | 5 of 5 station beacons decoded; full 25 B, primed 8 B, against 57 B plain |

The two programs share no code, so the second test is also a check that the written format is unambiguous.
The primed numbers are best case, because the test beacon repeats exactly. A measurement of the real codec on
captured packets from stations the model had not seen gave 21 B primed and 33 B full against 60 B plain.
Simulations on three hours of captured traffic gave 66 to 70 % overall with priming; those are simulations.
More detail and the test log: the `aprs-compression-proposal` repository (`docs/over-the-air-test.md`).

## What has *not* been tested

* Only one board: a TTGO LoRa32 V2.1 (ESP32 with an SX127x radio, 433 MHz). Other boards are untested.
* Only SF12 / 125 kHz. Other spreading factors are untested.
* Weak signals and lost frames over the air (the code handles a lost reference frame by dropping the primed
  frame that needs it; this has been checked in a simulation and a unit test, not on a marginal link).
* What *other* firmware does with a compressed frame. This firmware ignores frames it does not know (it looks for
  exactly `0x3C 0xFF 0x01`), and the Direwolf LoRa driver, without the compression code, logs one as a frame it could not parse and carries on. We have
  not checked trackers, T-Decks or other iGates, so please do not enable transmit in a network where you cannot
  tell what they do.

## Defaults and safety

* **Receive is always on** for compressed frames. It only has an effect if someone sends them.
* **Transmit is off by default** (`lora.compressTx`: false). A frame is only sent compressed when it is smaller.
* A frame is sent in full at least every `lora.compressFullEvery` packets (default 2); the others are primed.
* Flash: the ESP32 build is 88.3 % full on the 1.97 MB application partition (about 6 points of that is the
  125 KB model table). RAM 21.7 %. Check your own board's headroom before trying it.

## How to try it safely

1. Use **spare boards**, not a live station.
2. Build with the test mode, which sends a fixed list of packets and checks what it receives, never passing them
   on to APRS-IS, the digipeater, KISS or MQTT:

       PLATFORMIO_BUILD_FLAGS="-DAPRS_PPM_SELFTEST -DAPRS_PPM_SELFTEST_TX=20" pio run -e <your-env> -t upload   # sender
       PLATFORMIO_BUILD_FLAGS="-DAPRS_PPM_SELFTEST" pio run -e <your-env> -t upload                            # receiver

3. For a link check against a real station, two more build options send packets that cannot be gated:
   `-DAPRS_PPM_ONESHOT` (16 status packets with `RFONLY` in the path, no position, 30 s apart) and
   `-DAPRS_PPM_POSTEST` (12 compressed position packets, made-up coordinates, `RFONLY`). Use your own callsign,
   keep APRS-IS and the digipeater off, and transmit only on a frequency you are allowed to use.
4. The checklist and the pass/fail criteria are in the proposal repository, `docs/over-the-air-test.md`.

## What we would like

* Anyone willing to run the test on other boards or spreading factors and report what they see, good or bad.
* Review of the format and the choice of defaults. The format is deliberately small and versioned (a retrained
  table gets a new version number), so it can change if you find a problem.
* If, after that, the maintainers want it in the main project, we will prepare a pull request sized to their
  limits (for example without the test modes, or with the model in a smaller table). Not before.

## Licence and data

GPL-3.0, like the rest of this project. The model table is a set of statistics (byte-context counts, minimum
count 8) learned from public APRS-IS traffic; no raw packets are included. The positions in the test
packets are made up.

Contact: open an issue on this fork (issues are enabled there).
