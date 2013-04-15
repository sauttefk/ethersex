/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
 * (c) 2012 by Jörg Henne <hennejg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "rscp.h"

#ifdef RSCP_SUPPORT

static const ioConfig_t portConfig[RSCP_IOS] PROGMEM = { RSCP_IO_CONFIG(C) };

typedef struct  __attribute__ ((packed))
{
  uint16_t port;                // port id
  union {
    uint8_t flags;              // bit flags
    struct {
      uint8_t :1;
      uint8_t debounceDelay:4;  // the debounce delay in increments of 20ms
      uint8_t pullup:1;         // weak pullup resistor
      uint8_t report:1;         // report on change
      uint8_t negate:1;         // negate polarity
    };
  };
} binaryInputChannelConfig;

typedef struct  __attribute__ ((packed))
{
  union {
    uint8_t status;             // status flags
    struct {
      uint8_t lastRawState:1;
      uint8_t lastDebouncedState:1;
      uint8_t didChangeState:1;
      uint8_t debounceCounter:4;
      uint8_t :1;
    };
  };
} binaryInputChannelState;

static uint16_t numBinaryInputChannels;
static uint16_t firstBinaryInputChannelID;
static binaryInputChannelConfig *binaryInputChannelConfigs;
static binaryInputChannelState *binaryInputChannels = 0;

typedef struct  __attribute__ ((packed))
{
  uint16_t port;                // port id
  union {
    uint8_t flags;              // bit flags
    struct {
      uint8_t :4;
      uint8_t openDrain:1;      // open drain output
      uint8_t openSource:1;     // open source output => combined: bipolar
      uint8_t report:1;         // report on change
      uint8_t negate:1;         // negate polarity
    };
  };
} binaryOutputChannelConfig;

typedef struct  __attribute__ ((packed))
{
  uint8_t lastState:1;
} binaryOutputChannelState;

static uint16_t numBinaryOutputChannels;
static uint16_t firstBinaryOutputChannelID;
static binaryOutputChannelConfig *binaryOutputChannelConfigs;
static binaryOutputChannelState *binaryOutputChannels = 0;


static uint8_t setPortDDR(uint16_t portID, uint8_t value) {
  portPtrType portDDR =
      (portPtrType) pgm_read_word(&portConfig[portID].ddr);
  uint8_t bit = 1 << pgm_read_byte(&portConfig[portID].pin);

  RSCP_DEBUG_IO("set DDR port %d (%x, bit %d): %d\n", portID,
      pgm_read_word(&portConfig[portID].ddr), bit, value);

  // set direction
  if(value)
    return *portDDR |= bit;
  else
    return *portDDR &= ~bit;
}


static uint8_t setPortPORT(uint16_t portID, uint8_t value) {
  portPtrType portOut =
      (portPtrType) pgm_read_word(&portConfig[portID].portOut);
  uint8_t bit = 1 << pgm_read_byte(&portConfig[portID].pin);

  RSCP_DEBUG_IO("set PORT port %d (%x, bit %d): %d\n", portID,
      pgm_read_word(&portConfig[portID].portOut), bit, value);

  // set pullup
  if(value)
    return *portOut |= bit;
  else
    return *portOut &= ~bit;
}


//static uint8_t rscp_togglePortPORT(uint16_t portID) {
//  portPtrType portOut =
//      (portPtrType) pgm_read_word(&rscp_portConfig[portID].portOut);
//  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);
//
//  RSCP_DEBUG_IO("toggle PORT port %d (%x, bit %d)\n", portID, portOut, bit);
//
//  return *portOut ^= bit;
//}
//
//
//static uint8_t rscp_getPortPIN(uint16_t portID) {
//  portPtrType portIn =
//      (portPtrType) pgm_read_word(&rscp_portConfig[portID].portIn);
//  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);
//  return *portIn & bit ? 1 : 0;
//}

