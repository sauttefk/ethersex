#include <stdint.h>

#define crc32init() ((uint32_t)0xffffffff)
#define crc32finish(crc) (crc = crc^0xffffffff)
#define crc32update(crc,byte) (crc = _crc32update(crc, byte))

__inline__ uint32_t _crc32update(uint32_t crc, const uint8_t byte);

