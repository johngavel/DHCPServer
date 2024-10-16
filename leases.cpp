#include "leases.h"

#include "dhcpmemory.h"

#include <eeprom.h>

extern DHCPMemory dhcpMemory;
static const byte blankMac[6] = {0, 0, 0, 0, 0, 0};

static bool blankMAC(byte* mac) {
  if (memcmp(mac, blankMac, 6) == 0)
    return true;
  else
    return false;
}

unsigned long Lease::getLeaseTime() {
  return DHCP_MEMORY.leaseTime;
}

bool Lease::setLeaseTime(unsigned long time) {
  DHCP_MEMORY.leaseTime = time;
  return true;
}

bool Lease::validLease(byte lease) {
  bool value = false;
  if (validLeaseNumber(lease)) value = !blankMAC(DHCP_MEMORY.leasesMac[lease].macAddress);
  return value;
}

bool Lease::validLeaseNumber(byte lease) {
  bool value = false;

  if ((lease != INVALID_LEASE) && (lease < DHCP_MEMORY.leaseNum)) { value = true; }
  return value;
}

void Lease::setLease(byte lease, byte* __macAddress, long expires, byte status) {
  if (validLeaseNumber(lease)) {
    memcpy(DHCP_MEMORY.leasesMac[lease].macAddress, __macAddress, 6);
    dhcpMemory.leaseStatus[lease].expires = expires;
    dhcpMemory.leaseStatus[lease].status = status;
    dhcpMemory.leaseStatus[lease].ignore = false;
  }
}

byte Lease::getNewLease() {
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  EEPROM->breakSeal();
  return getLease(blank);
}

byte Lease::getLease(byte* __macAddress) {
  for (byte lease = 0; lease < DHCP_MEMORY.leaseNum; lease++)
    if (memcmp(DHCP_MEMORY.leasesMac[lease].macAddress, __macAddress, 6) == 0) return lease;

  // Clean up expired leases; need to do after we check for existing leases because of this iOS bug
  // http://www.net.princeton.edu/apple-ios/ios41-allows-lease-to-expire-keeps-using-IP-address.html
  // Don't need to check again AFTER the clean up as for DHCP REQUEST the client should already have the lease
  // and for DHCP DISCOVER we will check once more to assign a new lease
  long currTime = millis();
  for (byte lease = 0; lease < DHCP_MEMORY.leaseNum; lease++) {
    if ((validLease(lease)) && (dhcpMemory.leaseStatus[lease].expires < currTime)) dhcpMemory.leaseStatus[lease].status = DHCP_LEASE_AVAIL;
  }

  return INVALID_LEASE;
}

void Lease::swapLease(byte lease1, byte lease2) {
  LeaseMac tempMac;
  LeaseStatus tempStatus;

  if (validLeaseNumber(lease1) && validLeaseNumber(lease2)) {
    // Copy lease 1 to temp
    memcpy(&tempMac, &DHCP_MEMORY.leasesMac[lease1], sizeof(LeaseMac));
    memcpy(&tempStatus, &dhcpMemory.leaseStatus[lease1], sizeof(LeaseStatus));

    // Copy lease 2 to lease 1
    memcpy(&DHCP_MEMORY.leasesMac[lease1], &DHCP_MEMORY.leasesMac[lease2], sizeof(LeaseMac));
    memcpy(&dhcpMemory.leaseStatus[lease1], &dhcpMemory.leaseStatus[lease2], sizeof(LeaseStatus));

    // Copy temp to lease 2
    memcpy(&DHCP_MEMORY.leasesMac[lease2], &tempMac, sizeof(LeaseMac));
    memcpy(&dhcpMemory.leaseStatus[lease2], &tempStatus, sizeof(LeaseStatus));

    dhcpMemory.leaseStatus[lease1].status = DHCP_LEASE_AVAIL;
    dhcpMemory.leaseStatus[lease2].status = DHCP_LEASE_AVAIL;
  }
}

void Lease::deleteLease(byte lease) {
  if (validLeaseNumber(lease)) {
    memset(&DHCP_MEMORY.leasesMac[lease], 0, sizeof(LeaseMac));
    memset(&dhcpMemory.leaseStatus[lease], 0, sizeof(LeaseStatus));
  }
}

byte* Lease::getLeaseMACAddress(byte lease) {
  return DHCP_MEMORY.leasesMac[lease].macAddress;
}

bool Lease::getLeaseIPAddress(byte lease, byte* ipAddress) {
  memcpy(ipAddress, DHCP_MEMORY.ipAddress, 4);
  ipAddress[0] = ipAddress[0] & DHCP_MEMORY.subnetMask[0];
  ipAddress[1] = ipAddress[1] & DHCP_MEMORY.subnetMask[1];
  ipAddress[2] = ipAddress[2] & DHCP_MEMORY.subnetMask[2];
  ipAddress[3] = ipAddress[3] & DHCP_MEMORY.subnetMask[3];
  ipAddress[3] += (lease + DHCP_MEMORY.startAddressNumber) & 0xFF; // lease starts with 1
  return true;
}

void Lease::setIgnore(byte lease, bool ignore) {
  dhcpMemory.leaseStatus[lease].ignore = ignore;
}

bool Lease::ignoreLease(byte lease) {
  return dhcpMemory.leaseStatus[lease].ignore;
}

byte Lease::getLeaseStatus(byte lease) {
  return dhcpMemory.leaseStatus[lease].status;
}

String Lease::leaseStatusString(long status) {
  String string;
  switch (status) {
  case DHCP_LEASE_AVAIL: string = "DHCP_LEASE_AVAIL"; break;
  case DHCP_LEASE_OFFER: string = "DHCP_LEASE_OFFER"; break;
  case DHCP_LEASE_ACK: string = "DHCP_LEASE_ACK"; break;

  default: string = "UNKNOWN"; break;
  }
  return string;
}

bool Lease::getLeaseExpired(byte lease, long timeMs) {
  if (dhcpMemory.leaseStatus[lease].expires > timeMs)
    return false;
  else
    return true;
}

long Lease::getLeaseExpiresSec(byte lease, long timeMs) {
  long expiredTime;
  if (!getLeaseExpired(lease, timeMs))
    expiredTime = (dhcpMemory.leaseStatus[lease].expires - timeMs) / 1000;
  else
    expiredTime = (timeMs - dhcpMemory.leaseStatus[lease].expires) / 1000;
  return expiredTime;
}
