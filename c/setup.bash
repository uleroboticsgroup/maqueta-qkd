#!/bin/bash
set -e
source /opt/ros/humble/setup.bash
set +e

export QT_QPA_PLATFORM=offscreen
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
  kill -TERM "$ECHO_PID" 2>/dev/null || true
  wait "$SUB_PID" 2>/dev/null || true
  wait "$ECHO_PID" 2>/dev/null || true
}
trap cleanup EXIT

sleep 5

ros2 bag play /workspace/carry_try_1
BAG_EXIT=$?

echo "Bag playback finished (exit code: $BAG_EXIT), shutting down."

sleep 5

echo "Script completed, exit."