#!/usr/bin/env python3

import queue
import threading
import socket
import json
import time
import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from geometry_msgs.msg import Twist

class CmdVelSubscriber(Node):
    def __init__(self):
        super().__init__('cmd_vel_subscriber')
        self.subscription = self.create_subscription(Twist, '/cmd_vel', self.listener_callback, 10)
        
        self.msg_queue = queue.Queue()
        
        self.worker_running = True
        self.worker_thread = threading.Thread(target=self.tcp_sender_worker, daemon=True)
        self.worker_thread.start()
        
    def listener_callback(self, msg):
        data = {
            'linear': {'x': msg.linear.x, 'y': msg.linear.y, 'z': msg.linear.z},
            'angular': {'x': msg.angular.x, 'y': msg.angular.y, 'z': msg.angular.z}
        }
        self.msg_queue.put(json.dumps(data))
        
    def tcp_sender_worker(self):
        """ Background thread that handles network I/O with a persistent connection """
        sock = None
        while self.worker_running:
            try:
                json_str = self.msg_queue.get(timeout=0.5)
            except queue.Empty:
                continue

            payload = json_str.encode('utf-8')
            length = len(payload)
            success = False

            for i in range(5):
                try:
                    if sock is None:
                        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        sock.settimeout(3.0)
                        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                        sock.connect(('alice', 8081))
                        self.get_logger().info('Successfully connected/reconnected to Alice.')
                        
                    sock.sendall(length.to_bytes(4, 'big'))
                    sock.sendall(payload)
                    success = True
                    break
                    
                except Exception as e:
                    self.get_logger().warning(f'Network error, retrying ({i + 1}/5): {e}')
                    if sock:
                        try: sock.close(); 
                        except: pass
                    sock = None
                    time.sleep(0.1)
            
            if not success:
                self.get_logger().error('Dropped message: Unable to send to Alice')
            
            self.msg_queue.task_done()
        if sock:
            try: sock.close()
            except: pass

    def destroy_node(self):
        self.worker_running = False
        self.worker_thread.join(timeout=1.0)
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = CmdVelSubscriber()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    except Exception as e:
        print(f'Error: {e}')
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()