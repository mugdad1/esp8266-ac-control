#pragma once
// IR frame builder for the Westinghouse AC.
//
// Layout reverse-engineered bit-for-bit from the original Global Cache
// captures (OFF, on_18..on_25, swing variants) - every bit is accounted
// for and verified against all 17 captures. There is no checksum.
//
// Frame 0:  header 9000/4450
//   byte0: bits0-1 sync (11), bit4 power, bits5-7 swing (111=on)
//   byte1: (temp - 16) << 1        (4-bit field: temp 16..31; slider 23..30)
//   byte2: 0xC0 (fixed - proven pair, bit6 = display light toggle)
//   byte3: 0xA0                    (fixed)
//   nibble: 0x4                    (fixed)
// Frame gap 19900 us
// Frame 1:  header = first data bit (no special header)
//   byte0: swing mirror (bit0 - the first transmitted bit)
//   byte1: 0x20       byte2: 0x00
//   byte3: power off  -> 0xF0 (sentinel); power on -> ((temp - 11) & 0xF) << 4
//         (4-bit field, value temp-11, proven on_18..on_25 = 0x70..0xE0.
//          Extrapolated: 26 -> 0xF0 (same byte as OFF - frame 0's power bit
//          is the only difference), 27..30 wrap in-nibble to 0x00/0x10/0x20/
//          0x30. No captures above 25 exist anywhere, so 26..30 are the
//          protocol-math best guess, pending live test 2026-10-18.)
// Trailing mark. 38 kHz carrier, mark 650 us, 0-space 580 us, 1-space 1650 us.

#include <stdint.h>
#include <vector>

// ESPHome raw transmit requires signed alternating values: positive = mark
// (carrier on), negative = gap (carrier off). Gaps are stored negated.

static int tx_buf[160];
static int tx_len = 0;

// flags2 = payload byte 2; always 0xC0 in the final build - the proven
// pattern of every captured frame (bits6-7 = display light pair).
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
  // Power-off is context-strict on this unit: the original OFF capture is
  // (temp 18, swing 000), and live testing 2026-10-17 proved every other
  // combination (temp 24, swing on) is silently ignored while temp 18 +
  // swing off turns the unit off. Off frames always replay that photo;
  // the sliders keep their values for the next on-frame.
  if (!on) {
    temp = 18;
    swing = false;
  }
  if (temp < 16) temp = 16;   // frame-0 field range guard (16..31)
  if (temp > 30) temp = 30;
  // Frame-1 byte3 echo: 4-bit field = (temp - 11) & 0xF, no cap. The capped
  // echo (0xE0 for every temp > 25) made frame 1 contradict frame 0's true
  // 26..30 and the AC rejected those frames outright (live test 2026-10-18
  // 08:06). 26 = 0xF0 equals the off-frame byte but frame 0 carries the
  // power bit, which is how the unit tells them apart in the captures.
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
  emit_byte(on ? (uint8_t)(((temp - 11) & 0x0F) << 4) : 0xF0);
  tx_buf[i++] = MARK;
  tx_len = i;
}
