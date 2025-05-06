# **gRPC Order Routing Load Balancer**

This project implements a **gRPC-based Load Balancer** that distributes order requests across multiple backend gateways using various load balancing algorithms, such as **Round Robin**, **Least Connections**, and **Latency-Based Server Selection**. The load balancer ensures efficient routing of requests while maintaining high availability and scalability.

---

## **Features**
- **gRPC Integration**: Uses gRPC for communication between the client, load balancer, and backend gateways.
- **Load Balancing Policies**:
  - **Round Robin**: Distributes requests evenly across all gateways.
  - **Least Connections**: Routes requests to the gateway with the fewest active connections.
  - **Latency-Based Selection**: Selects the gateway with the lowest average latency.
- **Health Checks**: Monitors the health of gRPC channels and avoids routing requests to unhealthy gateways.
- **Dynamic Updates**: Tracks active connections and latency metrics in real-time.
- **Circuit Breaking**: Automatically trips and recovers gateways based on failure or slow response thresholds.
- **Latency Benchmarking**: Measures and reports latency distribution (P50, P99) under load.

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
├── README.md               # Project documentation
├── CMakeLists.txt          # Build configuration
```

---

## **Benchmarking Latency Distribution**

The load balancer tracks request latencies for each gateway and calculates key latency percentiles:
- **P50 (50th Percentile)**: Represents the median latency, where 50% of requests are faster than or equal to this value.
- **P99 (99th Percentile)**: Represents the tail latency, where 99% of requests are faster than or equal to this value.

### **How It Works**
1. **Latency Tracking**:
   - Each request's latency is recorded in milliseconds.
   - Latencies are stored per gateway in the `latency_records_` map.

2. **Percentile Calculation**:
   - Latencies are sorted, and the P50 and P99 values are computed.
   - Results are printed for each gateway.

### **Example Output**
```plaintext
Gateway: localhost:50052, P50 Latency: 10 ms, P99 Latency: 50 ms, Total Requests: 200
Gateway: localhost:50053, P50 Latency: 12 ms, P99 Latency: 55 ms, Total Requests: 150
Gateway: localhost:50054, P50 Latency: 15 ms, P99 Latency: 60 ms, Total Requests: 150
```

### **How to Run the Benchmark**
1. Build the project:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

2. Run the client application:
   ```bash
   ./client
   ```

3. Observe the latency distribution printed in the console.

---

## **Simulating Load**

To benchmark under load, the client application can simulate concurrent requests using threads. This ensures realistic performance testing.

### **Example: Concurrent Requests**
The client sends 500 requests across 5 threads:
```cpp
const int total_requests = 500;
const int num_threads = 5;
const int requests_per_thread = total_requests / num_threads;

std::vector<std::thread> threads;
for (int t = 0; t < num_threads; t++) {
    int start_id = t * requests_per_thread + 1;
    int end_id = (t + 1) * requests_per_thread;
    threads.emplace_back(sendRequests, std::ref(lb), start_id, end_id);
}

for (auto& thread : threads) {
    thread.join();
}
```

---

## **Future Improvements**
- Integrate with monitoring tools like **Prometheus** for real-time metrics.
- Add **SSL/TLS** support for secure communication.
- Implement **Geo-Based Selection** for geographically distributed systems.
- Extend benchmarking to include throughput and error rates.

---

## **Contributing**
Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a new branch for your feature or bug fix.
3. Submit a pull request with a detailed description of your changes.

---

## **License**
This project is licensed under the MIT License. See the LICENSE file for details.

---

## **Contact**
For questions or support, please contact:
- **Name**: [Your Name]
- **Email**: [Your Email]