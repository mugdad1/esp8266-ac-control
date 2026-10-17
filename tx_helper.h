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
//   byte2: flags byte              (bits6-7 proven pair, bits0-5 test)
//   byte3: 0xA0                    (fixed)
//   nibble: 0x4                    (fixed)
// Frame gap 19900 us
// Frame 1:  header = first data bit (no special header)
//   byte0: swing mirror (bit0 - the first transmitted bit)
//   byte1: 0x20       byte2: 0x00
//   byte3: power off  -> 0xF0 (sentinel); power on -> (temp - 11) << 4
// Trailing mark. 38 kHz carrier, mark 650 us, 0-space 580 us, 1-space 1650 us.

#include <stdint.h>
#include <vector>

// ESPHome raw transmit requires signed alternating values: positive = mark
// (carrier on), negative = gap (carrier off). Gaps are stored negated.

static int tx_buf[160];
static int tx_len = 0;

// flags2 = full payload byte 2: bits6-7 are proven (11 = the pair that
// toggled the display light live, kept at 11 by default), bits0-5 are
// unknown-function test fields (fan speed is somewhere in here or in
// frame 1 - all 17 captures had them constant).
static void build_ac_frame(int temp, int flags2, bool swing, bool on) {
  const int MARK = 650, SPACE0 = -580, SPACE1 = -1650;
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
  tx_buf[i++] = -4450;
  // Frame 0 payload is 35 bits: skip b0's first bit (it duplicated the
  // header when the layout was reverse-engineered - verified against the
  // captures: AC's view == this sequence). Structure matches the proven
  // arrays exactly: 35 bit pairs, lone mark, frame gap, frame 1, tail.
  uint8_t b0 = (uint8_t)((on ? 0x10 : 0x00) | (swing ? 0xE0 : 0x00) | 0x03);
  for (int k = 1; k < 8; k++)
    emit_bit((b0 >> k) & 1);
  emit_byte((uint8_t)((temp - 16) << 1));
  emit_byte((uint8_t) flags2);
  emit_byte(0xA0);
  emit_bit(0); emit_bit(0); emit_bit(1); emit_bit(0);
  tx_buf[i++] = MARK;
  tx_buf[i++] = -19900;
  emit_byte(swing ? 0x01 : 0x00);
  emit_byte(0x20);
  emit_byte(0x00);
  emit_byte(on ? (uint8_t)((temp - 11) << 4) : 0xF0);
  tx_buf[i++] = MARK;
  tx_len = i;
}
