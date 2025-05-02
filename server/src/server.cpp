#include <grpcpp/grpcpp.h>
#include "proto/order.grpc.pb.h"
#include "proto/order.pb.h"

class ProcessingImpl : public OrderRouter::Service {
    ::grpc::Status routeOrder(::grpc::ServerContext* context, const ::Order* request, ::ExecutionReport* response) {
        std::cout << "Order Received!\n";
        response->set_order_id(request->order_id());
        response->set_status(ExecutionReport::FILLED);
        response->set_total(request->price() * request->quantity());
        return grpc::Status::OK;
    }
};

int main() {
    ProcessingImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:9999", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    server->Wait();

    return 0;
}