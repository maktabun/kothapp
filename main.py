#!/usr/bin/env python3
import socket
import threading
import sys
import signal
import os

BUFFER_SIZE = 1024
MAX_MESSAGE_SIZE = 512

# Global socket for cleanup
sock = None

def receive_messages(connection):
    """Thread function to handle receiving messages"""
    try:
        while True:
            message = connection.recv(BUFFER_SIZE).decode('utf-8')
            if not message:
                print("\n[Connection lost or peer disconnected]")
                connection.close()
                os._exit(0)
            
            print(f"Peer: {message.rstrip()}")
            sys.stdout.flush()
    except Exception as e:
        print(f"\n[Error receiving message: {e}]")
        connection.close()
        os._exit(0)

def signal_handler(signum, frame):
    """Signal handler for clean exit"""
    print("\n[Exiting...]")
    if sock:
        sock.close()
    os._exit(0)

def run_server(port):
    """Server mode: listen for incoming connection"""
    try:
        server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        server_sock.bind(('', port))
        server_sock.listen(1)
        
        print(f"Server listening on port {port}...")
        print("Waiting for client to connect...")
        
        client_sock, client_addr = server_sock.accept()
        print(f"Client connected from {client_addr[0]}:{client_addr[1]}")
        print("Chat started! Type messages and press Enter.")
        print("Press Ctrl+C to exit.\n")
        
        server_sock.close()  # No longer need the listening socket
        return client_sock
        
    except Exception as e:
        print(f"Server error: {e}")
        return None

def run_client(server_ip, port):
    """Client mode: connect to server"""
    try:
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        print(f"Connecting to {server_ip}:{port}...")
        client_sock.connect((server_ip, port))
        
        print("Connected to server!")
        print("Chat started! Type messages and press Enter.")
        print("Press Ctrl+C to exit.\n")
        
        return client_sock
        
    except Exception as e:
        print(f"Connection error: {e}")
        return None

def main():
    global sock
    
    # Set up signal handlers for clean exit
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Check command line arguments
    if len(sys.argv) < 3:
        print("Usage:")
        print(f"  Server mode: {sys.argv[0]} server <port>")
        print(f"  Client mode: {sys.argv[0]} client <server_ip> <port>")
        return 1
    
    # Determine mode and establish connection
    if sys.argv[1] == "server":
        if len(sys.argv) != 3:
            print(f"Server usage: {sys.argv[0]} server <port>")
            return 1
        try:
            port = int(sys.argv[2])
            sock = run_server(port)
        except ValueError:
            print("Invalid port number")
            return 1
            
    elif sys.argv[1] == "client":
        if len(sys.argv) != 4:
            print(f"Client usage: {sys.argv[0]} client <server_ip> <port>")
            return 1
        try:
            server_ip = sys.argv[2]
            port = int(sys.argv[3])
            sock = run_client(server_ip, port)
        except ValueError:
            print("Invalid port number")
            return 1
    else:
        print("Invalid mode. Use 'server' or 'client'")
        return 1
    
    if sock is None:
        print("Failed to establish connection")
        return 1
    
    # Create thread for receiving messages
    receive_thread = threading.Thread(target=receive_messages, args=(sock,))
    receive_thread.daemon = True
    receive_thread.start()
    
    # Main thread handles sending messages
    try:
        while True:
            message = input()
            if not message:  # Empty message
                continue
                
            # Add newline if not present
            if not message.endswith('\n'):
                message += '\n'
            
            # Check message length
            if len(message) > MAX_MESSAGE_SIZE:
                print(f"Message too long (max {MAX_MESSAGE_SIZE} characters)")
                continue
            
            bytes_sent = sock.send(message.encode('utf-8'))
            if bytes_sent == 0:
                print("Failed to send message or connection lost")
                break
                
    except EOFError:
        # Handle Ctrl+D
        pass
    except KeyboardInterrupt:
        # Handle Ctrl+C
        pass
    except Exception as e:
        print(f"Error: {e}")
    
    # Cleanup
    if sock:
        sock.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())