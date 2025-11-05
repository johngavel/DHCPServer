#include "dhcpmemory.h"

#include "DHCPLite.h"
#include "leases.h"

#include <Terminal.h>
#include <asciitable/asciitable.h>
#include <export.h>
#include <serialport.h>
#include <stringutils.h>
#include <sys/_types.h>

DHCPMemory dhcpMemory;

static void configure(OutputInterface* terminal);
static void showLeases(OutputInterface* terminal);
static void moveLease(OutputInterface* terminal);
static void removeLease(OutputInterface* terminal);
static void startAddress(OutputInterface* terminal);
static void leaseNum(OutputInterface* terminal);
static void ignoreUnit(OutputInterface* terminal);
static void importMemory(OutputInterface* terminal);
static void exportMemory(OutputInterface* terminal);

void DHCPMemory::updateBroadcast() {
  broadcastAddress[0] = ~memory.mem.subnetMask[0] | memory.mem.ipAddress[0];
  broadcastAddress[1] = ~memory.mem.subnetMask[1] | memory.mem.ipAddress[1];
  broadcastAddress[2] = ~memory.mem.subnetMask[2] | memory.mem.ipAddress[2];
  broadcastAddress[3] = ~memory.mem.subnetMask[3] | memory.mem.ipAddress[3];
}

void DHCPMemory::setup() {
  TERM_CMD->addCmd("config", "...", "Configure Devices \"config ?\" for more", configure);
  TERM_CMD->addCmd("lease", "", "Displays the leases held in memory.", showLeases);
  TERM_CMD->addCmd("move", "[n] [n]", "Moves the lease from one IP to another.", moveLease);
  TERM_CMD->addCmd("remove", "[n]", "Removes the lease from the list.", removeLease);
  TERM_CMD->addCmd("start", "[n]", "Changes the start octet address", startAddress);
  TERM_CMD->addCmd("num", "[n]", "Restricts the number of leases available.", leaseNum);
  TERM_CMD->addCmd("export", "", "Export Configuration to File System.", exportMemory);
  TERM_CMD->addCmd("import", "", "Import Configuration to File System.", importMemory);
  TERM_CMD->addCmd("ignore", "[n] [0|1]", "Ignore a specific unit and never respond.", ignoreUnit);
  updateBroadcast();
}

void DHCPMemory::initMemory() {
  randomSeed(rp2040.hwrand32());
  EEPROM_TAKE;
  memset(memory.memoryArray, 0, sizeof(MemoryStruct));
  memset(leaseStatus, 0, sizeof(leaseStatus));
  memory.mem.leaseTime = 86400;
  memory.mem.macAddress[0] = 0xDE;
  memory.mem.macAddress[1] = 0xAD;
  memory.mem.macAddress[2] = 0xCC;
  memory.mem.macAddress[3] = random(256);
  memory.mem.macAddress[4] = random(256);
  memory.mem.macAddress[5] = random(256);
  memory.mem.ipAddress[0] = 10;
  memory.mem.ipAddress[1] = 10;
  memory.mem.ipAddress[2] = 0;
  memory.mem.ipAddress[3] = 1;
  memory.mem.dnsAddress[0] = 255;
  memory.mem.dnsAddress[1] = 255;
  memory.mem.dnsAddress[2] = 255;
  memory.mem.dnsAddress[3] = 255;
  memory.mem.subnetMask[0] = 255;
  memory.mem.subnetMask[1] = 255;
  memory.mem.subnetMask[2] = 0;
  memory.mem.subnetMask[3] = 0;
  memory.mem.gatewayAddress[0] = 255;
  memory.mem.gatewayAddress[1] = 255;
  memory.mem.gatewayAddress[2] = 255;
  memory.mem.gatewayAddress[3] = 255;
  memory.mem.startAddressNumber = 101;
  memory.mem.leaseNum = 100;
  EEPROM_GIVE;
  EEPROM_FORCE;
  updateBroadcast();
}

