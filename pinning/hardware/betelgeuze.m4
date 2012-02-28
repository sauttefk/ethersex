/* port the enc28j60 is attached to */
pin(SPI_CS_NET, SPI_CS_HARDWARE)

ifdef(`conf_IRMP', `dnl
pin(IRMP_RX, PC0)
#define IRMP_USE_TIMER0
#define IRMP_RX_LOW_ACTIVE
#undef IRMP_RX_LED_LOW_ACTIVE
pin(STATUSLED_IRMP_RX, Pc1, OUTPUT)
pin(IRMP_TX, PC2) dnl OC2/OC2A
#undef IRMP_TX_LED_LOW_ACTIVE
pin(STATUSLED_IRMP_TX, PC3, OUTPUT)
')

