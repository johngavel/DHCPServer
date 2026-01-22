#include "dhcpserver.h"

void DHCPServer::configure(unsigned char* __ipAddress, unsigned char* __subnetMask, unsigned char* __macAddress) {
  ipAddress = __ipAddress;
  subnetMask = __subnetMask;
  macAddress = __macAddress;
  updateBroadcast();
};

void DHCPServer::initMemory() {
  memset(memory.memoryArray, 0, sizeof(MemoryStruct));
  memset(leaseStatus, 0, sizeof(leaseStatus));
  memory.mem.leaseTime = 86400;
  memory.mem.startAddressNumber = 101;
  memory.mem.leaseNum = LEASESNUM;
  updateBroadcast();
}

void DHCPServer::printData(OutputInterface* terminal) {
  StringBuilder sb;
  char buffer[20];
  sb = "Broadcast Address: ";
  sb + getIPString(broadcastAddress, buffer, sizeof(buffer));
  terminal->println(INFO, sb.c_str());

  sb = "Start Address: ";
  sb + memory.mem.startAddressNumber;
  terminal->println(INFO, sb.c_str());

  sb = "Configured MAX Leases: ";
  sb + memory.mem.leaseNum;
  terminal->println(INFO, sb.c_str());

  sb = "MAX Leases Possible: ";
  sb + LEASESNUM;
  terminal->println(INFO, sb.c_str());

  sb = "Lease Time: ";
  sb + memory.mem.leaseTime;
  terminal->println(INFO, sb.c_str());
}

JsonDocument DHCPServer::createJson() {
  JsonDocument doc;
  char temp[128];
  byte ipAdd[4] = {0, 0, 0, 0};
  long current = millis();

  doc["leasetime"] = memory.mem.leaseTime;
  doc["startOctet"] = memory.mem.startAddressNumber;
  doc["lastOctet"] = memory.mem.startAddressNumber + memory.mem.leaseNum - 1;
  JsonArray data = doc["dhcptable"].to<JsonArray>();
  {
    JsonObject object = data.add<JsonObject>();
    object["ipAddress"] = getIPString(ipAddress, temp, sizeof(temp));
    object["macAddress"] = getMacString(macAddress, temp, sizeof(temp));
    object["expires"] = "N/A";
    object["stat"] = 2;
    object["status"] = "DHCP Server";
  }
  for (byte i = 0; i < memory.mem.leaseNum; i++) {
    if (validLease(i)) {
      getLeaseIPAddress(i, ipAdd);
      JsonObject object = data.add<JsonObject>();
      object["ipAddress"] = getIPString(ipAdd, temp, sizeof(temp));
      object["macAddress"] = getMacString(getLeaseMACAddress(i), temp, sizeof(temp));
      String expires = "";
      if (getLeaseExpired(i, current)) expires += "- ";
      expires += timeString(getLeaseExpiresSec(i, current), temp, sizeof(temp));
      object["expires"] = expires;
      object["exp"] = getLeaseExpired(i, current);
      object["stat"] = leaseStatus[i].status;
      object["status"] = leaseStatusString(leaseStatus[i].status);
    }
  }
  return doc;
}

bool DHCPServer::parseJson(JsonDocument& doc) {
  if (!doc["leasetime"].isNull()) { memory.mem.leaseTime = doc["leasetime"]; }
  if (!doc["startOctet"].isNull()) {
    byte value = doc["startOctet"];
    if ((value >= 1) && (value <= 255) && ((value + memory.mem.leaseNum) <= 255)) memory.mem.startAddressNumber = value;
  }
  if (!doc["lastOctet"].isNull()) {
    byte value = doc["lastOctet"];
    if ((value >= 1) && (value <= 255)) memory.mem.leaseNum = value - memory.mem.startAddressNumber + 1;
  }
  if (!doc["moveFrom"].isNull() && !doc["moveTo"].isNull()) {
    byte from = doc["moveFrom"];
    byte to = doc["moveTo"];
    from = from - memory.mem.startAddressNumber;
    to = to - memory.mem.startAddressNumber;
    if (validLeaseNumber(from) && validLeaseNumber(to)) { swapLease(from, to); }
  }
  if (!doc["deleteAll"].isNull()) {
    for (int i = 0; i < LEASESNUM; i++) { deleteLease(i); }
  }
  if (!doc["delete"].isNull()) {
    byte value = doc["delete"];
    value = value - memory.mem.startAddressNumber;
    if (validLeaseNumber(value)) deleteLease(value);
  }
  if (!doc["dhcptable"].isNull()) {
    JsonArray table = doc["dhcptable"].as<JsonArray>();
    for (int i = 0; i < LEASESNUM; i++) { deleteLease(i); }
    for (JsonObject item : table) {
      if (!item["ipAddress"].isNull() && !item["macAddress"].isNull()) {
        unsigned char ipBuffer[4];
        unsigned char macBuffer[6];
        const char* ip = item["ipAddress"];
        const char* mac = item["macAddress"];
        parseIPAddress(ip, ipBuffer);
        parseMacString(mac, macBuffer);
        if ((ipBuffer[3] >= memory.mem.startAddressNumber) &&
            (ipBuffer[3] < (memory.mem.startAddressNumber + memory.mem.leaseNum))) {
          unsigned int index = ipBuffer[3] - memory.mem.startAddressNumber;
          setLease(index, macBuffer);
        }
      }
    }
  }
  setInternal(true);
  return true;
}
