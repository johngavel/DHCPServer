#include "dhcpserver.h"

#include "DHCPLite.h"
#include "dhcpmemory.h"
#include "leases.h"

#include <commonhtml.h>
#include <serialport.h>
#include <servermodule.h>
#include <stringutils.h>
#include <watchdog.h>

extern DHCPMemory dhcpMemory;

static ErrorPage errorPage;
static CodePage codePage("DHCPServer", "https://github.com/johngavel/DHCPServer");
static UploadPage uploadPage;
static ExportPage exportPage;
static ImportPage importPage;
static UpgradePage upgradePage;
static RebootPage rebootPage;
static UpgradeProcessingFilePage upgradeProcessingFilePage;
static UploadProcessingFilePage uploadProcessingFilePage;
static ImportProcessingFilePage importProcessingFilePage;

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
    byte ipAddress[4] = {0, 0, 0, 0};
    if (Lease::validLease(i)) {
      Lease::getLeaseIPAddress(i, ipAddress);
      html->openTrTag()->println();
      html->openTdTag()
          ->closeTag("input",
                     "type=\"checkbox\" id=\"nak" + String(i) + "\" name=\"nak" + String(i) + "\" disabled" + ((Lease::ignoreLease(i)) ? " checked " : " "))
          ->closeTag()
          ->println();
      html->tdTag(getIPString(ipAddress))->println();
      html->tdTag(getMacString(Lease::getLeaseMACAddress(i)))->println();
      if (!Lease::getLeaseExpired(i, current))
        html->tdTag(timeString(Lease::getLeaseExpiresSec(i, current)));
      else
        html->tdTag("- " + timeString(Lease::getLeaseExpiresSec(i, current)), "bgcolor=\"red\"");
      html->println();
      if (dhcpMemory.leaseStatus[i].status == DHCP_LEASE_AVAIL)
        html->tdTag(Lease::leaseStatusString(Lease::getLeaseStatus(i)), "bgcolor=\"red\"");
      else
        html->tdTag(Lease::leaseStatusString(Lease::getLeaseStatus(i)));
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
    html->println(timeString(Lease::getLeaseTime()));
    html->brTag()->print("Increment: ");
    html->println(String(Lease::Lease::getIncrementPop()))->brTag();
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
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/import\"")->print("Import Server Configuration")->closeTag()->closeTag()->closeTag()->println();
    html->openTrTag()->openTdTag()->openTag("a", "href=\"/export\"")->print("Export Server Configuration")->closeTag()->closeTag()->closeTag()->println();
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
  bool success = false;
  if (Lease::validLeaseNumber(from) && Lease::validLeaseNumber(to)) {
    success = true;
    Lease::swapLease(from, to);
  }
  return success;
}

static bool removeLease(unsigned long __id) {
  unsigned long from = __id - DHCP_MEMORY.startAddressNumber;
  bool success = false;
  if (Lease::validLease(from)) {
    Lease::deleteLease(from);
    success = true;
  }
  return success;
}

