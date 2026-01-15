#!/bin/bash

# Source helper scripts
if ! source common.sh 2> /dev/null; then
  echo "Error: common.sh not found. Please ensure it's in the same directory." >&2
  exit 1
fi

# Source helper scripts
if ! source testcommon.sh 2> /dev/null; then
  log_error "testcommon.sh not found. Please ensure it's in the same directory." >&2
  exit 1
fi

BUILD="$1"
CURRENT_DIR="${2:-.}"

case "$BUILD" in
  --clean)
    generate_from_assets.sh -c -n DHCP -i "$CURRENT_DIR"/assets -o "$CURRENT_DIR"/files
    ;;
  --pre)
    generate_from_assets.sh -b -n DHCP -i "$CURRENT_DIR"/assets -o "$CURRENT_DIR"/files
    ;;
  --post) ;;
  --build)
    generate_from_assets.sh -b -n DHCP -i "$CURRENT_DIR"/assets -o "$CURRENT_DIR"/files
    ;;
  --test)
    # run_tests $CURRENT_DIR $DO_SHOW
    exit $?
    ;;
  *)
    log_failed "Invalid Command Argument: $BUILD"
    exit 1
    ;;
esac

exit 0
