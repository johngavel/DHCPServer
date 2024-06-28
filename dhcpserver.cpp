#include "dhcpserver.h"

#include "DHCPLite.h"
#include "dhcpmemory.h"

#include <commonhtml.h>
#include <serialport.h>
#include <servermodule.h>
#include <stringutils.h>
#include <watchdog.h>

extern DHCPMemory dhcpMemory;

static ErrorPage errorPage;
static CodePage codePage;
static UploadPage uploadPage;
static UpgradePage upgradePage;
static RebootPage rebootPage;
static UpgradeProcessingFilePage upgradeProcessingFilePage;
static UploadProcessingFilePage uploadProcessingFilePage;

void buildLeaseTable(HTMLBuilder* html) {
  long current = millis();
  html->println("Availabilty: " + String(DHCP_MEMORY.startAddressNumber) + " - " + String(DHCP_MEMORY.leaseNum + DHCP_MEMORY.startAddressNumber - 1))->brTag();
  html->openTag("table", "border=\"1\" class=\"center\"")->println();
  html->openTrTag()->println();
  html->thTag("Ignore")->println();
  html->thTag("IP Address")->println();
  html->thTag("MAC Address")->println();
  html->thTag("Expires (s)")->println();
  html->thTag("Status")->println();
  html->closeTag()->println();

  html->openTrTag()->println();
  html->tdTag("")->println();
  html->tdTag(getIPString(DHCP_MEMORY.ipAddress))->println();
  html->tdTag(getMacString(DHCP_MEMORY.macAddress))->println();
  html->tdTag("N/A")->println();
  html->tdTag("DHCP Server")->println();
  html->closeTag()->println();

  for (int i = 0; i < DHCP_MEMORY.leaseNum; i++) {
    byte blank[6] = {0, 0, 0, 0, 0, 0};
    byte ipAddress[4] = {0, 0, 0, 0};
    if (memcmp(DHCP_MEMORY.leasesMac[i].macAddress, blank, 6) != 0) {
      memcpy(ipAddress, DHCP_MEMORY.ipAddress, 4);
      ipAddress[0] = ipAddress[0] & DHCP_MEMORY.subnetMask[0];
      ipAddress[1] = ipAddress[1] & DHCP_MEMORY.subnetMask[1];
      ipAddress[2] = ipAddress[2] & DHCP_MEMORY.subnetMask[2];
      ipAddress[3] = ipAddress[3] & DHCP_MEMORY.subnetMask[3];
      ipAddress[2] += DHCP_MEMORY.ipAddressPop & 0xFF;
      ipAddress[3] += (i + DHCP_MEMORY.startAddressNumber) & 0xFF;
      html->openTrTag()->println();
      html->openTdTag()
          ->closeTag("input", "type=\"checkbox\" id=\"nak" + String(i) + "\" name=\"nak" + String(i) + "\" disabled" +
                                  ((dhcpMemory.leaseStatus[i].ignore) ? " checked " : " "))
          ->closeTag()
          ->println();
      html->tdTag(getIPString(ipAddress))->println();
      html->tdTag(getMacString(DHCP_MEMORY.leasesMac[i].macAddress))->println();
      if (dhcpMemory.leaseStatus[i].expires > current)
        html->tdTag(timeString(((dhcpMemory.leaseStatus[i].expires - current) / 1000)));
      else
        html->tdTag("- " + timeString(((current - dhcpMemory.leaseStatus[i].expires) / 1000)), "bgcolor=\"red\"");
      html->println();
      if (dhcpMemory.leaseStatus[i].status == DHCP_LEASE_AVAIL)
        html->tdTag(leaseStatusString(dhcpMemory.leaseStatus[i].status), "bgcolor=\"red\"");
      else
        html->tdTag(leaseStatusString(dhcpMemory.leaseStatus[i].status));
      html->println();
      html->closeTag()->println();
    }
  }
  html->closeTag()->println();
}

class RootPage : public BasicPage {
public:
  RootPage() {
    setPageName("index");
    refresh = 10;
  };
  void conductAction(){};
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    conductAction();
    sendPageBegin(html, true, refresh);
    html->print("Lease Time: ");
    html->println(timeString(DHCP_MEMORY.leaseTime));
    html->brTag()->print("Increment: ");
    html->println(String(DHCP_MEMORY.ipAddressPop))->brTag();
    buildLeaseTable(html);
    html->openTag("table", "class=\"center\"")->openTrTag()->println();
    html->openTdTag()->openTag("a", "href=\"/dhcpconfig\"")->print("DHCP Server Config")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/server\"")->print("Server Control")->closeTag()->closeTag()->closeTag()->closeTag()->println();
    sendPageEnd(html);
    return html;
  }
  int refresh;
} indexPage;

