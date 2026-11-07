/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
 *
 * This file is part of LoRa APRS iGate.
 *
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIGI_UTILS_H_
#define DIGI_UTILS_H_

#include <Arduino.h>


namespace DIGI_Utils {

    String  buildPacket(const String& path, const String& packet, bool crossFreq);
    String  generateDigipeatedPacket(const String& packet);
    void    processLoRaPacket(const String& packet);
    void    processRelayOnly(const String& front, unsigned key, int rssi, float snr, int freqError);   // a 3C FF 03 frame this station cannot decode: repeated by the digipeater's own rule, never shown, gated or sent as text

}

#endif