void buildConfigLeaseTable(HTMLBuilder* html) {
  long current = millis();
  html->println("Availabilty (Max Range is " + String(LEASESNUM) + "): ");
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
    byte ipAddress[4] = {0, 0, 0, 0};
    if (Lease::validLease(i)) {
      memcpy(ipAddress, DHCP_MEMORY.ipAddress, 4);
      Lease::getLeaseIPAddress(i, ipAddress);
      html->openTrTag()->println();
      html->openTdTag()
          ->closeTag("input", "type=\"checkbox\" id=\"nak" + String(i) + "\" name=\"nak" + String(i) + "\"" + ((Lease::ignoreLease(i)) ? " checked " : " "))
          ->closeTag()
          ->println();
      html->tdTag(getIPString(ipAddress))->println();
      html->tdTag(getMacString(Lease::getLeaseMACAddress(i)))->println();
      if (!Lease::getLeaseExpired(i, current))
        html->tdTag(timeString(Lease::getLeaseExpiresSec(i, current)));
      else
        html->tdTag("- " + timeString(Lease::getLeaseExpiresSec(i, current)), "bgcolor=\"red\"");
      html->println();
      if (Lease::getLeaseStatus(i) == DHCP_LEASE_AVAIL)
        html->tdTag(Lease::leaseStatusString(Lease::getLeaseStatus(i)), "bgcolor=\"red\"");
      else
        html->tdTag(Lease::leaseStatusString(Lease::getLeaseStatus(i)));
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
        ->openTag("input", "type=\"text\" maxlength=\"6\" size=\"6\" pattern=\"[^\\s]+\" value=\"" + String(Lease::getLeaseTime()) + "\" name=\"leaseTime\"\"")
        ->closeTag()
        ->closeTag()
        ->closeTag()
        ->println();
    html->openTrTag()
        ->tdTag("")
        ->tdTag("Address Increment")
        ->openTdTag()
        ->openTag("input",
                  "type=\"text\" maxlength=\"1\" size=\"1\" pattern=\"[^\\s]+\" value=\"" + String(Lease::Lease::getIncrementPop()) + "\" name=\"increment\"\"")
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
    for (int i = 0; i < LEASESNUM; i++) Lease::setIgnore(i, false);
    for (int i = 0; i < LIST->getCount(); i++) {
      Lease::setIgnore(i, false);
      if (LIST->getParameter(i).startsWith("nak")) {
        int nakIndex = LIST->getParameter(i).substring(3).toInt();
        Lease::setIgnore(nakIndex, true);
        ;
      } else if (LIST->getParameter(i).equals("leaseTime")) {
        Lease::setLeaseTime(LIST->getValue(i).toInt());
      } else if (LIST->getParameter(i).equals("increment")) {
        Lease::setIncrementPop(LIST->getValue(i).toInt());
      } else if (LIST->getParameter(i).equals("from")) {
        from = LIST->getValue(i).toInt();
      } else if (LIST->getParameter(i).equals("to")) {
        to = LIST->getValue(i).toInt();
      } else if (LIST->getParameter(i).equals("deleteid")) {
        id = LIST->getValue(i).toInt();
      } else if (LIST->getParameter(i).equals("move")) {
        move = true;
      } else if (LIST->getParameter(i).equals("delete")) {
        deleteid = true;
      } else if (LIST->getParameter(i).equals("startAddress")) {
        tempStartAddress = LIST->getValue(i).toInt();
      } else if (LIST->getParameter(i).equals("leaseNum")) {
        tempLeaseNum = LIST->getValue(i).toInt();
      } else if (LIST->getParameter(i).equals("deleteall")) {
        if (LIST->getValue(i) == "all") {
          memset(DHCP_MEMORY.leasesMac, 0, sizeof(DHCP_MEMORY.leasesMac));
          memset(dhcpMemory.leaseStatus, 0, sizeof(DHCPMemory::leaseStatus));
          PORT->println(WARNING, "Leases table deleted!");
        }
      } else
        PORT->println(ERROR, "Unknown parameter when processing DHCP Configuration Page: " + LIST->getParameter(i));
    }

    // The Range of the Addresses is as follows
    // Availabilty:  {tempStartAddress} - {tempLeaseNum}
    //                     101          -      200
    // Fourth Octet of the IP Address.
    // Convert the above two numbers to:
    //   Start Address, fourth octet of the IP Address.
    //   LeaseNum, the number of leases to give out.
    if ((tempStartAddress > 1) && (tempStartAddress < 255))       // Validate Input
      if ((tempLeaseNum > 1) && (tempLeaseNum < 255))             // Validate Input
        if (tempLeaseNum >= tempStartAddress)                     // End of the Range has to be greater than the beginning.
          if ((tempLeaseNum - tempStartAddress + 1) <= LEASESNUM) // Valid Number of Leases available.
          {
            DHCP_MEMORY.startAddressNumber = tempStartAddress;
            DHCP_MEMORY.leaseNum = tempLeaseNum - tempStartAddress + 1;
          }

    if (move) moveLease(from, to);
    if (deleteid) removeLease(id);
    parametersProcessed = true;
    EEPROM_FORCE;
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
    for (int i = 0; i < LIST->getCount(); i++) {
      if (LIST->getParameter(i).equals("ip0"))
        DHCP_MEMORY.ipAddress[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip1"))
        DHCP_MEMORY.ipAddress[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip2"))
        DHCP_MEMORY.ipAddress[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ip3"))
        DHCP_MEMORY.ipAddress[3] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm0"))
        DHCP_MEMORY.subnetMask[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm1"))
        DHCP_MEMORY.subnetMask[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm2"))
        DHCP_MEMORY.subnetMask[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("sm3"))
        DHCP_MEMORY.subnetMask[3] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga0"))
        DHCP_MEMORY.gatewayAddress[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga1"))
        DHCP_MEMORY.gatewayAddress[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga2"))
        DHCP_MEMORY.gatewayAddress[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("ga3"))
        DHCP_MEMORY.gatewayAddress[3] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da0"))
        DHCP_MEMORY.dnsAddress[0] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da1"))
        DHCP_MEMORY.dnsAddress[1] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da2"))
        DHCP_MEMORY.dnsAddress[2] = LIST->getValue(i).toInt();
      else if (LIST->getParameter(i).equals("da3"))
        DHCP_MEMORY.dnsAddress[3] = LIST->getValue(i).toInt();
      else
        PORT->println(ERROR, "Unknown parameter when processing IP Configuration Page.");
    }
    parametersProcessed = true;
    dhcpMemory.updateBroadcast();
    EEPROM_FORCE;
  }
} configIPPage;

void setupServerModule() {
  SERVER->setRootPage(&indexPage);
  SERVER->setUpgradePage(&upgradeProcessingFilePage);
  SERVER->setUploadPage(&uploadProcessingFilePage);
  SERVER->setUploadPage(&importProcessingFilePage);
  SERVER->setErrorPage(&errorPage);
  SERVER->setPage(&indexPage);
  SERVER->setPage(&serverPage);
  SERVER->setPage(&codePage);
  SERVER->setPage(&upgradePage);
  SERVER->setPage(&uploadPage);
  SERVER->setPage(&exportPage);
  SERVER->setPage(&importPage);
  SERVER->setPage(&rebootPage);
  SERVER->setPage(&configIPPage);
  SERVER->setFormProcessingPage(&configIPPage);
  SERVER->setPage(&configDHCPPage);
  SERVER->setFormProcessingPage(&configDHCPPage);
}
