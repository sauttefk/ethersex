/* port the enc28j60 is attached to */
pin(SPI_CS_NET, SPI_CS_HARDWARE)

ifdef(`conf_IRMP', `dnl
pin(IRMP_RX, PD0)
#define IRMP_USE_TIMER0
#define IRMP_RX_LOW_ACTIVE
undef IRMP_RX_LED_LOW_ACTIVE
pin(STATUSLED_IRMP_RX, PD7, OUTPUT)
pin(IRMP_TX, PC2) dnl OC2/OC2A
#undef IRMP_TX_LED_LOW_ACTIVE
#pin(STATUSLED_IRMP_TX, PD7, OUTPUT)
')

ifdef(`conf_STATUSLED_HB_ACT', `dnl
  pin(STATUSLED_HB_ACT, PD7, OUTPUT)
')dnl

ifdef(`conf_ONEWIRE', `dnl
  /* onewire port range */
  ONEWIRE_PORT_RANGE(PG4, PG4)
')dnl

ifdef(`conf_HD44780', `
  pin(HD44780_RS, PB5)
  pin(HD44780_RW, PB6)
  pin(HD44780_EN1, PB7)
  pin(HD44780_D4, PG0)
  pin(HD44780_D5, PG1)
  pin(HD44780_D6, PG2)
  pin(HD44780_D7, PG3)
')

