#pragma once
// IR frame builder for the Westinghouse AC.
//
// Layout reverse-engineered bit-for-bit from the original Global Cache
// captures (OFF, on_18..on_25, swing variants) - every bit is accounted
// for and verified against all 17 captures. There is no checksum.
//
// Frame 0:  header 9000/4450
//   byte0: bits0-1 sync (11), bit4 power, bits5-7 swing (111=on)
//   byte1: (temp - 16) << 1        (temp range 16..25, field full at 25)
//   byte2: fan << 6                (bits6-7, 11 = the value every
//                                   original capture used = likely auto)
//   byte3: 0xA0                    (fixed)
//   nibble: 0x4                    (fixed)
// Frame gap 19900 us
// Frame 1:  header = first data bit (no special header)
//   byte0: swing mirror (bit7)
//   byte1: 0x20       byte2: 0x00
//   byte3: power off  -> 0xF0 (sentinel); power on -> (temp - 11) << 4
// Trailing mark. 38 kHz carrier, mark 650 us, 0-space 580 us, 1-space 1650 us.

#include <stdint.h>
#include <vector>

static int tx_buf[160];
static int tx_len = 0;

static void build_ac_frame(int temp, int fan, bool swing, bool on) {
  const int MARK = 650, SPACE0 = 580, SPACE1 = 1650;
  int i = 0;
  auto emit_bit = [&](int bit) {
    tx_buf[i++] = MARK;
    tx_buf[i++] = bit ? SPACE1 : SPACE0;
  };
  auto emit_byte = [&](uint8_t b) {
    for (int k = 0; k < 8; k++)
      emit_bit((b >> k) & 1);
  };
  tx_buf[i++] = 9000;
  tx_buf[i++] = 4450;
  emit_byte((uint8_t)((on ? 0x10 : 0x00) | (swing ? 0xE0 : 0x00) | 0x03));
  emit_byte((uint8_t)((temp - 16) << 1));
  emit_byte((uint8_t)((fan & 3) << 6));
  emit_byte(0xA0);
  emit_bit(0); emit_bit(0); emit_bit(1); emit_bit(0);
  tx_buf[i++] = 19900;
  emit_byte(swing ? 0x80 : 0x00);
  emit_byte(0x20);
  emit_byte(0x00);
  emit_byte(on ? (uint8_t)((temp - 11) << 4) : 0xF0);
  tx_buf[i++] = MARK;
  tx_len = i;
}
