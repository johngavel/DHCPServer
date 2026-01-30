#include "dhcpserver.h"
#include "files/webpage_all.h"

#include <GavelEEProm.h>
#include <GavelEthernet.h>
#include <GavelI2CWire.h>
#include <GavelPico.h>
#include <GavelSPIWire.h>
#include <GavelServer.h>
#include <GavelServerStandard.h>
#include <GavelTelnet.h>

const char* ProgramInfo::AppName = "DHCP Server";
const char* ProgramInfo::ShortName = "dhcp";
const unsigned char ProgramInfo::ProgramNumber = 0x08;
const unsigned char ProgramInfo::MajorVersion = 0x02;
const unsigned char ProgramInfo::MinorVersion = 0x00;
const char* ProgramInfo::AuthorName = "John J. Gavel";

EthernetModule ethernetModule;
EEpromMemory memory;
TelnetModule telnet;
ServerModule server;
DHCPServer dhcpServer;

void setupDHCPServer() {
  ArrayDirectory* dir;
  dhcpServer.configure(ethernetModule.getIPAddressByte(), ethernetModule.getSubnetMaskByte(),
                       ethernetModule.getMACAddress());
  memory.setData(&dhcpServer);
  taskManager.add(&dhcpServer);
  dir = static_cast<ArrayDirectory*>(fileSystem.open("/www"));
  dir->addFile(new StaticFile(dhcpconfightml_string, dhcpconfightml, dhcpconfightml_len));
  dir = static_cast<ArrayDirectory*>(fileSystem.open("/www/api"));
  dir->addFile(new JsonFile(&dhcpServer, "dhcp-info.json", READ_WRITE, JsonFile::LARGE_BUFFER_SIZE));
  dir = static_cast<ArrayDirectory*>(fileSystem.open("/www/js"));
  dir->addFile(new StaticFile(dhcptablejs_string, dhcptablejs, dhcptablejs_len));
  dir->addFile(new StaticFile(dhcpconfigjs_string, dhcpconfigjs, dhcpconfigjs_len));
  license.addLibrary("DHCP Lite", "v0.14", "LICENSE", "https://github.com/pkulchenko/DHCPLite/tree/master");
}

// ---------- Setup / Loop ----------
void setup() {
  setup0Start(TERM_CMD);

  setup0SerialPort(0, 1);
  i2cWire.begin(4, 5);
  spiWire.begin(18, 19, 16, 17);

  ethernetModule.configure();
  ethernetModule.allowDHCP(false);
  license.addLibrary(ETHERNET_INDEX);

  memory.configure(I2C_DEVICESIZE_24LC256);
  pico.rebootCallBacks.addCallback([&]() { (void) memory.forceWrite(); });

  memory.setData(&programMem);
  memory.setData(ethernetModule.getMemory());

  StringBuilder sb = ProgramInfo::ShortName;
  sb + ":\\> ";
  telnet.configure(ethernetModule.getServer(TELNET_PORT), sb.c_str(), banner);

  loadServerStandard(&programMem, &hardwareList, &license, ethernetModule.getMemory(), &server, &fileSystem,
                     &taskManager, &memory, &pico);
  ArrayDirectory* dir = static_cast<ArrayDirectory*>(fileSystem.open("/www"));
  dir->addFile(new StaticFile("favicon.ico", faviconblueico, faviconblueico_len));
  dir->addFile(new StaticFile(indexhtml_string, indexhtml, indexhtml_len));

  server.configure(ethernetModule.getServer(HTTP_PORT), &fileSystem);

  hardwareList.add(&memory);
  hardwareList.add(&ethernetModule);

  taskManager.add(&memory);
  taskManager.add(&ethernetModule);
  taskManager.add(&server);
  taskManager.add(&telnet);

  gpioManager.setCore(1);
  serialPort.setCore(1);
  blink.setCore(1);
  fileSystem.setCore(1);
  license.setCore(1);
  memory.setCore(1);

  setupDHCPServer();

  setup0Complete();
}

void setup1() {
  setup1Start();
  setup1Complete();
}

void loop() {
  loop_0();
}

void loop1() {
  loop_1();
}