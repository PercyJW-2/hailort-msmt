#include <iostream>
#include <hailo/hailort.hpp>
#include "structopt.hpp"

struct Args {
    // Port that is used to send measurement data
    std::optional<uint16_t> data_port;
    // Port that is used to control this interface
    std::optional<uint16_t> control_port;
    // bind address
    std::optional<std::string> bind_address;
};
STRUCTOPT(Args, data_port, control_port, bind_address);

int main(const int argc, char* argv[]) {
    Args args;
    try {
        args = structopt::app("hailort-msmt").parse<Args>(argc, argv);
    } catch (structopt::exception& e) {
        std::cout << e.what() << std::endl;
        std::cout << e.help();
        return 0;
    }

    auto devices_result = hailort::Device::scan();
    if (!devices_result.has_value()) {
        std::cout << "Could not retrieve device list!\n";
        return -1;
    }
    const auto& devices = devices_result.value();
    if (devices.empty()) {
        std::cout << "No devices found!\n";
        return -2;
    }
    auto device_result = hailort::Device::create(devices[0]);

    if (!device_result.has_value()) {
        std::cout << "Could not create device!\n";
        return -3;
    }
    auto& device = device_result.value();

    auto power_result = device->power_measurement(HAILO_DVM_OPTIONS_AUTO, HAILO_POWER_MEASUREMENT_TYPES__POWER);

    //device.get()->get_power_measurement()

    return 0;
}