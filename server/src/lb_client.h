#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"

class LoadBalancer {
public:
  LoadBalancer(const std::vector<std::string>& gateway_addresses);
  ::grpc::Status RouteOrder(const Order& order, ExecutionReport* report);
  void channelUseFrequency();
  void seeChannelState(const std::shared_ptr<grpc::Channel>& channel, size_t index);
  void printAverageLatencies();

private:
  std::vector<std::shared_ptr<grpc::Channel>> channels_;
  std::vector<std::unique_ptr<OrderRouter::Stub>> stubs_;
  std::vector<std::string> gateway_addresses_;
  size_t current_gateway_ = 0;
  std::unordered_map<std::string, int> failure_counts_;
  std::unordered_map<std::string, int> channel_freq_;
  std::unordered_map<std::string, std::vector<long>> latency_records_;


  std::shared_ptr<grpc::Channel> selectChannel();
  bool isHealthy(const std::shared_ptr<grpc::Channel>& channel);
};