# Experimental branch: compressed LoRa APRS frames (APRS-PPM)

This is a branch of a **fork** of [richonguzman/LoRa_APRS_iGate](https://github.com/richonguzman/LoRa_APRS_iGate)
(the original project and its own README are there). It is an experiment, **not a pull request** and not part of
that project. It adds an optional frame format that carries the same APRS text in fewer bytes.

It is an idea that is showing promise and needs more testing, shared so people can try it, not as a request: **no pull request will be opened against the original project
unless its maintainer says a pull request would be welcome.**

**Start here: [what this branch is for, what has been proven, what has not, and how to try it safely](https://github.com/radiohound/LoRa_APRS_iGate/blob/feature/order3-compression/tools/aprs_compress/EXPERIMENT.md)**

## In short

* Plain LoRa APRS frames are unchanged. A compressed frame is a different frame type that firmware which does
  not know it ignores.
* **Transmit is off by default.** Receiving compressed frames is always on, and only has an effect if someone sends them.
* Tested over the air (24 to 25 October 2026, 433.775 MHz, SF12, three kinds of hardware over two links): every packet
  decoded exactly, in both directions between this firmware and a Direwolf build, and from a T-Deck tracker. A 75-byte
  position packet took 38 B (full frame) or 14 B (frame that refers to the station's previous packet). Over a full day,
  a T-Deck beaconing every 15 minutes to a Direwolf iGate about 2 miles away had 80 beacons expanded of about 96 sent,
  with 51 % fewer bytes on the air.
* Not tested yet: a busy channel, a really weak signal, other spreading factors, and how other firmware reacts to a
  compressed frame. Please use spare boards only.

## More

* [A short illustrated summary (PDF)](https://github.com/radiohound/aprs-compression-proposal/blob/main/docs/brochure.pdf) and [questions and answers](https://github.com/radiohound/aprs-compression-proposal/blob/main/docs/faq.md)
* [Format description, tests and results](https://github.com/radiohound/aprs-compression-proposal) (the proposal repository)
* [The over-the-air test log and checklist](https://github.com/radiohound/aprs-compression-proposal/blob/main/docs/over-the-air-test.md)
* Questions, results and problems: please open an [issue on this fork](https://github.com/radiohound/LoRa_APRS_iGate/issues).

Licence: GPL-3.0, as in the original project.
