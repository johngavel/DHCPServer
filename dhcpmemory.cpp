#include "dhcpmemory.h"

#include "DHCPLite.h"

#include <serialport.h>
#include <stringutils.h>
#include <sys/_types.h>

DHCPMemory dhcpMemory;

static void configure();
static void increaseCount();
static void showLeases();
static void moveLease();
static void removeLease();
static void startAddress();
static void leaseNum();

void DHCPMemory::updateBroadcast() {
  broadcastAddress[0] = ~memory.mem.subnetMask[0] | memory.mem.ipAddress[0];
  broadcastAddress[1] = ~memory.mem.subnetMask[1] | memory.mem.ipAddress[1];
  broadcastAddress[2] = ~memory.mem.subnetMask[2] | memory.mem.ipAddress[2];
  broadcastAddress[3] = ~memory.mem.subnetMask[3] | memory.mem.ipAddress[3];
}

void DHCPMemory::setup() {
  PORT->addCmd("config", "...", "Configure Devices \"config ?\" for more", configure);
  PORT->addCmd("pop", "[n]", "Increase the IP Addresses given by n. n = 0 - 9", increaseCount);
  PORT->addCmd("lease", "", "Displays the leases held in memory.", showLeases);
  PORT->addCmd("move", "[n] [n]", "Moves the lease from one IP to another.", moveLease);
  PORT->addCmd("remove", "[n]", "Removes the lease from the list.", removeLease);
  PORT->addCmd("start", "[n]", "Changes the start octect address", startAddress);
  PORT->addCmd("num", "[n]", "Restricts the number of leases available.", leaseNum);
  updateBroadcast();
}

void DHCPMemory::initMemory() {
  randomSeed(rp2040.hwrand32());
  EEPROM_TAKE;
  EEPROM->breakSeal();
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
  updateBroadcast();
}

void DHCPMemory::printData() {
  EEPROM_TAKE;
  PORT->print(INFO, "Lease Time: ");
  PORT->println(INFO, String(memory.mem.leaseTime));
  PORT->print(INFO, "Increment: ");
  PORT->println(INFO, String(memory.mem.ipAddressPop));
  PORT->print(INFO, "MAC: ");
  PORT->print(INFO, String(memory.mem.macAddress[0], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[1], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[2], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[3], HEX) + ":");
  PORT->print(INFO, String(memory.mem.macAddress[4], HEX) + ":");
  PORT->println(INFO, String(memory.mem.macAddress[5], HEX));
  PORT->print(INFO, "IP Address: ");
  PORT->print(INFO, String(memory.mem.ipAddress[0]) + ".");
  PORT->print(INFO, String(memory.mem.ipAddress[1]) + ".");
  PORT->print(INFO, String(memory.mem.ipAddress[2]) + ".");
  PORT->println(INFO, String(memory.mem.ipAddress[3]));
  PORT->print(INFO, "Broadcast Address: ");
  PORT->print(INFO, String(broadcastAddress[0]) + ".");
  PORT->print(INFO, String(broadcastAddress[1]) + ".");
  PORT->print(INFO, String(broadcastAddress[2]) + ".");
  PORT->println(INFO, String(broadcastAddress[3]));
  PORT->print(INFO, "Subnet Mask: ");
  PORT->print(INFO, String(memory.mem.subnetMask[0]) + ".");
  PORT->print(INFO, String(memory.mem.subnetMask[1]) + ".");
  PORT->print(INFO, String(memory.mem.subnetMask[2]) + ".");
  PORT->println(INFO, String(memory.mem.subnetMask[3]));
  PORT->print(INFO, "Gateway: ");
  PORT->print(INFO, String(memory.mem.gatewayAddress[0]) + ".");
  PORT->print(INFO, String(memory.mem.gatewayAddress[1]) + ".");
  PORT->print(INFO, String(memory.mem.gatewayAddress[2]) + ".");
  PORT->println(INFO, String(memory.mem.gatewayAddress[3]));
  PORT->print(INFO, "DNS Address: ");
  PORT->print(INFO, String(memory.mem.dnsAddress[0]) + ".");
  PORT->print(INFO, String(memory.mem.dnsAddress[1]) + ".");
  PORT->print(INFO, String(memory.mem.dnsAddress[2]) + ".");
  PORT->println(INFO, String(memory.mem.dnsAddress[3]));
  EEPROM_GIVE;
}

