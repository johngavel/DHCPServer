#include "DHCPLite.h"
#include "asciitable/asciitable.h"
#include "dhcpserver.h"

void DHCPServer::addCmd(TerminalCommand* __termCmd) {
  __termCmd->addCmd("time", "[n]", "Configures the lease time.",
                    [this](TerminalLibrary::OutputInterface* terminal) { leaseTime(terminal); });
  __termCmd->addCmd("lease", "", "Displays the leases held in memory.",
                    [this](TerminalLibrary::OutputInterface* terminal) { showLeases(terminal); });
  __termCmd->addCmd("move", "[n] [n]", "Moves the lease from one IP to another.",
                    [this](TerminalLibrary::OutputInterface* terminal) { moveLease(terminal); });
  __termCmd->addCmd("remove", "[n]", "Removes the lease from the list.",
                    [this](TerminalLibrary::OutputInterface* terminal) { removeLease(terminal); });
  __termCmd->addCmd("start", "[n]", "Changes the start octet address",
                    [this](TerminalLibrary::OutputInterface* terminal) { startAddress(terminal); });
  __termCmd->addCmd("num", "[n]", "Restricts the number of leases available.",
                    [this](TerminalLibrary::OutputInterface* terminal) { leaseNum(terminal); });
}

void DHCPServer::reservePins(BackendPinSetup* pinsetup) {
  return;
}

bool DHCPServer::setupTask(OutputInterface* __terminal) {
  broadcast = new IPAddress(broadcastAddress[0], broadcastAddress[1], broadcastAddress[2], broadcastAddress[3]);
  Udp.begin(DHCP_SERVER_PORT);
  setRefreshMilli(10);
  return true;
}

bool DHCPServer::executeTask() {
  int packetSize = 0;
  unsigned char packetBuffer[DHCP_MESSAGE_SIZE];
  packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    // read the packet into packetBuffer
    Udp.read(packetBuffer, DHCP_MESSAGE_SIZE);
    packetSize = DHCPreply((RIP_MSG*) packetBuffer, packetSize, ipAddress, domainName);
    Udp.beginPacket(*broadcast, Udp.remotePort());

    Udp.write(packetBuffer, packetSize);

    Udp.endPacket();
  }
  return true;
}

void DHCPServer::leaseTime(OutputInterface* terminal) {
  char* value;
  value = terminal->readParameter();
  if (value == NULL) {
    terminal->print(INFO, "Lease Time: ");
    terminal->println(INFO, String(memory.mem.leaseTime));
  } else {
    setLeaseTime(atoi(value));
    terminal->println(WARNING, "Changing the lease time requires YOU");
    terminal->println(WARNING, "to reboot everything for the new lease time.");
    setInternal(true);
  }
  terminal->prompt();
}

void DHCPServer::showLeases(OutputInterface* terminal) {
  AsciiTable table(terminal);
  char buffer[20];
  byte ipAddress[4] = {0, 0, 0, 0};
  long current = millis();
  terminal->print(INFO, "Lease Time: ");
  terminal->println(INFO, String(memory.mem.leaseTime));
  terminal->println(INFO, "Availability: " + String(memory.mem.startAddressNumber) + " - " +
                              String(memory.mem.leaseNum + memory.mem.startAddressNumber - 1));

  table.addColumn(Normal, "IpAddress", 17);
  table.addColumn(Green, "MAC Address", 19);
  table.addColumn(Cyan, "Expires(s)", 12);
  table.addColumn(Yellow, "Status", 19);

  table.printHeader();

  String ipaddress = getIPString(ipAddress, buffer, sizeof(buffer));
  String mac = "N/A";
  String expires = "N/A";
  String status = "DHCP Server";
  table.printData(ipaddress, mac, expires, status);

  for (int i = 0; i < memory.mem.leaseNum; i++) {
    if (validLease(i)) {
      getLeaseIPAddress(i, ipAddress);
      ipaddress = getIPString(ipAddress, buffer, sizeof(buffer));
      mac = getMacString(getLeaseMACAddress(i), buffer, sizeof(buffer));
      expires = "";
      if (getLeaseExpired(i, current)) expires += "- ";
      expires += timeString(getLeaseExpiresSec(i, current), buffer, sizeof(buffer));
      status = leaseStatusString(leaseStatus[i].status);
      table.printData(ipaddress, mac, expires, status);
    }
  }
  table.printDone("Lease Table");
  terminal->prompt();
}

void DHCPServer::moveLease(OutputInterface* terminal) {
  bool success = false;
  int from = atoi(terminal->readParameter()) - memory.mem.startAddressNumber;
  int to = atoi(terminal->readParameter()) - memory.mem.startAddressNumber;
  if (validLeaseNumber(from)) {
    if (validLeaseNumber(to)) {
      swapLease(from, to);
      success = true;
      setInternal(true);
    } else
      terminal->println(ERROR, "To index is invalid");
  } else
    terminal->println(ERROR, "From index is invalid");
  terminal->println((success) ? PASSED : FAILED, "Move Lease Complete");
  terminal->prompt();
}

void DHCPServer::removeLease(OutputInterface* terminal) {
  bool success = false;
  String parameter = terminal->readParameter();
  if (parameter == "all") {
    success = true;
    for (int i = 0; i < LEASESNUM; i++) { deleteLease(i); }
  } else {
    int from = parameter.toInt() - memory.mem.startAddressNumber;
    if (validLeaseNumber(from)) {
      if (validLease(from)) {
        deleteLease(from);
        success = true;
        setInternal(true);
      } else
        terminal->println(ERROR, "Lease already empty!");
    } else
      terminal->println(ERROR, "Index is invalid");
  }
  terminal->println((success) ? PASSED : FAILED, "Remove Lease Complete");
  terminal->prompt();
}

void DHCPServer::startAddress(OutputInterface* terminal) {
  bool success = false;
  int address = atoi(terminal->readParameter());
  if ((address > 1) && (address < 255)) {
    if ((memory.mem.leaseNum + address) < 255) {
      memory.mem.startAddressNumber = address;
      success = true;
      setInternal(true);
    }
    terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  }
  terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  terminal->println((success) ? PASSED : FAILED, "Change Start Address Complete");
  terminal->prompt();
}

void DHCPServer::leaseNum(OutputInterface* terminal) {
  bool success = false;
  int number = atoi(terminal->readParameter());
  if ((number >= 0) && (number < LEASESNUM)) {
    if ((number + memory.mem.startAddressNumber) < 255) {
      memory.mem.leaseNum = number;
      success = true;
      setInternal(true);
    }
    terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  }
  terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  terminal->println((success) ? PASSED : FAILED, "Change Number of Leases Available Complete");
  terminal->prompt();
}