class ServerPage : public BasicPage {
public:
  ServerPage() { setPageName("server"); };
  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    String versionString = "Ver. " + String(ProgramInfo::MajorVersion) + String(".") + String(ProgramInfo::MinorVersion);

    html->openTag("h2")->print(ProgramInfo::AppName)->closeTag()->println();
    html->openTag("table", "class=\"center\"")->openTrTag();
    html->tdTag(versionString)->closeTag()->println();
    html->openTrTag()->tdTag("Build Date: " + String(ProgramInfo::compileDate) + " Time: " + String(ProgramInfo::compileTime))->closeTag()->println();
    html->openTrTag()->tdTag("Author: " + String(ProgramInfo::AuthorName))->closeTag()->closeTag()->println();
    html->println();

    html->openTag("h2")->print("MAC Address")->closeTag()->println();
    html->openTag("table", "class=\"center\"")->openTrTag();
    html->tdTag("MAC Address:")->tdTag(getMacString(DHCP_MEMORY.macAddress));
    html->closeTag()->closeTag()->println();

    IPAddress ipAddress = ETHERNET->getIPAddress();
    html->openTag("h2")->print("IP Configuration")->closeTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()->tdTag("IP Address:")->tdTag(ipAddress.toString())->closeTag()->println();

    html->openTrTag()->tdTag("IP Broadcast:")->tdTag(getIPString(dhcpMemory.broadcastAddress))->closeTag()->println();

    ipAddress = ETHERNET->getDNS();
    html->openTrTag()->tdTag("DNS Server:")->tdTag(ipAddress.toString())->closeTag()->println();

    ipAddress = ETHERNET->getSubnetMask();
    html->openTrTag()->tdTag("Subnet Mask:")->tdTag(ipAddress.toString())->closeTag()->println();

    ipAddress = ETHERNET->getGateway();
    html->openTrTag()->tdTag("Gateway:")->tdTag(ipAddress.toString())->closeTag()->println();
    html->closeTag()->brTag()->println();

    html->openTag("table", "class=\"center\"");
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/ipconfig\"")->print("Configure IP Addresses")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/upgrade\"")->print("Upgrade the DHCP Server")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/code\"")->print("Source Code of the DHCP Server")->closeTag()->closeTag()->closeTag()->println();
    html->closeTag()->brTag()->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()
        ->openTdTag()
        ->openTag("a", "href=\"/\"")
        ->openTag("button", "type=\"button\" class=\"button2 button\"")
        ->print("Cancel")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->openTrTag()
        ->openTdTag()
        ->openTag("a", "href=\"/reboot\"")
        ->openTag("button", "type=\"button\" class=\"button\"")
        ->print("REBOOT")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    sendPageEnd(html);
    return html;
  }
} serverPage;

static bool moveLease(unsigned long __from, unsigned long __to) {
  unsigned long from = __from - DHCP_MEMORY.startAddressNumber;
  unsigned long to = __to - DHCP_MEMORY.startAddressNumber;
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  bool success = false;
  if (from < DHCP_MEMORY.leaseNum) {
    if (to < DHCP_MEMORY.leaseNum) {
      if (memcmp(blank, DHCP_MEMORY.leasesMac[to].macAddress, 6) == 0) {
        memcpy(DHCP_MEMORY.leasesMac[to].macAddress, DHCP_MEMORY.leasesMac[from].macAddress, 6);
        memcpy(DHCP_MEMORY.leasesMac[from].macAddress, blank, 6);
        dhcpMemory.leaseStatus[to].expires = dhcpMemory.leaseStatus[from].expires;
        dhcpMemory.leaseStatus[from].expires = 0;
        dhcpMemory.leaseStatus[to].status = dhcpMemory.leaseStatus[from].status = DHCP_LEASE_AVAIL;
        success = true;
      }
    }
  }
  return success;
}

static bool removeLease(unsigned long __id) {
  unsigned long from = __id - DHCP_MEMORY.startAddressNumber;
  byte blank[6] = {0, 0, 0, 0, 0, 0};
  bool success = false;
  if (from < DHCP_MEMORY.leaseNum) {
    if (memcmp(blank, DHCP_MEMORY.leasesMac[from].macAddress, 6) != 0) {
      memcpy(DHCP_MEMORY.leasesMac[from].macAddress, blank, 6);
      dhcpMemory.leaseStatus[from].expires = 0;
      dhcpMemory.leaseStatus[from].status = DHCP_LEASE_AVAIL;
      success = true;
    }
  }
  return success;
}

