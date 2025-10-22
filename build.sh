#!/bin/bash

# Source helper scripts
if ! source common.sh 2> /dev/null; then
  echo "Error: common.sh not found. Please ensure it's in the same directory." >&2
  exit 1
fi

BUILD="$1"
CURRENT_DIR="$2"

case "$BUILD" in
  --clean)
    Delete "$CURRENT_DIR"/favicon.h
    Delete "$CURRENT_DIR"/dhcplite_license.h
    create_libraries_used.sh --clean $HOME_DIR/libraries/GavelLicense

    ;;
  --pre)
    createfileheader.sh "$CURRENT_DIR"/assets/DHCPLite_LICENSE.txt "$CURRENT_DIR"/dhcplite_license.h DHCPLite
    createfileheader.sh "$CURRENT_DIR"/assets/favicon_blue.ico "$CURRENT_DIR"/favicon.h favicon
    create_libraries_used.sh --build $HOME_DIR/libraries/GavelLicense ETHERNET_USED TERMINAL_USED TCA9555_USED I2C_EEPROM_USED
    ;;
  --post)
    create_libraries_used.sh --clean $HOME_DIR/libraries/GavelLicense
    ;;
  --build)
    createfileheader.sh "$CURRENT_DIR"/assets/DHCPLite_LICENSE.txt "$CURRENT_DIR"/dhcplite_license.h DHCPLite
    createfileheader.sh "$CURRENT_DIR"/assets/favicon_blue.ico "$CURRENT_DIR"/favicon.h favicon
    create_libraries_used.sh --build $HOME_DIR/libraries/GavelLicense ETHERNET_USED TERMINAL_USED TCA9555_USED I2C_EEPROM_USED
    ;;
  *)
    log_failed "Invalid Command Argument: $BUILD"
    exit 1
    ;;
esac

exit 0
