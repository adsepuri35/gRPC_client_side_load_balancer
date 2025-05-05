#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"

class GatewayServiceImpl final : public OrderRouter::Service {
    public:
        grpc::Status RouteOrder(grpc::ServerContext* context, const Order* order, ExecutionReport* report) override {
            report->set_order_id(order->order_id());
            std::cout << "Order " << report->order_id() << " received and processed\n";
            return grpc::Status::OK;
        }
};

int main() {
    GatewayServiceImpl service;

    grpc::ServerBuilder builder1, builder2, builder3;
    builder1.AddListeningPort("0.0.0.0:50052", grpc::InsecureServerCredentials());
    builder2.AddListeningPort("0.0.0.0:50053", grpc::InsecureServerCredentials());
    builder3.AddListeningPort("0.0.0.0:50054", grpc::InsecureServerCredentials());

    builder1.RegisterService(&service);
    builder2.RegisterService(&service);
    builder3.RegisterService(&service);

    auto server1 = builder1.BuildAndStart();
    auto server2 = builder2.BuildAndStart();
    auto server3 = builder3.BuildAndStart();

    std::cout << "Gateway servers are running on ports 50052, 50053, 50054" << std::endl;

    server1->Wait();
    server2->Wait();
    server3->Wait();

    return 0;

}