unsigned char* DHCPMemory::getData() {
  return memory.memoryArray;
}

unsigned long DHCPMemory::getLength() {
  return sizeof(MemoryStruct);
}

enum ConfigItem { None = 0, LeaseTime, IpAddress, IpDNS, IpSubnet, IpGW };

static void configure() {
  char* value;
  ConfigItem item = None;
  unsigned long parameters[4];
  unsigned long count = 0;
  char* stringParameter = NULL;
  bool requiresStringParameter = false;
  bool commandComplete = true;

  PORT->println();
  value = PORT->readParameter();

  if (value == NULL) {
    PORT->println(WARNING, "Missing any parameters....");
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
    PORT->print(WARNING, "Invalid Config: <");
    PORT->print(WARNING, value);
    PORT->println(WARNING, ">");
    item = None;
    commandComplete = false;
  }
  for (unsigned long i = 0; i < count; i++) {
    value = PORT->readParameter();
    if (value == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      PORT->println(WARNING, "Missing Parameters in config");
      break;
    } else {
      parameters[i] = atoi(value);
    }
  }
  if (requiresStringParameter == true) {
    stringParameter = PORT->readParameter();
    if (stringParameter == NULL) {
      item = None;
      count = 0;
      requiresStringParameter = false;
      commandComplete = false;
      PORT->println(WARNING, "Missing Parameters in config");
    }
  }
  switch (item) {
  case LeaseTime:
    DHCP_MEMORY.leaseTime = parameters[0];
    PORT->println(WARNING, "Changing the lease time requires YOU");
    PORT->println(WARNING, "to reboot everything for the new lease time.");
    EEPROM->breakSeal();
    break;
  case IpAddress:
    DHCP_MEMORY.ipAddress[0] = parameters[0];
    DHCP_MEMORY.ipAddress[1] = parameters[1];
    DHCP_MEMORY.ipAddress[2] = parameters[2];
    DHCP_MEMORY.ipAddress[3] = parameters[3];
    EEPROM->breakSeal();
    break;
  case IpDNS:
    DHCP_MEMORY.dnsAddress[0] = parameters[0];
    DHCP_MEMORY.dnsAddress[1] = parameters[1];
    DHCP_MEMORY.dnsAddress[2] = parameters[2];
    DHCP_MEMORY.dnsAddress[3] = parameters[3];
    EEPROM->breakSeal();
    break;
  case IpSubnet:
    DHCP_MEMORY.subnetMask[0] = parameters[0];
    DHCP_MEMORY.subnetMask[1] = parameters[1];
    DHCP_MEMORY.subnetMask[2] = parameters[2];
    DHCP_MEMORY.subnetMask[3] = parameters[3];
    EEPROM->breakSeal();
    break;
  case IpGW:
    DHCP_MEMORY.gatewayAddress[0] = parameters[0];
    DHCP_MEMORY.gatewayAddress[1] = parameters[1];
    DHCP_MEMORY.gatewayAddress[2] = parameters[2];
    DHCP_MEMORY.gatewayAddress[3] = parameters[3];
    EEPROM->breakSeal();
    break;
  case None:
  default:
    PORT->println(HELP, "config lt [n]      ", "- sets the lease time in seconds on DHCP Server");
    PORT->println(HELP, "config ip [n] [n] [n] [n]     ", "- Sets the IP address n.n.n.n");
    PORT->println(HELP, "config bc [n] [n] [n] [n]     ", "- Sets the Broadcast address n.n.n.n");
    PORT->println(HELP, "config dns [n] [n] [n] [n]    ", "- Sets the DNS address n.n.n.n");
    PORT->println(HELP, "config gw [n] [n] [n] [n]     ", "- Sets the Gateway address n.n.n.n");
    PORT->println(HELP, "config subnet [n] [n] [n] [n] ", "- Sets the Subnet Mask n.n.n.n");
    PORT->println(HELP, "config help/?          ", "- Print config Help");
    PORT->println();
    PORT->println(HELP, "Note: Addresses use a space seperator, so "
                        "\"192.168.168.4\" is \"192 168 168 4\"");
    PORT->println(HELP, "      Must Reboot the system for some changes to take effect");
  }
  dhcpMemory.updateBroadcast();
  PORT->println((commandComplete) ? PASSED : FAILED, "Command Complete");
  PORT->prompt();
}

