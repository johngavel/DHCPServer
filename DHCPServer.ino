#include "dhcpmemory.h"
#include "dhcpserver.h"
#include "dhcptask.h"

#include <architecture.h>
#include <eeprom.h>
#include <ethernetmodule.h>
#include <files.h>
#include <gpio.h>
#include <license.h>
#include <onboardled.h>
#include <serialport.h>
#include <servermodule.h>
#include <startup.h>
#include <telnet.h>
#include <watchdog.h>

const char* ProgramInfo::AppName = "DHCPServer";
const char* ProgramInfo::ShortName = "dhcp";
const unsigned char ProgramInfo::ProgramNumber = 0x08;
const unsigned char ProgramInfo::MajorVersion = 0x01;
const unsigned char ProgramInfo::MinorVersion = 0x05;
const char* ProgramInfo::AuthorName = "John J. Gavel";
const HardwareWire ProgramInfo::hardwarewire = HardwareWire(&Wire, 4, 5);
const HardwareSerialPort ProgramInfo::hardwareserial = HardwareSerialPort(&Serial1, 0, 1);

extern DHCPTask dhcpTask;
extern DHCPMemory dhcpMemory;

void setup() {
  LICENSE;
  setup0Start();

  PORT->setup();
  BLINK->setup();
  FILES->setup();
  EEPROM->configure(I2C_DEVICESIZE_24LC256);
  EEPROM->setData(&dhcpMemory);
  EEPROM->setup();
  ETHERNET->configure(DHCP_MEMORY.macAddress, false, DHCP_MEMORY.ipAddress, DHCP_MEMORY.dnsAddress, DHCP_MEMORY.subnetMask, DHCP_MEMORY.gatewayAddress);
  ETHERNET->setup();
  dhcpTask.setup();
  SERVER->configure(ETHERNET->getServer(HTTP_PORT));
  SERVER->setup();
  setupServerModule();
  TELNET->configure(ETHERNET->getServer(TELNET_PORT));
  TELNET->setup();

  GPIO->setup();
  LICENSE->setup();
  WATCHDOG->setup();
  setup0Complete();
}

void setup1() {
  setup1Start();
  setup1Complete();
}

void loop() {
  PORT->loop();
  ETHERNET->loop();
  dhcpTask.loop();
  SERVER->loop();
  TELNET->loop();
  EEPROM->loop();
  WATCHDOG->loop();
}

void loop1() {
  delay(100);
  BLINK->loop();
  GPIO->loop();
  WATCHDOG->loop();
}
