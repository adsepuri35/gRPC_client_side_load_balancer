#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"

#include "lb_client.h"

LoadBalancer::LoadBalancer(const std::vector<std::string>& gateway_addresses) {
    for (const auto& address : gateway_addresses) {
        auto currChannel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials()); //change to secure later

        channels_.push_back(currChannel);
        stubs_.push_back(OrderRouter::NewStub(currChannel));

    }
}

grpc::Status RouteOrder(const Order& order, ExecutionReport* report) {
    auto channel = selectChannel();
    if (!channel) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "No healthy gateways available");
    }

    grpc::ClientContext context;

}


//implement
std::shared_ptr<grpc::Channel> selectChannel() {

};

