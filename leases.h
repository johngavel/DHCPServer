#ifndef __DHCP_LEASES
#define __DHCP_LEASES

#include <Arduino.h>

/* DHCP lease status */
#define DHCP_LEASE_AVAIL 0
#define DHCP_LEASE_OFFER 1
#define DHCP_LEASE_ACK 2

#define INVALID_LEASE 0xFF

class Lease {
public:
  static unsigned long getLeaseTime();
  static bool setLeaseTime(unsigned long time);

  static bool validLease(byte lease);
  static bool validLeaseNumber(byte lease);
  static void setLease(byte lease, byte* __macAddress, long expires, byte status);
  static byte getLease(byte* __macAddress);
  static byte getNewLease();
  static void swapLease(byte lease1, byte lease2);
  static void deleteLease(byte lease);

  static byte getIncrementPop();
  static bool setIncrementPop(byte pop);

  static byte* getLeaseMACAddress(byte lease);
  static bool getLeaseIPAddress(byte lease, byte* ipAddress);

  static void setIgnore(byte lease, bool ignore);
  static bool ignoreLease(byte lease);

  static byte getLeaseStatus(byte lease);

  static String leaseStatusString(long status);

  static bool getLeaseExpired(byte lease, long timeMs);
  static long getLeaseExpiresSec(byte lease, long timeMs);
};

#endif