#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"
#include <grpcpp/health_check_service_interface.h>

#include "lb_client.h"
#include <iostream>

LoadBalancer::LoadBalancer(const std::vector<std::string>& gateway_addresses, std::vector<std::string> exchange_names) {
    gateway_addresses_ = gateway_addresses;
    exchange_names_ = exchange_names;
    for (size_t i = 0; i < gateway_addresses.size(); i++) {
        exchange_to_channel_map_[exchange_names[i]] = i;
        active_connections_[gateway_addresses[i]] = 0;

        // keep servers warm
        grpc::ChannelArguments channelArgs;
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        channelArgs.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);

        auto currChannel = grpc::CreateCustomChannel(gateway_addresses[i], grpc::InsecureChannelCredentials(), channelArgs); //change to secure later

        // warm up channels from initial IDLE state
        grpc_connectivity_state state = currChannel->GetState(true);
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(1);
        currChannel->WaitForStateChange(state, deadline);

        channels_.push_back(currChannel);
        stubs_.push_back(OrderRouter::NewStub(currChannel));

        connection_queue_.push({gateway_addresses[i], currChannel, 0});

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
            std::cout << "Dummy order sent successfully to " << gateway_addresses[i] << "\n";
        } else {
            std::cerr << "Failed to send dummy order to " << gateway_addresses[i] << ": " << status.error_message() << "\n";
        }
    }
}


::grpc::Status LoadBalancer::RouteOrder(const Order& order, ExecutionReport* report) {
    std::shared_ptr<grpc::Channel> channel;
    if (order.exchange_id() != "") {
        channel = forceGateway(order.exchange_id());
    } else {
        if (current_policy_ == RoundRobin) {
            channel = selectRoundRobin();
        } else if (current_policy_ == LeastConnections) {
            channel = selectLeastConnections();
        } else if (current_policy_ == LowestLatency) {
            channel = selectLowestLatency();
        }
    }
    
    if (!channel) {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "No healthy gateways available");
    }

    size_t index = std::distance(channels_.begin(), std::find(channels_.begin(), channels_.end(), channel));
    auto& stub = stubs_[index];

    updateConnections(gateway_addresses_[index], 1);

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(10));

    auto start_time = std::chrono::high_resolution_clock::now();
    grpc::Status status = stub->RouteOrder(&context, order, report);
    auto end_time = std::chrono::high_resolution_clock::now();

    updateConnections(gateway_addresses_[index], -1);

    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
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


std::shared_ptr<grpc::Channel> LoadBalancer::forceGateway(const std::string exchange_id) {
    auto it = exchange_to_channel_map_.find(exchange_id);
    if (it != exchange_to_channel_map_.end()) {
        size_t index = it->second;

        if (isHealthy(channels_[index])) {
            return channels_[index];
        } else {
            std::cerr << "Gateway for exchange " << exchange_id << " is unhealthy\n";
            return nullptr;
        }
    }
    
    std::cerr << "No gateway found for exchange " << exchange_id << "\n";
    return nullptr;
}

std::shared_ptr<grpc::Channel> LoadBalancer::selectRoundRobin() {
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
}

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

std::shared_ptr<grpc::Channel> LoadBalancer::selectLeastConnections() {
    while (!connection_queue_.empty()) {
        GatewayInfo top = connection_queue_.top();
        connection_queue_.pop();

        if (isHealthy(top.channel)) {
            connection_queue_.push(top);
            return top.channel;
        }
    }
    return nullptr;
}

void LoadBalancer::updateConnections(const std::string& address, int delta) {
    std::priority_queue<GatewayInfo, std::vector<GatewayInfo>, std::greater<>> new_queue;

    while (!connection_queue_.empty()) {
        GatewayInfo top = connection_queue_.top();
        connection_queue_.pop();

        if (top.address == address) {
            top.active_connections += delta;
        }

        new_queue.push(top);
    }

    connection_queue_ = std::move(new_queue);
}

std::shared_ptr<grpc::Channel> LoadBalancer::selectLowestLatency() {
    return channels_[0];
}