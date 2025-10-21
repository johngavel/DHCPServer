#!/bin/bash

# Source helper scripts
if ! source common.sh 2> /dev/null; then
  echo "Error: common.sh not found. Please ensure it's in the same directory." >&2
  exit 1
fi

BUILD="$1"
CURRENT_DIR="$2"

build() {
  createfileheader.sh "$CURRENT_DIR"/assets/favicon_blue.ico "$CURRENT_DIR"/favicon.h favicon
}

case "$BUILD" in
  --clean)
    Delete "$CURRENT_DIR"/favicon.h
    ;;
  --pre)
    build
    ;;
  --post) ;;
  --build)
    build
    ;;
  *)
    log_failed "Invalid Command Argument: $BUILD"
    exit 1
    ;;
esac

exit 0
