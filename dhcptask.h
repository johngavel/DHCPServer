#ifndef __GAVEL_DHCPTASK
#define __GAVEL_DHCPTASK

#include "DHCPLite.h"
#include "dhcpmemory.h"

#include <EthernetUdp.h>
#include <architecture.h>

class DHCPTask : public Task {
public:
  DHCPTask() : Task("DHCPTask"), broadcast(nullptr) { memset(packetBuffer, 0, DHCP_MESSAGE_SIZE); };
  void setupTask();
  void executeTask();

private:
  EthernetUDP Udp;
  char packetBuffer[DHCP_MESSAGE_SIZE]; // buffer to hold incoming packet,
  IPAddress* broadcast;
  const char* domainName = "testsite.net";
};

#endif
