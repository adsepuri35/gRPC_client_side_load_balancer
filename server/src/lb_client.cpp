#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"
#include <grpcpp/health_check_service_interface.h>

#include "lb_client.h"
#include <iostream>

LoadBalancer::LoadBalancer(const std::vector<std::string>& gateway_addresses) {
    gateway_addresses_ = gateway_addresses;
    for (const auto& address : gateway_addresses) {
        auto currChannel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials()); //change to secure later

        channels_.push_back(currChannel);
        stubs_.push_back(OrderRouter::NewStub(currChannel));
    }
}


::grpc::Status LoadBalancer::RouteOrder(const Order& order, ExecutionReport* report) {
    auto channel = selectChannel();
    if (!channel) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "No healthy gateways available");
    }

    size_t index = std::distance(channels_.begin(), std::find(channels_.begin(), channels_.end(), channel));
    auto& stub = stubs_[index];

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(100));

    grpc::Status status = stub->RouteOrder(&context, order, report);

    std::cout << "Used channel: " << channel << "\n";
    channel_freq_[gateway_addresses_[index]]++;
    
    // add to failure count

    return status;
}


std::shared_ptr<grpc::Channel> LoadBalancer::selectChannel() {
    size_t attempts = 0;
    while (attempts < channels_.size()) {
        current_gateway_ = (current_gateway_ + 1) % channels_.size();
        auto channel = channels_[current_gateway_];
        
        if (isHealthy(channel)) {
            return channel;
        }

        attempts++;
    }
    return nullptr;
};

//implement
bool LoadBalancer::isHealthy(const std::shared_ptr<grpc::Channel>& channel) {
    return true;
};

void LoadBalancer::channelUseFrequency() {
    for (auto kvpair : channel_freq_) {
        std::cout << kvpair.first << " : " << kvpair.second << "\n";
    }
}