void rscp_parseBIC(void *ptr, uint16_t items, uint16_t firstChannelID) {
  if(binaryInputChannels)
    free(binaryInputChannels);

  binaryInputChannels = malloc(items * sizeof(binaryInputChannelState));
  if (!binaryInputChannels) {
    numBinaryInputChannels = 0;
    RSCP_DEBUG_CONF("Out of memory\n");
    return;
  }

  binaryInputChannelConfigs = (binaryInputChannelConfig*) ptr;
  numBinaryInputChannels = items;

  memset(binaryInputChannels, 0, items * sizeof(binaryInputChannelState));
  RSCP_DEBUG_CONF(
      "Allocated %d bytes for %d binary input channels\n", items * sizeof(binaryInputChannelState), items);

  binaryInputChannelConfig bicc;
  for (uint16_t i = 0; i < items; ++i) {
    rscpEEReadStruct(&bicc, binaryInputChannelConfigs + i);

    RSCP_DEBUG_CONF(
        "binary input: port: %d - flags: %02x -> %c%c%c\n", bicc.port, bicc.flags, bicc.pullup ? 'P' : 'p', bicc.negate ? 'N' : 'n', bicc.report ? 'R' : 'r');

    setPortDDR(bicc.port, 0);
    setPortPORT(bicc.port, bicc.pullup);
  }
}

void rscp_parseBOC(void *ptr, uint16_t items, uint16_t firstChannelID) {
  if(binaryOutputChannels)
    free(binaryOutputChannels);

  binaryOutputChannels = malloc(items * sizeof(binaryOutputChannelState));
  if (binaryOutputChannels == NULL ) {
    numBinaryOutputChannels = 0;
    RSCP_DEBUG_CONF("out of memory\n");
    return;
  }

  numBinaryOutputChannels = items;
  binaryOutputChannelConfigs = (binaryOutputChannelConfig*) ptr;

  memset(binaryOutputChannels, 0, items * sizeof(binaryOutputChannelState));
  RSCP_DEBUG_CONF(
      "allocated %d bytes for %d binary output channels\n", items * sizeof(binaryOutputChannelState), items);

  binaryOutputChannelConfig bocc;
  for (uint16_t i = 0; i < items; ++i) {
    rscpEEReadStruct(&bocc, binaryOutputChannelConfigs + i);

    if(i==0)
      firstBinaryOutputChannelID = firstChannelID + i;

    RSCP_DEBUG_CONF(
        "binary output: port:%d - flags: 0x%02x -> %c%c%c%c\n", bocc.port, bocc.flags, bocc.openDrain ? 'D' : 'd', bocc.openSource ? 'S' : 's', bocc.negate ? 'N' : 'n', bocc.report ? 'R' : 'r');

    setPortDDR(bocc.port, 1);
    setPortPORT(bocc.port, bocc.negate);
  }
}

/* ---------------------------------------------------------------------------
 * change of button state
 */

static void pollBinaryOutputChannelState(binaryOutputChannelState *boc, binaryOutputChannelConfig *bocc, uint16_t channelID)  {
  /* get current value from portpin... */
  volatile uint8_t portState = *((portPtrType) pgm_read_word(&portConfig[bocc->port].portIn));
  uint8_t bit = 1 << pgm_read_byte(&portConfig[bocc->port].pin);
  uint8_t curState = (portState & bit ? 1 : 0) ^ (bocc->negate ? 1 : 0);

  /* current state hasn't changed since the last read... */
  if (boc->lastState != curState)
  {
    boc->lastState = curState;
    RSCP_DEBUG_IO("BinaryOutputChannel %hu changed to %hu\n", channelID,
    boc->lastState);
    rscp_txBinaryIOChannelChange(channelID, boc->lastState);
  }
}

