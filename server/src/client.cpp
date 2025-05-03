#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"
#include <iostream>

int main() {
    auto channel = grpc::CreateChannel("localhost:9999", grpc::InsecureChannelCredentials());
    auto stub = OrderRouter::NewStub(channel);

    Order order;
    order.set_order_id("123445");
    order.set_price(100.0);
    order.set_quantity(2);

    ExecutionReport report;


    grpc::ClientContext context;
    auto status = stub->RouteOrder(&context, order, &report);

    if (status.ok()) {
        std::cout << "Order routed successfully!\n";
        std::cout << "Order ID: " << report.order_id() << "\n";
        std::cout << "Status: " << report.status() << "\n";
        std::cout << "Total: " << report.total() << "\n";
    } else {
        std::cerr << "Failed to route order: " << status.error_message() << "\n";
    }

    return 0;

}