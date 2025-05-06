# **gRPC Order Routing Load Balancer (In Progress)**

This project implements a **gRPC-based Load Balancer** that distributes order requests across multiple backend gateways using various load balancing algorithms, such as **Round Robin**, **Least Connections**, and **Latency-Based Server Selection**. The load balancer ensures efficient routing of requests while maintaining high availability and scalability.

---

## **Features**
- **Load Balancing Policies**:
  - **Round Robin**: Distributes requests evenly across all gateways.
  - **Least Connections**: Routes requests to the gateway with the fewest active connections.
  - **Latency-Based Selection**: Selects the gateway with the lowest average latency.
- **Health Checks**: Monitors the health of gRPC channels and avoids routing requests to unhealthy gateways.
- **Dynamic Updates**: Tracks active connections and latency metrics in real-time.
- **gRPC Integration**: Uses gRPC for communication between the client, load balancer, and backend gateways.

---

## **Project Structure**
```plaintext
workspace/
├── proto/                  # Protocol Buffers definitions
│   ├── order.proto         # gRPC service definition for order routing
├── server/
│   ├── src/
│   │   ├── client.cpp      # Client application to send orders
│   │   ├── lb_client.cpp   # LoadBalancer implementation
│   │   ├── lb_client.h     # LoadBalancer header file
│   │   ├── gateway_server.cpp # Backend gateway server implementation
├── [README.md](http://_vscodecontentref_/1)               # Project documentation
├── CMakeLists.txt          # Build configuration