void
rscp_IOChannels_periodic(void)
{
  /* Check all configured inputs */
  binaryInputChannelConfig bicc;
  for (uint8_t i = 0; i < numBinaryInputChannels; i++)
  {
    binaryInputChannelState *bic = &binaryInputChannels[i];

    // read config from eeprom
    rscpEEReadStruct(&bicc, binaryInputChannelConfigs + i);

    /* get current value from portpin... */
    volatile uint8_t portState =
        *((portPtrType) pgm_read_word(&portConfig[bicc.port].portIn));
    uint8_t bit = 1 << pgm_read_byte(&portConfig[bicc.port].pin);
    uint8_t curState = (portState & bit ? 1 : 0) ^ (bicc.negate ? 1 : 0);
    uint16_t channelID = firstBinaryInputChannelID + i;

    /* current state hasn't changed since the last read... */
    if (bic->lastRawState == curState)
    {
      /* if the current button state is different from the last stable state,
       * run the debounce timer. Also keep the debounce timer running if the
       * button is pressed, because we need it for long press/repeat
       * recognition */
      if (bic->lastRawState != bic->lastDebouncedState) {
        RSCP_DEBUG_IO("c %d debounce for: %d - %d (%d)\n", channelID,
          curState, bic->debounceCounter, bicc.debounceDelay);
        bic->debounceCounter++;
      }
    }
    else
    {
      /* current state has changed since the last read.
       * restart the debounce timer */
      RSCP_DEBUG_IO("c %d raw change to: %d\n", channelID, curState);
      bic->debounceCounter = 0;
      bic->lastRawState = curState;
    }

    /* button was stable for debounceDelay * 20 ms */
    if (bic->lastRawState != bic->lastDebouncedState &&
        bicc.debounceDelay <= bic->debounceCounter)
    {
      bic->lastDebouncedState = bic->lastRawState;
      RSCP_DEBUG_IO("Debounced BinaryInputChannel %hu changed to %hu\n",
          channelID, bic->lastDebouncedState);
      rscp_txBinaryIOChannelChange(channelID, bic->lastDebouncedState);
    }
  }

  /* Check all configured outputs */
  binaryOutputChannelConfig bocc;
  for (uint8_t i = 0; i < numBinaryOutputChannels; i++)
  {
    rscpEEReadStruct(&bocc, binaryOutputChannelConfigs + i);
    pollBinaryOutputChannelState(&binaryOutputChannels[i], &bocc, firstBinaryOutputChannelID + i);
  }
}

static void setBinaryOutputChannel(binaryOutputChannelState *boc, binaryOutputChannelConfig *bocc, uint16_t channelID,
    uint8_t* payload) {
  RSCP_DEBUG("setBinaryOutputChannel(%d): ", channelID);

  switch (payload[0]) {
  case 0x10: // boolean false
    RSCP_DEBUG("off\n");
    setPortPORT(bocc->port, 0);
    break;
  case 0x11: // boolean true
    RSCP_DEBUG("on\n");
    setPortPORT(bocc->port, 1);
    break;
  default:
    RSCP_DEBUG(
        "Invalid value for setBinaryOutputChannel of type %d", payload[0]);
    break;
  }

  pollBinaryOutputChannelState(boc, bocc, channelID); // report new state of output
}

bool rscp_maybeHandleBOC_CSC(uint16_t channelID, uint8_t *payload) {
  if(channelID >= firstBinaryOutputChannelID
      && channelID < firstBinaryOutputChannelID + numBinaryOutputChannels) {
    uint16_t idx = channelID - firstBinaryOutputChannelID;
    binaryOutputChannelConfig bocc;

    rscpEEReadStruct(&bocc, binaryOutputChannelConfigs + idx);

    setBinaryOutputChannel(&(binaryOutputChannels[idx]), &bocc, channelID, &(payload[2]));

    return true;
  }
  return false;
}

#endif /* RSCP_SUPPORT */

/*
  -- Ethersex META --
  header(protocols/rscp/rscp_io.h)
  timer(1, rscp_IOChannels_periodic())
  block(Miscelleanous)
 */
