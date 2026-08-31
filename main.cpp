#include <iostream>
#include <hailo/hailort.hpp>
#include "structopt.hpp"
#define ASIO_STANDALONE
#include <asio.hpp>

using asio::ip::tcp;
using asio::ip::udp;

struct Args {
    // Port that is used to send measurement data
    std::optional<uint16_t> data_port = 4000;
    // Port that is used to control this interface
    std::optional<uint16_t> control_port = 4001;
};
STRUCTOPT(Args, data_port, control_port);

static std::atomic_bool msmt_running{false};

static void run_udp_interface(asio::io_context& ctx, const uint16_t port) {
    // init hailo device
    auto devices_result = hailort::Device::scan();
    if (!devices_result.has_value()) {
        std::cout << "Could not retrieve device list!\n";
        return;
    }
    const auto& devices = devices_result.value();
    if (devices.empty()) {
        std::cout << "No devices found!\n";
        return;
    }
    auto device_result = hailort::Device::create(devices[0]);

    if (!device_result.has_value()) {
        std::cout << "Could not create device!\n";
        return;
    }
    auto& device = device_result.value();

    try {
        udp::socket udp_socket(ctx, udp::endpoint(udp::v4(), port));
        std::cout << "listening on port " << port << " for measurement data\n";

        while (true) {
            std::string rx_buffer;
            udp::endpoint remote_endpoint;

            size_t bytes_rcvd = udp_socket.receive_from(
                asio::buffer(rx_buffer, 65507),
                remote_endpoint);
            std::cout << "Got UDP connection, starting to send data\n";
            msmt_running = true;

            float last_msmt = -1;
            auto start = std::chrono::high_resolution_clock::now();
            while (msmt_running) {
                auto power_result =
                    device->power_measurement(HAILO_DVM_OPTIONS_AUTO, HAILO_POWER_MEASUREMENT_TYPES__POWER);
                if (!power_result.has_value()) {
                    std::cout << "Could not retrieve measurement data!\n";
                    msmt_running = false;
                }
                if (last_msmt != power_result.value()) {
                    last_msmt = power_result.value();
                    auto now = std::chrono::high_resolution_clock::now() - start;
                    auto time = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
                    std::string msmt = std::to_string(time) + ',' + std::to_string(last_msmt) + '\n';
                    udp_socket.send_to(asio::buffer(msmt), remote_endpoint);
                }
            }
        }
    } catch (std::exception& e) {
        std::cerr << "network error: " << e.what() << "\n";
    }
}

int main(const int argc, char* argv[]) {
    Args args;
    try {
        args = structopt::app("hailort-msmt").parse<Args>(argc, argv);
    } catch (structopt::exception& e) {
        std::cout << e.what() << std::endl;
        std::cout << e.help();
        return 0;
    }



    // init control network communication
    try {
        asio::io_context ctx;

        std::thread udp_thread(run_udp_interface, std::ref(ctx), args.data_port.value());

        tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), args.control_port.value()));
        std::cout << "listening on port " << args.control_port.value() << " for control messages\n";

        while (true) {
            tcp::socket socket = acceptor.accept();
            std::cout << "client connected!\n";

            std::string msg;
            asio::read_until(socket, asio::dynamic_buffer(msg), '\n');

            std::string return_msg;
            if (msg == "stop\n") {
                if (msmt_running) {
                    msmt_running = false;
                    return_msg = "stopping measurement...\n";
                } else {
                    return_msg = "measurement is currently not running\n";
                }
            } else {
                return_msg = "invalid message \n";
            }
            std::cout << return_msg;
            asio::write(socket, asio::buffer(return_msg));
        }
    } catch (std::exception& e) {
        std::cerr << "network error: " << e.what() << "\n";
    }

    return 0;
}