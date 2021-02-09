#pragma once

#include "comm_base.hpp"
#include "comm_can.hpp"
#include "comm_serial.hpp"

namespace RoverRobotics {
class BaseProtocolObject;
}
class RoverRobotics::BaseProtocolObject {
 public:
  BaseProtocolObject() = default;
  virtual ~BaseProtocolObject();
  /* Accept a double that changes how the robot trim incase the robot is not
   * driving straight */
  virtual void update_drivetrim(double) = 0;
  /* Handle Software Estop Event */
  virtual void translate_send_estop(bool) = 0;
  /* Accept a double array of speeds to send to the robot*/
  virtual void translate_send_speed(double*) = 0;
  /* Handle robot status request*/
  virtual statusData translate_send_robot_status_request() = 0;
  /* Handle robot unique info request*/
  virtual statusData translate_send_robot_info_request() = 0;
  /* Accept vector of unsigned int 32
   * Callback function to handle message from robot*/
  virtual void unpack_comm_response(std::vector<uint32_t>) = 0;
  /* Check if the robot is still connected */
  virtual bool isConnected() = 0;
  /* Setup communication interface */
  virtual void register_comm_base(const char* device) = 0;
};
