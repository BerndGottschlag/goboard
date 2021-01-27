/* Linker script to configure memory regions. */

/* 152K = 0x26000 */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000 + 152K, LENGTH = 0xed000 - 152K
  EXTFLASH (rx) : ORIGIN = 0x12000000, LENGTH = 0x8000000
  RAM (rwx) : ORIGIN = 0x20000000 + 0x10000, LENGTH = 0x40000 - 0x10000
  CODE_RAM (rwx) : ORIGIN = 0x800000, LENGTH = 0x40000
}


INCLUDE "nrf_common.ld"
