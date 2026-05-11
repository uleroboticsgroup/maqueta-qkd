#!/usr/bin/env python3

import signal
import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from geometry_msgs.msg import Twist
import socket
import json
import time

class CmdVelSubscriber(Node):
    def __init__(self):
        super().__init__('cmd_vel_subscriber')
        self.subscription = self.create_subscription(Twist,'/turtle1/cmd_vel',self.listener_callback,10)
        self.subscription
        
    def listener_callback(self, msg):
        data = {
            'linear': {
                'x': msg.linear.x,
                'y': msg.linear.y,
                'z': msg.linear.z
            },
            'angular': {
                'x': msg.angular.x,
                'y': msg.angular.y,
                'z': msg.angular.z
            }
        }
        json_str = json.dumps(data)

        for i in range(10):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2)
                sock.connect(('alice', 8081))
                payload = json_str.encode('utf-8')
                length = len(payload)
                sock.sendall(length.to_bytes(4, 'big'))
                sock.sendall(payload)
                sock.close()
                break
            except Exception as e:
                self.get_logger().warning(f'Failed to send to Alice ({i + 1}/10): {e}')
                time.sleep(1)
        else:
            self.get_logger().error('Unable to connect to Alice')

def main(args=None):
    rclpy.init(args=args)
    node = CmdVelSubscriber()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Keyboard Interrupt')
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()