#!/bin/bash

# Directory to delete
TARGET_DIR="/tmp/grpc_raft/"


# Delete the directory if it exists
if [ -d "$TARGET_DIR" ]; then
  echo "Deleting directory $TARGET_DIR"
  rm -rf "$TARGET_DIR"
else
  echo "Directory $TARGET_DIR does not exist"
fi