void increaseCount() {
  int value = 0;
  PORT->println();
  value = atoi(PORT->readParameter());
  if ((value >= 0) && (value <= 9)) {
    DHCP_MEMORY.ipAddressPop = value;
    EEPROM->breakSeal();
  }
  for (int i = 0; i < DHCP_MEMORY.leaseNum; i++) { dhcpMemory.leaseStatus[i].status = DHCP_LEASE_AVAIL; }
  PORT->prompt();
}

String leaseStatusString(long status) {
  String string;
  switch (status) {
  case DHCP_LEASE_AVAIL: string = "DHCP_LEASE_AVAIL"; break;
  case DHCP_LEASE_OFFER: string = "DHCP_LEASE_OFFER"; break;
  case DHCP_LEASE_ACK: string = "DHCP_LEASE_ACK"; break;

  default: string = "UNKNOWN"; break;
  }
  return string;
}

void showLeases() {
  byte ipAddress[4] = {0, 0, 0, 0};
  long current = millis();
  PORT->println();
  PORT->print(INFO, "Lease Time: ");
  PORT->println(INFO, String(DHCP_MEMORY.leaseTime));
  PORT->print(INFO, "Increment: ");
  PORT->println(INFO, String(DHCP_MEMORY.ipAddressPop));
  PORT->println(INFO, "Availabilty: " + String(DHCP_MEMORY.startAddressNumber) + " - " + String(DHCP_MEMORY.leaseNum + DHCP_MEMORY.startAddressNumber - 1));

  PORT->println();
  PORT->println(PROMPT, "IP Address      | MAC Address       | Expires (s) | Status           ");
  PORT->println(PROMPT, "----------------|-------------------|-------------|------------------");

  String ipaddress = String(DHCP_MEMORY.ipAddress[0]) + "." + String(DHCP_MEMORY.ipAddress[1]) + "." + String(DHCP_MEMORY.ipAddress[2]) + "." +
                     String(DHCP_MEMORY.ipAddress[3]);
  String mac = " " + getMacString(DHCP_MEMORY.macAddress);
  String indentifier = ipaddress + tab(ipaddress.length(), 16) + "|" + mac + tab(mac.length(), 19) + "|";
  String expires = " N/A";
  String status = " DHCP Server";
  String data = expires + tab(expires.length(), 13) + "|" + status;
  PORT->println(HELP, indentifier, data);

  for (int i = 0; i < DHCP_MEMORY.leaseNum; i++) {
    byte blank[6] = {0, 0, 0, 0, 0, 0};
    if (memcmp(DHCP_MEMORY.leasesMac[i].macAddress, blank, 6) != 0) {
      memcpy(ipAddress, DHCP_MEMORY.ipAddress, 4);
      ipAddress[0] = ipAddress[0] & DHCP_MEMORY.subnetMask[0];
      ipAddress[1] = ipAddress[1] & DHCP_MEMORY.subnetMask[1];
      ipAddress[2] = ipAddress[2] & DHCP_MEMORY.subnetMask[2];
      ipAddress[3] = ipAddress[3] & DHCP_MEMORY.subnetMask[3];
      ipaddress = String(ipAddress[0]) + "." + String(ipAddress[1]) + "." + String(ipAddress[2] + DHCP_MEMORY.ipAddressPop) + "." +
                  String(ipAddress[3] + i + DHCP_MEMORY.startAddressNumber);
      mac = " " + getMacString(DHCP_MEMORY.leasesMac[i].macAddress);
      indentifier = ipaddress + tab(ipaddress.length(), 16) + "|" + mac + tab(mac.length(), 19) + "|";
      expires = " ";
      if (dhcpMemory.leaseStatus[i].expires > current)
        expires += timeString(((dhcpMemory.leaseStatus[i].expires - current) / 1000));
      else
        expires += "- " + timeString(((current - dhcpMemory.leaseStatus[i].expires) / 1000));
      status = " " + leaseStatusString(dhcpMemory.leaseStatus[i].status);
      data = expires + tab(expires.length(), 13) + "|" + status;
      PORT->println(HELP, indentifier, data);
    }
  }
  PORT->println();
  PORT->prompt();
}

