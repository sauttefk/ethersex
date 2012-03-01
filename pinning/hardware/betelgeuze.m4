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