void DHCPMemory::printData(OutputInterface* terminal) {
  EEPROM_TAKE;
  terminal->print(INFO, "Lease Time: ");
  terminal->println(INFO, String(memory.mem.leaseTime));
  terminal->print(INFO, "MAC: ");
  terminal->print(INFO, String(memory.mem.macAddress[0], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[1], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[2], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[3], HEX) + ":");
  terminal->print(INFO, String(memory.mem.macAddress[4], HEX) + ":");
  terminal->println(INFO, String(memory.mem.macAddress[5], HEX));
  terminal->print(INFO, "IP Address: ");
  terminal->print(INFO, String(memory.mem.ipAddress[0]) + ".");
  terminal->print(INFO, String(memory.mem.ipAddress[1]) + ".");
  terminal->print(INFO, String(memory.mem.ipAddress[2]) + ".");
  terminal->println(INFO, String(memory.mem.ipAddress[3]));
  terminal->print(INFO, "Broadcast Address: ");
  terminal->print(INFO, String(broadcastAddress[0]) + ".");
  terminal->print(INFO, String(broadcastAddress[1]) + ".");
  terminal->print(INFO, String(broadcastAddress[2]) + ".");
  terminal->println(INFO, String(broadcastAddress[3]));
  terminal->print(INFO, "Subnet Mask: ");
  terminal->print(INFO, String(memory.mem.subnetMask[0]) + ".");
  terminal->print(INFO, String(memory.mem.subnetMask[1]) + ".");
  terminal->print(INFO, String(memory.mem.subnetMask[2]) + ".");
  terminal->println(INFO, String(memory.mem.subnetMask[3]));
  terminal->print(INFO, "Gateway: ");
  terminal->print(INFO, String(memory.mem.gatewayAddress[0]) + ".");
  terminal->print(INFO, String(memory.mem.gatewayAddress[1]) + ".");
  terminal->print(INFO, String(memory.mem.gatewayAddress[2]) + ".");
  terminal->println(INFO, String(memory.mem.gatewayAddress[3]));
  terminal->print(INFO, "DNS Address: ");
  terminal->print(INFO, String(memory.mem.dnsAddress[0]) + ".");
  terminal->print(INFO, String(memory.mem.dnsAddress[1]) + ".");
  terminal->print(INFO, String(memory.mem.dnsAddress[2]) + ".");
  terminal->println(INFO, String(memory.mem.dnsAddress[3]));
  EEPROM_GIVE;
}

unsigned char* DHCPMemory::getData() {
  return memory.memoryArray;
}

unsigned long DHCPMemory::getLength() {
  return sizeof(MemoryStruct);
}

enum ConfigItem { None = 0, LeaseTime, IpAddress, IpDNS, IpSubnet, IpGW };

