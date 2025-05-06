#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"
#include <queue>

enum LoadBalancingPolicy {
  RoundRobin,
  LeastConnections,
  LowestLatency
};

struct GatewayInfoConnections {
  std::string address;
  std::shared_ptr<grpc::Channel> channel;
  size_t active_connections;

  // minheap
  bool operator>(const GatewayInfoConnections& other) const {
      return active_connections > other.active_connections;
  }
};

struct GatewayInfoLatency {
  std::string address;
  std::shared_ptr<grpc::Channel> channel;
  double avg_latency;

  // minheap
  bool operator>(const GatewayInfoLatency& other) const {
      return avg_latency > other.avg_latency;
  }
};

class LoadBalancer {
public:
  LoadBalancer(const std::vector<std::string>& gateway_addresses, std::vector<std::string> exchange_names);
  ::grpc::Status RouteOrder(const Order& order, ExecutionReport* report);
  void channelUseFrequency();
  void seeChannelState(const std::shared_ptr<grpc::Channel>& channel, size_t index);
  void printAverageLatencies();
  void setPolicy(LoadBalancingPolicy policy) {
    current_policy_ = policy;
  }

private:
  LoadBalancingPolicy current_policy_ = LoadBalancingPolicy::RoundRobin;
  std::vector<std::shared_ptr<grpc::Channel>> channels_;
  std::vector<std::unique_ptr<OrderRouter::Stub>> stubs_;
  std::vector<std::string> gateway_addresses_;
  std::vector<std::string> exchange_names_;
  size_t current_gateway_ = 0;
  std::unordered_map<std::string, int> failure_counts_;
  std::unordered_map<std::string, int> channel_freq_;
  std::unordered_map<std::string, std::vector<long>> latency_records_;
  std::unordered_map<std::string, size_t> exchange_to_channel_map_;
  std::priority_queue<GatewayInfoConnections, std::vector<GatewayInfoConnections>, std::greater<>> connection_queue_;
  std::priority_queue<GatewayInfoLatency, std::vector<GatewayInfoLatency>, std::greater<>> latency_queue_;
  std::unordered_map<std::string, size_t> active_connections_;

  std::shared_ptr<grpc::Channel> forceGateway(const std::string exchange_id);
  std::shared_ptr<grpc::Channel> selectRoundRobin();
  std::shared_ptr<grpc::Channel> selectLeastConnections();
  std::shared_ptr<grpc::Channel> selectLowestLatency();
  bool isHealthy(const std::shared_ptr<grpc::Channel>& channel);
  void updateConnections(const std::string& address, int delta);
  void updateLatency(const std::string& address, double new_latency);
};