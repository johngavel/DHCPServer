#include "dhcptask.h"

#include <license.h>
#include <serialport.h>

DHCPTask dhcpTask;
extern DHCPMemory dhcpMemory;

void DHCPTask::setupTask() {
  broadcast = new IPAddress(dhcpMemory.broadcastAddress[0], dhcpMemory.broadcastAddress[1], dhcpMemory.broadcastAddress[2], dhcpMemory.broadcastAddress[3]);
  Udp.begin(DHCP_SERVER_PORT);
  setRefreshMilli(10);
  LICENSE_ADD("DHCPLite", "Ver. 0.14", "DHCP_LICENSE.txt");
  CONSOLE->println(PASSED, "DHCP Server Task Complete");
}

void DHCPTask::executeTask() {
  int packetSize = 0;
  COMM_TAKE;
  packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    // read the packet into packetBuffer
    Udp.read(packetBuffer, DHCP_MESSAGE_SIZE);
    packetSize = DHCPreply((RIP_MSG*) packetBuffer, packetSize, DHCP_MEMORY.ipAddress, domainName);
    Udp.beginPacket(*broadcast, Udp.remotePort());

    Udp.write(packetBuffer, packetSize);

    Udp.endPacket();
  }
  COMM_GIVE;
}