static void configure(OutputInterface* terminal) {
  char* value;
  ConfigItem item = None;
  unsigned long parameters[4];
  unsigned long count = 0;
  char* stringParameter = NULL;
  bool requiresStringParameter = false;
  bool commandComplete = true;

  value = terminal->readParameter();

  if (value == NULL) {
    terminal->println(WARNING, "Missing any parameters....");
    commandComplete = false;
    item = None;
  } else if (strncmp("lt", value, 2) == 0) {
    item = LeaseTime;
    count = 1;
  } else if (strncmp("ip", value, 2) == 0) {
    item = IpAddress;
    count = 4;
  } else if (strncmp("dns", value, 3) == 0) {
    item = IpDNS;
    count = 4;
  } else if (strncmp("gw", value, 2) == 0) {
    item = IpGW;
    count = 4;
  } else if (strncmp("subnet", value, 6) == 0) {
    item = IpSubnet;
    count = 4;
  } else if ((strncmp("?", value, 1) == 0) || (strncmp("help", value, 1) == 0)) {
    item = None;
  } else {
    terminal->print(INFO, "Invalid Config: <");
    terminal->print(WARNING, value);
    terminal->println(INFO, ">");
    item = None;
    commandComplete = false;
  }
  for (unsigned long i = 0; i < count; i++) {
    value = terminal->readParameter();
    if (value == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      terminal->println(WARNING, "Missing Parameters in config");
      break;
    } else {
      parameters[i] = atoi(value);
    }
  }
  if (requiresStringParameter == true) {
    stringParameter = terminal->readParameter();
    if (stringParameter == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      terminal->println(WARNING, "Missing Parameters in config");
    }
  }
  switch (item) {
  case LeaseTime:
    Lease::setLeaseTime(parameters[0]);
    terminal->println(WARNING, "Changing the lease time requires YOU");
    terminal->println(WARNING, "to reboot everything for the new lease time.");
    EEPROM_FORCE;
    break;
  case IpAddress:
    DHCP_MEMORY.ipAddress[0] = parameters[0];
    DHCP_MEMORY.ipAddress[1] = parameters[1];
    DHCP_MEMORY.ipAddress[2] = parameters[2];
    DHCP_MEMORY.ipAddress[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case IpDNS:
    DHCP_MEMORY.dnsAddress[0] = parameters[0];
    DHCP_MEMORY.dnsAddress[1] = parameters[1];
    DHCP_MEMORY.dnsAddress[2] = parameters[2];
    DHCP_MEMORY.dnsAddress[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case IpSubnet:
    DHCP_MEMORY.subnetMask[0] = parameters[0];
    DHCP_MEMORY.subnetMask[1] = parameters[1];
    DHCP_MEMORY.subnetMask[2] = parameters[2];
    DHCP_MEMORY.subnetMask[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case IpGW:
    DHCP_MEMORY.gatewayAddress[0] = parameters[0];
    DHCP_MEMORY.gatewayAddress[1] = parameters[1];
    DHCP_MEMORY.gatewayAddress[2] = parameters[2];
    DHCP_MEMORY.gatewayAddress[3] = parameters[3];
    EEPROM_FORCE;
    break;
  case None:
  default:
    terminal->println(HELP, "config lt [n]      ", "- sets the lease time in seconds on DHCP Server");
    terminal->println(HELP, "config ip [n] [n] [n] [n]     ", "- Sets the IP address n.n.n.n");
    terminal->println(HELP, "config dns [n] [n] [n] [n]    ", "- Sets the DNS address n.n.n.n");
    terminal->println(HELP, "config gw [n] [n] [n] [n]     ", "- Sets the Gateway address n.n.n.n");
    terminal->println(HELP, "config subnet [n] [n] [n] [n] ", "- Sets the Subnet Mask n.n.n.n");
    terminal->println(HELP, "config help/?          ", "- Print config Help");
    terminal->println();
    terminal->println(HELP, "Note: Addresses use a space separator, so "
                            "\"192.168.168.4\" is \"192 168 168 4\"");
    terminal->println(HELP, "      Must Reboot the system for some changes to take effect");
  }
  dhcpMemory.updateBroadcast();
  terminal->println((commandComplete) ? PASSED : FAILED, "Command Complete");
  terminal->prompt();
}

void showLeases(OutputInterface* terminal) {
  AsciiTable table(terminal);
  byte ipAddress[4] = {0, 0, 0, 0};
  long current = millis();
  terminal->print(INFO, "Lease Time: ");
  terminal->println(INFO, String(DHCP_MEMORY.leaseTime));
  terminal->println(INFO,
                    "Availability: " + String(DHCP_MEMORY.startAddressNumber) + " - " + String(DHCP_MEMORY.leaseNum + DHCP_MEMORY.startAddressNumber - 1));

  table.addColumn(Normal, "IpAddress", 17);
  table.addColumn(Green, "MAC Address", 19);
  table.addColumn(Red, "Ignore", 8);
  table.addColumn(Cyan, "Expires(s)", 12);
  table.addColumn(Yellow, "Status", 19);

  table.printHeader();

  String ipaddress = getIPString(DHCP_MEMORY.ipAddress);
  String mac = getMacString(DHCP_MEMORY.macAddress);
  String ignore = "N/A";
  String expires = "N/A";
  String status = "DHCP Server";
  table.printData(ipaddress, mac, ignore, expires, status);

  for (int i = 0; i < DHCP_MEMORY.leaseNum; i++) {
    if (Lease::validLease(i)) {
      Lease::getLeaseIPAddress(i, ipAddress);
      ipaddress = getIPString(ipAddress);
      mac = getMacString(Lease::getLeaseMACAddress(i));
      ignore = (Lease::ignoreLease(i)) ? "True" : "False";
      expires = "";
      if (Lease::getLeaseExpired(i, current)) expires += "- ";
      expires += timeString(Lease::getLeaseExpiresSec(i, current));
      status = Lease::leaseStatusString(dhcpMemory.leaseStatus[i].status);
      table.printData(ipaddress, mac, ignore, expires, status);
    }
  }
  table.printDone("Lease Table");
  terminal->prompt();
}

void moveLease(OutputInterface* terminal) {
  bool success = false;
  int from = atoi(terminal->readParameter()) - DHCP_MEMORY.startAddressNumber;
  int to = atoi(terminal->readParameter()) - DHCP_MEMORY.startAddressNumber;
  if (Lease::validLeaseNumber(from)) {
    if (Lease::validLeaseNumber(to)) {
      Lease::swapLease(from, to);
      success = true;
    } else
      terminal->println(ERROR, "To index is invalid");
  } else
    terminal->println(ERROR, "From index is invalid");
  terminal->println((success) ? PASSED : FAILED, "Move Lease Complete");
  terminal->prompt();
}

void removeLease(OutputInterface* terminal) {
  bool success = false;
  String parameter = terminal->readParameter();
  if (parameter == "all") {
    success = true;
    for (int i = 0; i < LEASESNUM; i++) { Lease::deleteLease(i); }
  } else {
    int from = parameter.toInt() - DHCP_MEMORY.startAddressNumber;
    if (Lease::validLeaseNumber(from)) {
      if (Lease::validLease(from)) {
        Lease::deleteLease(from);
        success = true;
      } else
        terminal->println(ERROR, "Lease already empty!");
    } else
      terminal->println(ERROR, "Index is invalid");
  }
  terminal->println((success) ? PASSED : FAILED, "Remove Lease Complete");
  terminal->prompt();
}

void startAddress(OutputInterface* terminal) {
  bool success = false;
  int address = atoi(terminal->readParameter());
  if ((address > 1) && (address < 255)) {
    if ((DHCP_MEMORY.leaseNum + address) < 255) {
      DHCP_MEMORY.startAddressNumber = address;
      success = true;
    }
    terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  }
  terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  terminal->println((success) ? PASSED : FAILED, "Change Start Address Complete");
  terminal->prompt();
}

void leaseNum(OutputInterface* terminal) {
  bool success = false;
  int number = atoi(terminal->readParameter());
  if ((number >= 0) && (number < LEASESNUM)) {
    if ((number + DHCP_MEMORY.startAddressNumber) < 255) {
      DHCP_MEMORY.leaseNum = number;
      success = true;
    }
    terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  }
  terminal->println(ERROR, "Address space and leases are restricted in the fourth octet to 1 - 254");
  terminal->println((success) ? PASSED : FAILED, "Change Number of Leases Available Complete");
  terminal->prompt();
}

void exportMemory(OutputInterface* terminal) {
  DHCP_DATA->exportMem();
  terminal->println(PASSED, "Export Complete.");
  terminal->prompt();
}

void DHCPMemory::exportMem() {
  Export exportMem(MEMORY_CONFIG_FILE);
  exportMem.exportData("mac", DHCP_MEMORY.macAddress, 6);
  exportMem.exportData("ip", DHCP_MEMORY.ipAddress, 4);
  exportMem.exportData("startAddressNumber", DHCP_MEMORY.startAddressNumber);
  exportMem.exportData("leaseNum", DHCP_MEMORY.leaseNum);
  exportMem.exportData("leaseTime", DHCP_MEMORY.leaseTime);
  exportMem.exportData("dnsAddress", DHCP_MEMORY.dnsAddress, 4);
  exportMem.exportData("subnetMask", DHCP_MEMORY.subnetMask, 4);
  exportMem.exportData("gatewayAddress", DHCP_MEMORY.gatewayAddress, 4);
  for (int i = 0; i < LEASESNUM; i++) { exportMem.exportData("leasesMac" + String(i), DHCP_MEMORY.leasesMac[i].macAddress, 6); }
  exportMem.close();
}

void importMemory(OutputInterface* terminal) {
  DHCP_DATA->importMem();
  terminal->println(PASSED, "Import Complete.");
  terminal->prompt();
}

void DHCPMemory::importMem() {
  Import importMem(MEMORY_CONFIG_FILE);
  String parameter;
  String value;
  while (importMem.importParameter(&parameter)) {
    if (parameter == "mac")
      importMem.importData(DHCP_MEMORY.macAddress, 6);
    else if (parameter == "ip")
      importMem.importData(DHCP_MEMORY.ipAddress, 4);
    else if (parameter == "startAddressNumber")
      importMem.importData(&DHCP_MEMORY.startAddressNumber);
    else if (parameter == "leaseNum")
      importMem.importData(&DHCP_MEMORY.leaseNum);
    else if (parameter == "leaseTime")
      importMem.importData(&DHCP_MEMORY.leaseTime);
    else if (parameter == "dnsAddress")
      importMem.importData(DHCP_MEMORY.dnsAddress, 4);
    else if (parameter == "subnetMask")
      importMem.importData(DHCP_MEMORY.subnetMask, 4);
    else if (parameter == "gatewayAddress")
      importMem.importData(DHCP_MEMORY.gatewayAddress, 4);
    else if (parameter.startsWith("leasesMac")) {
      int macIndex = parameter.substring(String("leasesMac").length()).toInt();
      importMem.importData(DHCP_MEMORY.leasesMac[macIndex].macAddress, 6);
    } else {
      importMem.importData(&value);
      CONSOLE->println(ERROR, "Unknown Parameter in File: " + parameter + " - " + value);
    }
  }

  EEPROM_FORCE;
}

void ignoreUnit(OutputInterface* terminal) {
  bool success = false;
  int number = atoi(terminal->readParameter()) - DHCP_MEMORY.startAddressNumber;
  int ignore = atoi(terminal->readParameter());
  if ((number >= 0) && (number < DHCP_MEMORY.leaseNum)) {
    Lease::setIgnore(number, (ignore == 1));
    success = true;
  } else {
    terminal->println(ERROR, "Invalid Unit Number");
  }
  terminal->println((success) ? PASSED : FAILED, "Ignore Unit Complete");
  terminal->prompt();
}
