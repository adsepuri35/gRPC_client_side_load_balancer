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

        // warm up channels from initial IDLE state
        grpc_connectivity_state state = currChannel->GetState(true);
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(1);
        currChannel->WaitForStateChange(state, deadline);

        channels_.push_back(currChannel);
        stubs_.push_back(OrderRouter::NewStub(currChannel));

        // send dummy order to warm up channel
        Order dummyOrder;
        dummyOrder.set_order_id("dummy");
        dummyOrder.set_price(0.0);
        dummyOrder.set_quantity(0);

        ExecutionReport dummyReport;
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(100));

        auto& stub = stubs_.back();
        grpc::Status status = stub->RouteOrder(&context, dummyOrder, &dummyReport);

        if (status.ok()) {
            std::cout << "Dummy order sent successfully to " << address << "\n";
        } else {
            std::cerr << "Failed to send dummy order to " << address << ": " << status.error_message() << "\n";
        }
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
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(10));

    auto start_time = std::chrono::high_resolution_clock::now();

    grpc::Status status = stub->RouteOrder(&context, order, report);

    //calc latency
    auto end_time = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    // std::cout << "Latency for routing order to " << gateway_addresses_[index] << ": " << latency << " ms\n";
    latency_records_[gateway_addresses_[index]].push_back(latency);
    
    // add to failure count
    if (status.ok()) {
        channel_freq_[gateway_addresses_[index]]++;
    } else {
        failure_counts_[gateway_addresses_[index]]++;
        std::cerr << "Failed to route order to " << gateway_addresses_[index]
                  << ": " << status.error_message() << "\n";
    }

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


bool LoadBalancer::isHealthy(const std::shared_ptr<grpc::Channel>& channel) {
    grpc_connectivity_state state = channel->GetState(true);
    if (state == GRPC_CHANNEL_READY) {
        return true;
    } else if (state == GRPC_CHANNEL_IDLE || GRPC_CHANNEL_CONNECTING) {
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(50);
        if (channel->WaitForStateChange(state, deadline)) {
            return channel->GetState(false) == GRPC_CHANNEL_READY;
        }
    }
    return false;
}

void LoadBalancer::channelUseFrequency() {
    for (auto kvpair : channel_freq_) {
        std::cout << kvpair.first << " : " << kvpair.second << "\n";
    }
}

void LoadBalancer::seeChannelState(const std::shared_ptr<grpc::Channel>& channel, size_t index) {
    grpc_connectivity_state state = channel->GetState(true);

    switch (state) {
        case GRPC_CHANNEL_IDLE:
            std::cout << "Channel "<< index << " is IDLE\n";
            break;
        case GRPC_CHANNEL_CONNECTING:
            std::cout << "Channel "<< index << " is CONNECTING\n";
            break;
        case GRPC_CHANNEL_READY:
            std::cout << "Channel "<< index << " is READY\n";
            break;
        case GRPC_CHANNEL_TRANSIENT_FAILURE:
            std::cout << "Channel "<< index << " is in TRANSIENT FAILURE\n";
            break;
        case GRPC_CHANNEL_SHUTDOWN:
            std::cout << "Channel "<< index << " is SHUTDOWN\n";
            break;
        default:
            std::cout << "Unknown channel state\n";
            break;
    }
}

void LoadBalancer::printAverageLatencies() {
    for (const auto& kvpair : latency_records_) {
        const auto& latencies = kvpair.second;
        long total_latency = std::accumulate(latencies.begin(), latencies.end(), 0L);
        double average_latency = static_cast<double>(total_latency) / latencies.size();

        std::cout << "Gateway: " << kvpair.first
                  << ", Average Latency: " << average_latency << " ms"
                  << ", Total Requests: " << latencies.size() << "\n";
    }
}