#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"
#include "lb_client.h"
#include <iostream>


int main() {
    std::vector<std::string> gateway_addresses = {"localhost:50052", "localhost:50053", "localhost:50054"};
    std::vector<std::string> exchange_names = {"Binance", "Coinbase", "Kraken"};
    LoadBalancer lb(gateway_addresses, exchange_names);

    lb.setPolicy(LeastConnections);

    for (int i = 1; i <= 10; i++) {
        Order currOrder;
        currOrder.set_order_id(std::to_string(i));
        currOrder.set_price(100.00);
        currOrder.set_quantity(3);
        // currOrder.set_exchange_id("Coinbase");
        
        ExecutionReport currReport;

        grpc::Status currStatus = lb.RouteOrder(currOrder, &currReport);

        if (currStatus.ok()) {
            std::cout << "Order " << currReport.order_id() << " routed successfully" << "\n";
        } else {
            std::cerr << "Failed to route order: " << i << " " << currStatus.error_message() << "\n";
        }
    }

    lb.printAverageLatencies();

    // lb.channelUseFrequency();

    return 0;
}