void buildConfigLeaseTable(HTMLBuilder* html) {
  long current = millis();
  html->println("Availabilty: ");
  html->openTag("input",
                "type=\"text\" maxlength=\"6\" size=\"6\" pattern=\"[^\\s]+\" value=\"" + String(DHCP_MEMORY.startAddressNumber) + "\" name=\"startAddress\"\"")
      ->closeTag();
  html->println(" - ");
  html->openTag("input", "type=\"text\" maxlength=\"6\" size=\"6\" pattern=\"[^\\s]+\" value=\"" +
                             String(DHCP_MEMORY.leaseNum + DHCP_MEMORY.startAddressNumber - 1) + "\" name=\"leaseNum\"\"")
      ->closeTag();
  html->brTag();
  html->openTag("table", "border=\"1\" class=\"center\"")->println();
  html->openTrTag()->println();
  html->thTag("Ignore")->println();
  html->thTag("IP Address")->println();
  html->thTag("MAC Address")->println();
  html->thTag("Expires (s)")->println();
  html->thTag("Status")->println();
  html->closeTag()->println();

  html->openTrTag()->println();
  html->tdTag("")->println();
  html->tdTag(getIPString(DHCP_MEMORY.ipAddress))->println();
  html->tdTag(getMacString(DHCP_MEMORY.macAddress))->println();
  html->tdTag("N/A")->println();
  html->tdTag("DHCP Server")->println();
  html->closeTag()->println();

  for (int i = 0; i < DHCP_MEMORY.leaseNum; i++) {
    byte blank[6] = {0, 0, 0, 0, 0, 0};
    byte ipAddress[4] = {0, 0, 0, 0};
    if (memcmp(DHCP_MEMORY.leasesMac[i].macAddress, blank, 6) != 0) {
      memcpy(ipAddress, DHCP_MEMORY.ipAddress, 4);
      ipAddress[0] = ipAddress[0] & DHCP_MEMORY.subnetMask[0];
      ipAddress[1] = ipAddress[1] & DHCP_MEMORY.subnetMask[1];
      ipAddress[2] = ipAddress[2] & DHCP_MEMORY.subnetMask[2];
      ipAddress[3] = ipAddress[3] & DHCP_MEMORY.subnetMask[3];
      ipAddress[2] += DHCP_MEMORY.ipAddressPop & 0xFF;
      ipAddress[3] += (i + DHCP_MEMORY.startAddressNumber) & 0xFF;
      html->openTrTag()->println();
      html->openTdTag()
          ->closeTag("input",
                     "type=\"checkbox\" id=\"nak" + String(i) + "\" name=\"nak" + String(i) + "\"" + ((dhcpMemory.leaseStatus[i].ignore) ? " checked " : " "))
          ->closeTag()
          ->println();
      html->tdTag(getIPString(ipAddress))->println();
      html->tdTag(getMacString(DHCP_MEMORY.leasesMac[i].macAddress))->println();
      if (dhcpMemory.leaseStatus[i].expires > current)
        html->tdTag(timeString(((dhcpMemory.leaseStatus[i].expires - current) / 1000)));
      else
        html->tdTag("- " + timeString(((current - dhcpMemory.leaseStatus[i].expires) / 1000)), "bgcolor=\"red\"");
      html->println();
      if (dhcpMemory.leaseStatus[i].status == DHCP_LEASE_AVAIL)
        html->tdTag(leaseStatusString(dhcpMemory.leaseStatus[i].status), "bgcolor=\"red\"");
      else
        html->tdTag(leaseStatusString(dhcpMemory.leaseStatus[i].status));
      html->println();
      html->closeTag()->println();
    }
  }
  html->closeTag()->println();
}

