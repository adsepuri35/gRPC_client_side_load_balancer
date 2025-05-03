#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"

class GatewayServiceImpl final : public OrderRouter::Service {
    public:
        grpc::Status RouteOrder(grpc::ServerContext* context, const Order* order, ExecutionReport* report) override {
            report->set_order_id(order->order_id());
            report->set_status(ExecutionReport::FILLED);
            report->set_total(order->price() * order->quantity());
            return grpc::Status::OK;
        }
};

int main() {
    GatewayServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50052", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Gateway server is running on 0.0.0.0:50052" << std::endl;

    server->Wait();

    return 0;

}