void moveLease() {
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  bool success = false;
  PORT->println();
  int from = atoi(PORT->readParameter()) - DHCP_MEMORY.startAddressNumber;
  int to = atoi(PORT->readParameter()) - DHCP_MEMORY.startAddressNumber;
  if ((from >= 0) && (from < DHCP_MEMORY.leaseNum)) {
    if ((to >= 0) && (to < DHCP_MEMORY.leaseNum)) {
      if (memcmp(blank, DHCP_MEMORY.leasesMac[to].macAddress, 6) == 0) {
        memcpy(DHCP_MEMORY.leasesMac[to].macAddress, DHCP_MEMORY.leasesMac[from].macAddress, 6);
        memcpy(DHCP_MEMORY.leasesMac[from].macAddress, blank, 6);
        dhcpMemory.leaseStatus[to].expires = dhcpMemory.leaseStatus[from].expires;
        dhcpMemory.leaseStatus[from].expires = 0;
        dhcpMemory.leaseStatus[to].status = dhcpMemory.leaseStatus[from].status = DHCP_LEASE_AVAIL;
        success = true;
      } else
        PORT->println(ERROR, "Lease already occupied!");
    } else
      PORT->println(ERROR, "To index is invalid");
  } else
    PORT->println(ERROR, "From index is invalid");
  PORT->println((success) ? PASSED : FAILED, "Move Lease Complete");
  PORT->prompt();
}

void removeLease() {
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  bool success = false;
  PORT->println();
  String parameter = PORT->readParameter();
  if (parameter == "all") {
    success = true;
    for (int i = 0; i < LEASESNUM; i++) {
      memset(DHCP_MEMORY.leasesMac, 0, sizeof(DHCP_MEMORY.leasesMac));
      memset(dhcpMemory.leaseStatus, 0, sizeof(DHCPMemory::leaseStatus));
    }
  } else {
    int from = parameter.toInt() - DHCP_MEMORY.startAddressNumber;
    if ((from >= 0) && (from < DHCP_MEMORY.leaseNum)) {
      if (memcmp(blank, DHCP_MEMORY.leasesMac[from].macAddress, 6) != 0) {
        memcpy(DHCP_MEMORY.leasesMac[from].macAddress, blank, 6);
        dhcpMemory.leaseStatus[from].expires = 0;
        dhcpMemory.leaseStatus[from].status = DHCP_LEASE_AVAIL;
        success = true;
      } else
        PORT->println(ERROR, "Lease already empty!");
    } else
      PORT->println(ERROR, "Index is invalid");
  }
  PORT->println((success) ? PASSED : FAILED, "Remove Lease Complete");
  PORT->prompt();
}

void startAddress() {
  bool success = false;
  int address = atoi(PORT->readParameter());
  if ((address > 1) && (address < 255)) {
    if ((DHCP_MEMORY.leaseNum + address) < 255) {
      DHCP_MEMORY.startAddressNumber = address;
      success = true;
    }
    PORT->println(ERROR, "Address space and leases are restricted in the fourth octect to 1 - 254");
  }
  PORT->println(ERROR, "Address space and leases are restricted in the fourth octect to 1 - 254");
  PORT->println();
  PORT->println((success) ? PASSED : FAILED, "Change Start Adderess Complete");
  PORT->prompt();
}

void leaseNum() {
  bool success = false;
  int number = atoi(PORT->readParameter());
  if ((number >= 0) && (number < LEASESNUM)) {
    if ((number + DHCP_MEMORY.startAddressNumber) < 255) {
      DHCP_MEMORY.leaseNum = number;
      success = true;
    }
    PORT->println(ERROR, "Address space and leases are restricted in the fourth octect to 1 - 254");
  }
  PORT->println(ERROR, "Address space and leases are restricted in the fourth octect to 1 - 254");
  PORT->println();
  PORT->println((success) ? PASSED : FAILED, "Change Number of Leases Available Complete");
  PORT->prompt();
}