class ConfigDHCPPage : public ProcessPage {
public:
  ConfigDHCPPage() { setPageName("dhcpconfig"); };

  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    if (parametersProcessed) html->brTag()->print("Configuration Saved")->brTag()->brTag();
    html->openTag("form", "action=\"/dhcpconfig\" method=\"GET\"")->println();
    html->openTag("table", "class=\"center\"");
    html->openTrTag()
        ->tdTag("")
        ->tdTag("Lease Time (s)")
        ->openTdTag()
        ->openTag("input", "type=\"text\" maxlength=\"6\" size=\"6\" pattern=\"[^\\s]+\" value=\"" + String(DHCP_MEMORY.leaseTime) + "\" name=\"leaseTime\"\"")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->openTrTag()
        ->tdTag("")
        ->tdTag("Address Increment")
        ->openTdTag()
        ->openTag("input",
                  "type=\"text\" maxlength=\"1\" size=\"1\" pattern=\"[^\\s]+\" value=\"" + String(DHCP_MEMORY.ipAddressPop) + "\" name=\"increment\"\"")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->openTrTag()
        ->openTdTag()
        ->closeTag("input", "type=\"checkbox\" id=\"move\" name=\"move\"")
        ->openTag("label", "for=\"move\"")
        ->print("Move IP")
        ->closeTag()
        ->closeTag()
        ->tdTag("From:")
        ->openTdTag()
        ->openTag("input",
                  "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(DHCP_MEMORY.startAddressNumber) + "\" name=\"from\"\"")
        ->closeTag()
        ->closeTag()
        ->tdTag("To:")
        ->openTdTag()
        ->openTag("input",
                  "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(DHCP_MEMORY.startAddressNumber) + "\" name=\"to\"\"")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->openTrTag()
        ->openTdTag()
        ->closeTag("input", "type=\"checkbox\" id=\"delete\" name=\"delete\"")
        ->openTag("label", "for=\"delete\"")
        ->print("Delete IP")
        ->closeTag()
        ->closeTag()
        ->tdTag("Delete:")
        ->openTdTag()
        ->openTag("input",
                  "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(DHCP_MEMORY.startAddressNumber) + "\" name=\"deleteid\"\"")
        ->closeTag()
        ->closeTag()
        ->tdTag("")
        ->tdTag("")
        ->closeTag()
        ->println();
    html->openTrTag()
        ->tdTag("Delete All Leases")
        ->tdTag(" enter \"all\" ")
        ->openTdTag()
        ->openTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" value=\"no\" name=\"deleteall\"\"")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    buildConfigLeaseTable(html);
    html->brTag()->brTag();
    html->print("Changing the lease time requires YOU")->brTag();
    html->print("to reboot everything for the new lease time.")->brTag()->brTag()->println();

    html->openTag("table", "class=\"center\"")->openTrTag()->println();
    html->openTdTag()
        ->openTag("button", "type=\"submit\" class=\"button4 button\"")
        ->print("Submit")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("button", "type=\"reset\" class=\"button\"")
        ->print("Reset")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("a", "href=\"/\"")
        ->openTag("button", "type=\"button\" class=\"button2 button\"")
        ->print("Cancel")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    html->closeTag();
    html->closeTag();
    sendPageEnd(html);
    parametersProcessed = false;
    return html;
  };

  void processParameterList() {
    unsigned long from = 0, to = 0, id = 0;
    unsigned long tempStartAddress = 0;
    unsigned long tempLeaseNum = 0;
    bool move = false;
    bool deleteid = false;
    for (int i = 0; i < LEASESNUM; i++) dhcpMemory.leaseStatus[i].ignore = false;
    for (int i = 0; i < list.parameterCount; i++) {
      dhcpMemory.leaseStatus[i].ignore = false;
      if (list.parameters[i].parameter.startsWith("nak")) {
        int nakIndex = list.parameters[i].parameter.substring(3).toInt();
        dhcpMemory.leaseStatus[nakIndex].ignore = true;
      } else if (list.parameters[i].parameter.equals("leaseTime")) {
        DHCP_MEMORY.leaseTime = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("increment")) {
        DHCP_MEMORY.ipAddressPop = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("from")) {
        from = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("to")) {
        to = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("deleteid")) {
        id = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("move")) {
        move = true;
      } else if (list.parameters[i].parameter.equals("delete")) {
        deleteid = true;
      } else if (list.parameters[i].parameter.equals("startAddress")) {
        tempStartAddress = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("leaseNum")) {
        tempLeaseNum = list.parameters[i].value.toInt();
      } else if (list.parameters[i].parameter.equals("deleteall")) {
        if (list.parameters[i].value == "all") {
          memset(DHCP_MEMORY.leasesMac, 0, sizeof(DHCP_MEMORY.leasesMac));
          memset(dhcpMemory.leaseStatus, 0, sizeof(DHCPMemory::leaseStatus));
          PORT->println(WARNING, "Leases table deleted!");
        }
      } else
        PORT->println(ERROR, "Unknown parameter when processing DHCP Configuration Page: " + list.parameters[i].parameter);
    }
    if ((tempStartAddress > 1) && (tempStartAddress < 255))
      if ((tempLeaseNum >= tempStartAddress) && ((tempLeaseNum - tempStartAddress + 1) < LEASESNUM) && (tempLeaseNum < 255)) {
        DHCP_MEMORY.startAddressNumber = tempStartAddress;
        DHCP_MEMORY.leaseNum = tempLeaseNum - tempStartAddress + 1;
      }
    if (move) moveLease(from, to);
    if (deleteid) removeLease(id);
    parametersProcessed = true;
    EEPROM->breakSeal();
  };
} configDHCPPage;

