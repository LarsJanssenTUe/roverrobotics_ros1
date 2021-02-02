
#include "protocol_zero.hpp"

namespace RoverRobotics {
ZeroProtocolObject::ZeroProtocolObject(const char* device,
                                       std::string new_comm_type) {}

ZeroProtocolObject::~ZeroProtocolObject() {}

void ZeroProtocolObject::update_drivetrim(double) {}

void ZeroProtocolObject::translate_send_estop() {}

statusData ZeroProtocolObject::translate_send_robot_status_request() {}

statusData ZeroProtocolObject::translate_send_robot_info_request() {}

void ZeroProtocolObject::translate_send_speed(double*) {}

void ZeroProtocolObject::handle_unsupported_ros_message() {}

void ZeroProtocolObject::unpack_robot_response(unsigned char * a) {}

bool ZeroProtocolObject::isConnected() {}

// void register_state_response_cb(boost::function<int(void)> _f);

void ZeroProtocolObject::register_comm_base(const char* device) {}

}  // namespace RoverRobotics