#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"
#include "lb_client.h"
#include <iostream>

// int main() {
//     auto channel = grpc::CreateChannel("localhost:9999", grpc::InsecureChannelCredentials());
//     auto stub = OrderRouter::NewStub(channel);

//     Order order;
//     order.set_order_id("123445");
//     order.set_price(100.0);
//     order.set_quantity(2);

//     ExecutionReport report;


//     grpc::ClientContext context;
//     auto status = stub->RouteOrder(&context, order, &report);

//     if (status.ok()) {
//         std::cout << "Order routed successfully!\n";
//         std::cout << "Order ID: " << report.order_id() << "\n";
//         std::cout << "Status: " << report.status() << "\n";
//         std::cout << "Total: " << report.total() << "\n";
//     } else {
//         std::cerr << "Failed to route order: " << status.error_message() << "\n";
//     }

//     return 0;

// }

int main() {
    std::vector<std::string> gateway_addresses = {"localhost:50052", "localhost:50053", "localhost:50054"};
    LoadBalancer lb(gateway_addresses);

    for (int i = 1; i <= 10; i++) {
        Order currOrder;
        currOrder.set_order_id(std::to_string(i));
        currOrder.set_price(100.90);
        currOrder.set_quantity(3);

        ExecutionReport currReport;

        grpc::Status currStatus = lb.RouteOrder(currOrder, &currReport);

        if (currStatus.ok()) {
            std::cout << "Order " << currReport.order_id() << " routed successfully" << "\n";
        } else {
            std::cerr << "Failed to route order: " << i << " " << currStatus.error_message() << "\n";
        }
    }

    return 0;
}