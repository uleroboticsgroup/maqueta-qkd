#!/bin/bash
set -e
source /opt/ros/humble/setup.bash

ros2 run turtlesim turtlesim_node &
TURTLE_PID=$!

python3 rosbag_subscriber.py &
SUB_PID=$!

cleanup() {
  python3 -c "
import socket
import struct
try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('alice', 8081))
    data = b'{\"end\":true}'
    sock.sendall(struct.pack('!I', len(data)))
    sock.sendall(data)
    sock.close()
except Exception as e:
    pass
  "
  kill -TERM "$SUB_PID" 2>/dev/null || true
  kill -TERM "$TURTLE_PID" 2>/dev/null || true
  wait "$SUB_PID" 2>/dev/null || true
  wait "$TURTLE_PID" 2>/dev/null || true
}
trap cleanup EXIT

ros2 bag play rosbag2.db3

echo "Bag playback finished, shutting down."