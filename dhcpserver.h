#ifndef __DCHP_SERVER_H
#define __DCHP_SERVER_H

#include <EthernetUdp.h>
#include <GavelInterfaces.h>
#include <GavelTask.h>

struct LeaseMac {
  byte macAddress[6];
};

struct LeaseStatus {
  long expires;
  long status;
};

#define LEASESNUM 100
#define INVALID_LEASE 0xFF
/* DHCP lease status */
#define DHCP_LEASE_AVAIL 0
#define DHCP_LEASE_OFFER 1
#define DHCP_LEASE_ACK 2

class DHCPServer : public IMemory, public Task {
public:
  DHCPServer() : IMemory("DHCPServer"), Task("DHCPServer"){};
  virtual void addCmd(TerminalCommand* __termCmd) override;
  virtual void reservePins(BackendPinSetup* pinsetup) override;
  virtual bool setupTask(OutputInterface* __terminal) override;
  virtual bool executeTask() override;

  struct MemoryStruct {
    byte startAddressNumber;
    byte leaseNum;
    unsigned long leaseTime;
    byte spare[30];
    LeaseMac leasesMac[LEASESNUM];
  };

  static_assert(sizeof(MemoryStruct) == 640, "DHCPMemory size unexpected - check packing/padding.");

  typedef union {
    MemoryStruct mem;
    byte memoryArray[sizeof(MemoryStruct)];
  } MemoryUnion;

  LeaseStatus leaseStatus[LEASESNUM];
  MemoryUnion memory;

  void configure(unsigned char* __ipAddress, unsigned char* __subnetMask, unsigned char* __macAddress);
  void updateBroadcast() {
    if (!ipAddress || !subnetMask) {
      broadcastAddress[0] = 0;
      broadcastAddress[1] = 0;
      broadcastAddress[2] = 0;
      broadcastAddress[3] = 0;
      return;
    }
    broadcastAddress[0] = ~subnetMask[0] | ipAddress[0];
    broadcastAddress[1] = ~subnetMask[1] | ipAddress[1];
    broadcastAddress[2] = ~subnetMask[2] | ipAddress[2];
    broadcastAddress[3] = ~subnetMask[3] | ipAddress[3];
  };

  // IMemory overrides
  virtual const unsigned char& operator[](std::size_t index) const override { return memory.memoryArray[index]; }
  virtual unsigned char& operator[](std::size_t index) override { return memory.memoryArray[index]; }
  virtual std::size_t size() const noexcept override { return sizeof(MemoryUnion::memoryArray); }
  virtual void initMemory() override;
  virtual void printData(OutputInterface* terminal) override;
  virtual void updateExternal() {
    updateBroadcast();
    setInternal(true);
  }
  virtual JsonDocument createJson() override;
  virtual bool parseJson(JsonDocument& doc) override;

  /* Lease Control Methods */
  unsigned long getLeaseTime();
  bool setLeaseTime(unsigned long time);

  bool validLease(byte lease);
  bool validLeaseNumber(byte lease);
  void setLease(byte lease, byte* __macAddress, long expires = 0, byte status = DHCP_LEASE_AVAIL);
  byte getLease(byte* __macAddress);
  byte getNewLease();
  void swapLease(byte lease1, byte lease2);
  void deleteLease(byte lease);

  byte* getLeaseMACAddress(byte lease);
  bool getLeaseIPAddress(byte lease, byte* ipAddress);

  void setIgnore(byte lease, bool ignore);
  bool ignoreLease(byte lease);

  byte getLeaseStatus(byte lease);

  String leaseStatusString(long status);

  bool getLeaseExpired(byte lease, long timeMs);
  long getLeaseExpiresSec(byte lease, long timeMs);

  /* Terminal Commands */
  void leaseTime(OutputInterface* terminal);
  void showLeases(OutputInterface* terminal);
  void moveLease(OutputInterface* terminal);
  void removeLease(OutputInterface* terminal);
  void startAddress(OutputInterface* terminal);
  void leaseNum(OutputInterface* terminal);

private:
  EthernetUDP Udp;
  IPAddress* broadcast;
  const char* domainName = "testsite.net";
  byte broadcastAddress[4];
  unsigned char* ipAddress = nullptr;
  unsigned char* subnetMask = nullptr;
  unsigned char* macAddress = nullptr;
};

#endif // __DCHP_SERVER_H