class ConfigIPPage : public ProcessPage {
public:
  ConfigIPPage() { setPageName("ipconfig"); };

  HTMLBuilder* getHtml(HTMLBuilder* html) {
    sendPageBegin(html);
    if (parametersProcessed) html->brTag()->print("Configuration Saved")->brTag()->brTag();
    html->openTag("form", "action=\"/ipconfig\" method=\"GET\"")->println();
    html->openTag("table", "class=\"center\"");

    byte* address = DHCP_MEMORY.ipAddress;
    html->openTrTag()->tdTag("IPAddress");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"ip0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"ip1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"ip2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"ip3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    address = DHCP_MEMORY.subnetMask;
    html->openTrTag()->tdTag("Subnet Mask");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"sm0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"sm1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"sm2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"sm3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    address = DHCP_MEMORY.gatewayAddress;
    html->openTrTag()->tdTag("Gateway Address");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"ga0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"ga1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"ga2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"ga3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    address = DHCP_MEMORY.dnsAddress;
    html->openTrTag()->tdTag("DNS Address");
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[0]) + "\" name=\"da0\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[1]) + "\" name=\"da1\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[2]) + "\" name=\"da2\"\"")
        ->closeTag()
        ->println();
    html->openTdTag()
        ->closeTag("input", "type=\"text\" maxlength=\"3\" size=\"3\" pattern=\"[^\\s]+\" value=\"" + String(address[3]) + "\" name=\"da3\"\"")
        ->closeTag()
        ->println();
    html->closeTag()->println();

    html->openTag("table", "class=\"center\"")->openTrTag()->println();
    html->openTdTag()
        ->openTag("button", "type=\"submit\" class=\"button4 button\"")
        ->print("Submit")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("button", "type=\"reset\" class=\"button\"")
        ->print("Reset")
        ->closeTag()
        ->closeTag()
        ->println()
        ->openTdTag()
        ->openTag("a", "href=\"/\"")
        ->openTag("button", "type=\"button\" class=\"button2 button\"")
        ->print("Cancel")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->closeTag();
    html->closeTag();
    html->closeTag();
    html->closeTag();
    sendPageEnd(html);
    parametersProcessed = false;
    return html;
  }

  void processParameterList() {
    for (int i = 0; i < list.parameterCount; i++) {
      if (list.parameters[i].parameter.equals("ip0"))
        DHCP_MEMORY.ipAddress[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip1"))
        DHCP_MEMORY.ipAddress[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip2"))
        DHCP_MEMORY.ipAddress[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ip3"))
        DHCP_MEMORY.ipAddress[3] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm0"))
        DHCP_MEMORY.subnetMask[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm1"))
        DHCP_MEMORY.subnetMask[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm2"))
        DHCP_MEMORY.subnetMask[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("sm3"))
        DHCP_MEMORY.subnetMask[3] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga0"))
        DHCP_MEMORY.gatewayAddress[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga1"))
        DHCP_MEMORY.gatewayAddress[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga2"))
        DHCP_MEMORY.gatewayAddress[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("ga3"))
        DHCP_MEMORY.gatewayAddress[3] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da0"))
        DHCP_MEMORY.dnsAddress[0] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da1"))
        DHCP_MEMORY.dnsAddress[1] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da2"))
        DHCP_MEMORY.dnsAddress[2] = list.parameters[i].value.toInt();
      else if (list.parameters[i].parameter.equals("da3"))
        DHCP_MEMORY.dnsAddress[3] = list.parameters[i].value.toInt();
      else
        PORT->println(ERROR, "Unknown parameter when processing IP Configuration Page.");
    }
    parametersProcessed = true;
    dhcpMemory.updateBroadcast();
    EEPROM->breakSeal();
  }
} configIPPage;

void setupServerModule() {
  SERVER->setRootPage(&indexPage);
  SERVER->setUpgradePage(&upgradeProcessingFilePage);
  SERVER->setUploadPage(&uploadProcessingFilePage);
  SERVER->setErrorPage(&errorPage);
  SERVER->setPage(&indexPage);
  SERVER->setPage(&serverPage);
  SERVER->setPage(&codePage);
  SERVER->setPage(&upgradePage);
  SERVER->setPage(&uploadPage);
  SERVER->setPage(&rebootPage);
  SERVER->setPage(&configIPPage);
  SERVER->setFormProcessingPage(&configIPPage);
  SERVER->setPage(&configDHCPPage);
  SERVER->setFormProcessingPage(&configDHCPPage);
}
