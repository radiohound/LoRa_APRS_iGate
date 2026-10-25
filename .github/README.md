# Experimental branch: compressed LoRa APRS frames (APRS-PPM)

This is a branch of a **fork** of [richonguzman/LoRa_APRS_iGate](https://github.com/richonguzman/LoRa_APRS_iGate)
(the original project and its own README are there). It is an experiment, **not a pull request** and not part of
that project. It adds an optional frame format that carries the same APRS text in fewer bytes.

**Start here: [what this branch is for, what has been proven, what has not, and how to try it safely](https://github.com/radiohound/LoRa_APRS_iGate/blob/feature/order3-compression/tools/aprs_compress/EXPERIMENT.md)**

## In short

* Plain LoRa APRS frames are unchanged. A compressed frame is a different frame type that firmware which does
  not know it ignores.
* **Transmit is off by default.** Receiving compressed frames is always on, and only has an effect if someone sends them.
* First short-range over-the-air tests (24 October 2026, 433.775 MHz, SF12): every packet decoded exactly, in both
  directions between this firmware and a Direwolf build. A 75-byte position packet took 38 B (full frame) or
  14 B (frame that refers to the station's previous packet): on average 65 % fewer bytes on the air.
* Not tested yet: other boards, other spreading factors, weak signals and lost frames, and how other firmware
  reacts to a compressed frame. Please use spare boards only.

## More

* [Format description, tests and results](https://github.com/radiohound/aprs-compression-proposal) (the proposal repository)
* [The over-the-air test log and checklist](https://github.com/radiohound/aprs-compression-proposal/blob/main/docs/over-the-air-test.md)
* Questions, results and problems: please open an [issue on this fork](https://github.com/radiohound/LoRa_APRS_iGate/issues).

Licence: GPL-3.0, as in the original project.
