#include "dhcpserver.h"

static const byte blankMac[6] = {0, 0, 0, 0, 0, 0};

static bool blankMAC(byte* mac) {
  if (memcmp(mac, blankMac, 6) == 0)
    return true;
  else
    return false;
}

unsigned long DHCPServer::getLeaseTime() {
  return memory.mem.leaseTime;
}

bool DHCPServer::setLeaseTime(unsigned long time) {
  memory.mem.leaseTime = time;
  return true;
}

bool DHCPServer::validLease(byte lease) {
  bool value = false;
  if (validLeaseNumber(lease)) value = !blankMAC(memory.mem.leasesMac[lease].macAddress);
  return value;
}

bool DHCPServer::validLeaseNumber(byte lease) {
  bool value = false;

  if ((lease != INVALID_LEASE) && (lease < memory.mem.leaseNum)) { value = true; }
  return value;
}

void DHCPServer::setLease(byte lease, byte* __macAddress, long expires, byte status) {
  if (validLeaseNumber(lease)) {
    memcpy(memory.mem.leasesMac[lease].macAddress, __macAddress, 6);
    leaseStatus[lease].expires = expires;
    leaseStatus[lease].status = status;
  }
}

byte DHCPServer::getNewLease() {
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  return getLease(blank);
}

byte DHCPServer::getLease(byte* __macAddress) {
  for (byte lease = 0; lease < memory.mem.leaseNum; lease++)
    if (memcmp(memory.mem.leasesMac[lease].macAddress, __macAddress, 6) == 0) return lease;

  // Clean up expired leases; need to do after we check for existing leases because of this iOS bug
  // http://www.net.princeton.edu/apple-ios/ios41-allows-lease-to-expire-keeps-using-IP-address.html
  // Don't need to check again AFTER the clean up as for DHCP REQUEST the client should already have the lease
  // and for DHCP DISCOVER we will check once more to assign a new lease
  long currTime = millis();
  for (byte lease = 0; lease < memory.mem.leaseNum; lease++) {
    if ((validLease(lease)) && (leaseStatus[lease].expires < currTime)) leaseStatus[lease].status = DHCP_LEASE_AVAIL;
  }

  return INVALID_LEASE;
}

void DHCPServer::swapLease(byte lease1, byte lease2) {
  LeaseMac tempMac;
  LeaseStatus tempStatus;

  if (validLeaseNumber(lease1) && validLeaseNumber(lease2)) {
    // Copy lease 1 to temp
    memcpy(&tempMac, &memory.mem.leasesMac[lease1], sizeof(LeaseMac));
    memcpy(&tempStatus, &leaseStatus[lease1], sizeof(LeaseStatus));

    // Copy lease 2 to lease 1
    memcpy(&memory.mem.leasesMac[lease1], &memory.mem.leasesMac[lease2], sizeof(LeaseMac));
    memcpy(&leaseStatus[lease1], &leaseStatus[lease2], sizeof(LeaseStatus));

    // Copy temp to lease 2
    memcpy(&memory.mem.leasesMac[lease2], &tempMac, sizeof(LeaseMac));
    memcpy(&leaseStatus[lease2], &tempStatus, sizeof(LeaseStatus));

    leaseStatus[lease1].status = DHCP_LEASE_AVAIL;
    leaseStatus[lease2].status = DHCP_LEASE_AVAIL;
  }
}

void DHCPServer::deleteLease(byte lease) {
  if (validLeaseNumber(lease)) {
    memset(&memory.mem.leasesMac[lease], 0, sizeof(LeaseMac));
    memset(&leaseStatus[lease], 0, sizeof(LeaseStatus));
  }
}

byte* DHCPServer::getLeaseMACAddress(byte lease) {
  return memory.mem.leasesMac[lease].macAddress;
}

bool DHCPServer::getLeaseIPAddress(byte lease, byte* __ipAddress) {
  memcpy(__ipAddress, ipAddress, 4);
  __ipAddress[0] = __ipAddress[0] & subnetMask[0];
  __ipAddress[1] = __ipAddress[1] & subnetMask[1];
  __ipAddress[2] = __ipAddress[2] & subnetMask[2];
  __ipAddress[3] = __ipAddress[3] & subnetMask[3];
  __ipAddress[3] += (lease + memory.mem.startAddressNumber) & 0xFF; // lease starts with 1
  return true;
}

byte DHCPServer::getLeaseStatus(byte lease) {
  return leaseStatus[lease].status;
}

String DHCPServer::leaseStatusString(long status) {
  String string;
  switch (status) {
  case DHCP_LEASE_AVAIL: string = "DHCP_LEASE_AVAIL"; break;
  case DHCP_LEASE_OFFER: string = "DHCP_LEASE_OFFER"; break;
  case DHCP_LEASE_ACK: string = "DHCP_LEASE_ACK"; break;

  default: string = "UNKNOWN"; break;
  }
  return string;
}

bool DHCPServer::getLeaseExpired(byte lease, long timeMs) {
  if (leaseStatus[lease].expires > timeMs) return false;

  return true;
}

long DHCPServer::getLeaseExpiresSec(byte lease, long timeMs) {
  long expiredTime;
  if (!getLeaseExpired(lease, timeMs))
    expiredTime = (leaseStatus[lease].expires - timeMs) / 1000;
  else
    expiredTime = (timeMs - leaseStatus[lease].expires) / 1000;
  return expiredTime;
}
