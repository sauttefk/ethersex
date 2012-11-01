#include <stdint.h>

uint32_t crc32init(void) {
  return 0xffffffff;
}

uint32_t _crc32update(uint32_t crc, const uint8_t byte)
{
  // crc = crc ^ byte;
  asm(
    "eor %A[crc],%[byte] \r\n"
    : [crc] "+r" (crc)
    : [byte] "r" (byte)
    :);


  for(uint8_t i=8; i!=0; i--) {

    uint8_t tmp;

    asm (
      // crc = crc >> 1
      "lsr %D[crc] \r\n"
      "ror %C[crc] \r\n"
      "ror %B[crc] \r\n"
      "ror %A[crc] \r\n"

      "brcc next_%= \r\n"

      // crc ^= 0xedb88320
      "ldi %[tmp], hhi8(%[polynom]) \r\n"
      "eor %D[crc], %[tmp] \r\n"
      "ldi %[tmp], hlo8(%[polynom]) \r\n"
      "eor %C[crc], %[tmp] \r\n"
      "ldi %[tmp], hi8(%[polynom]) \r\n"
      "eor %B[crc], %[tmp] \r\n"
      "ldi %[tmp], lo8(%[polynom]) \r\n"
      "eor %A[crc], %[tmp] \r\n"

      "next_%=:"

      : [crc] "+r" (crc),
        [tmp] "=&a" (tmp)
      : [polynom] "i" (0xEDB88320)
      : );

  }

  return crc;
}

uint32_t crc32finish(uint32_t crc) {
  return crc^0xffffffff;
}
