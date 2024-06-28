#include "lwip/dhcp.h"
// Copyright (C) 2011 by Paul Kulchenko
// Retrieved from Github: https://github.com/pkulchenko/DHCPLite/tree/master

#include "DHCPLite.h"
#include "dhcpmemory.h"
#include "serialport.h"

extern DHCPMemory dhcpMemory;

byte quads[4];
byte* long2quad(unsigned long value) {
  for (int k = 0; k < 4; k++) quads[3 - k] = value >> (k * 8);
  return quads;
}

// leases are numbered 1..LEASENUM (to use 0 to signal no lease)
void setLease(byte lease, byte* __macAddress, long expires, byte status) {
  if (lease > 0 && lease <= DHCP_MEMORY.leaseNum) {
    memcpy(DHCP_MEMORY.leasesMac[lease - 1].macAddress, __macAddress, 6);
    dhcpMemory.leaseStatus[lease - 1].expires = expires;
    dhcpMemory.leaseStatus[lease - 1].status = status;
    EEPROM->breakSeal();
  }
}

byte getLease(byte* __macAddress) {
  for (byte lease = 0; lease < DHCP_MEMORY.leaseNum; lease++)
    if (memcmp(DHCP_MEMORY.leasesMac[lease].macAddress, __macAddress, 6) == 0) return lease + 1;

  // Clean up expired leases; need to do after we check for existing leases because of this iOS bug
  // http://www.net.princeton.edu/apple-ios/ios41-allows-lease-to-expire-keeps-using-IP-address.html
  // Don't need to check again AFTER the clean up as for DHCP REQUEST the client should already have the lease
  // and for DHCP DISCOVER we will check once more to assign a new lease
  long currTime = millis();
  for (byte lease = 0; lease < DHCP_MEMORY.leaseNum; lease++)
    if (dhcpMemory.leaseStatus[lease].expires < currTime) setLease(lease + 1, DHCP_MEMORY.leasesMac[lease].macAddress, 0, DHCP_LEASE_AVAIL);

  return 0;
}

int getOption(int dhcpOption, byte* options, int optionSize, int* optionLength) {
  for (int i = 0; i < optionSize && (options[i] != dhcpEndOption); i += 2 + options[i + 1]) {
    if (options[i] == dhcpOption) {
      if (optionLength) *optionLength = (int) options[i + 1];
      return i + 2;
    }
  }
  if (optionLength) *optionLength = 0;
  return 0;
}

int populatePacket(byte* packet, int currLoc, byte marker, byte* what, int dataSize) {
  packet[currLoc] = marker;
  packet[currLoc + 1] = dataSize;
  memcpy(packet + currLoc + 2, what, dataSize);
  return dataSize + 2;
}

int DHCPreply(RIP_MSG* packet, int packetSize, byte* serverIP, char* domainName) {
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  if (packet->op != DHCP_BOOTREQUEST) return 0; // limited check that we're dealing with DHCP/BOOTP request
  byte OPToffset = (byte*) packet->OPT - (byte*) packet;

  packet->op = DHCP_BOOTREPLY;
  packet->secs = 0; // some of the secs come malformed; don't want to send them back

  int dhcpMessageOffset = getOption(dhcpMessageType, packet->OPT, packetSize - OPToffset, NULL);
  byte dhcpMessage = packet->OPT[dhcpMessageOffset];

  byte lease = getLease(packet->chaddr);
  byte response = DHCP_NAK;
  if (dhcpMessage == DHCP_DISCOVER) {
    if (!lease) lease = getLease(blank); // use existing lease or get a new one
    if (lease && !dhcpMemory.leaseStatus[lease - 1].ignore) {
      response = DHCP_OFFER;
      setLease(lease, packet->chaddr, millis() + 10000, DHCP_LEASE_OFFER); // 10s
      EEPROM->breakSeal();
    }
  } else if (dhcpMessage == DHCP_REQUEST) {
    if (lease && !dhcpMemory.leaseStatus[lease - 1].ignore) {
      response = DHCP_ACK;

      // find hostname option in the request and store to provide DNS info
      setLease(lease, packet->chaddr, millis() + (DHCP_MEMORY.leaseTime * 1000),
               DHCP_LEASE_ACK); // DHCP_LEASETIME is in seconds
    }
  }

  if (lease) { // Dynamic IP configuration
    memcpy(packet->yiaddr, serverIP, 4);
    packet->yiaddr[0] = packet->yiaddr[0] & DHCP_MEMORY.subnetMask[0];
    packet->yiaddr[1] = packet->yiaddr[1] & DHCP_MEMORY.subnetMask[1];
    packet->yiaddr[2] = packet->yiaddr[2] & DHCP_MEMORY.subnetMask[2];
    packet->yiaddr[3] = packet->yiaddr[3] & DHCP_MEMORY.subnetMask[3];
    packet->yiaddr[2] += DHCP_MEMORY.ipAddressPop & 0xFF;
    packet->yiaddr[3] += (lease + DHCP_MEMORY.startAddressNumber - 1) & 0xFF; // lease starts with 1
  }

  int currLoc = 0;
  packet->OPT[currLoc++] = dhcpMessageType;
  packet->OPT[currLoc++] = 1;
  packet->OPT[currLoc++] = response;

  int reqLength;
  int reqListOffset = getOption(dhcpParamRequest, packet->OPT, packetSize - OPToffset, &reqLength);
  byte reqList[12];
  if (reqLength > 12) reqLength = 12;
  memcpy(reqList, packet->OPT + reqListOffset, reqLength);

  // iPod with iOS 4 doesn't want to process DHCP OFFER if dhcpServerIdentifier does not follow dhcpMessageType
  // Windows Vista and Ubuntu 11.04 don't seem to care
  currLoc += populatePacket(packet->OPT, currLoc, dhcpServerIdentifier, serverIP, 4);

  // DHCP lease timers: http://www.tcpipguide.com/free/t_DHCPLeaseLifeCycleOverviewAllocationReallocationRe.htm
  // Renewal Timer (T1): This timer is set by default to 50% of the lease period.
  // Rebinding Timer (T2): This timer is set by default to 87.5% of the length of the lease.
  currLoc += populatePacket(packet->OPT, currLoc, dhcpIPaddrLeaseTime, long2quad(DHCP_MEMORY.leaseTime), 4);
  currLoc += populatePacket(packet->OPT, currLoc, dhcpT1value, long2quad(DHCP_MEMORY.leaseTime * 0.5), 4);
  currLoc += populatePacket(packet->OPT, currLoc, dhcpT2value, long2quad(DHCP_MEMORY.leaseTime * 0.875), 4);

  for (int i = 0; i < reqLength; i++) {
    switch (reqList[i]) {
    case dhcpSubnetMask:
      currLoc += populatePacket(packet->OPT, currLoc, reqList[i], long2quad(0xFFFF0000UL), 4); // 255.255.255.0
      break;
    case dhcpLogServer: currLoc += populatePacket(packet->OPT, currLoc, reqList[i], long2quad(0), 4); break;
    case dhcpDns:
    case dhcpRoutersOnSubnet: currLoc += populatePacket(packet->OPT, currLoc, reqList[i], serverIP, 4); break;
    case dhcpDomainName:
      if (domainName && strlen(domainName)) currLoc += populatePacket(packet->OPT, currLoc, reqList[i], (byte*) domainName, strlen(domainName));
      break;
    }
  }
  packet->OPT[currLoc++] = dhcpEndOption;

  return OPToffset + currLoc;
}
