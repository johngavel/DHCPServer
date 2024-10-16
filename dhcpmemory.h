#ifndef __DCHP_MEMORY
#define __DCHP_MEMORY
#include <eeprom.h>

#define DHCP_DATA ((DHCPMemory*) EEPROM->getData())
#define DHCP_MEMORY DHCP_DATA->memory.mem

struct LeaseMac {
  byte macAddress[6];
};

struct LeaseStatus {
  bool ignore;
  long expires;
  long status;
};

#define LEASESNUM 100

class DHCPMemory : public Data {
public:
  struct MemoryStruct {
    byte macAddress[6];
    byte ipAddress[4];
    byte startAddressNumber;
    byte leaseNum;
    byte spare1[2];
    byte dnsAddress[4];
    byte subnetMask[4];
    byte gatewayAddress[4];
    unsigned long leaseTime;
    byte spare2[4];
    LeaseMac leasesMac[LEASESNUM];
  };

  typedef union {
    MemoryStruct mem;
    byte memoryArray[sizeof(MemoryStruct)];
  } MemoryUnion;

  LeaseStatus leaseStatus[LEASESNUM];
  MemoryUnion memory;
  void setup();
  void initMemory();
  void printData(Terminal* terminal);
  unsigned char* getData();
  unsigned long getLength();
  void updateBroadcast();
  byte broadcastAddress[4];
  void exportMem();
  void importMem();

private:
};

#endif