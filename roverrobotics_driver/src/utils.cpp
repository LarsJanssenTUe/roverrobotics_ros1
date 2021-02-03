#include "utils.hpp"

namespace RoverRobotics {
PidGains::PidGains() {}
void OdomControl::start(bool use_control, PidGains pid_gains, int max,
                        int min) {
  K_P_ = pid_gains.Kp;
  K_I_ = pid_gains.Ki;
  K_D_ = pid_gains.Kd;
}

OdomControl::OdomControl()
    : MOTOR_MAX_(250),
      MOTOR_MIN_(0),
      MOTOR_DEADBAND_(9),
      MAX_ACCEL_CUTOFF_(5.0),
      MIN_VELOCITY_(0.03),
      MAX_VELOCITY_(5),
      fs_(nullptr),
      K_P_(0),
      K_I_(0),
      K_D_(0),
      velocity_filtered_history_(5, 0),
      velocity_history_(3, 0),
      use_control_(false),
      skip_measurement_(false),
      at_max_motor_speed_(false),
      at_min_motor_speed_(false),
      stop_integrating_(false),
      velocity_error_(0),
      integral_error_(0),
      differential_error_(0),
      velocity_commanded_(0),
      velocity_measured_(0),
      velocity_filtered_(0) {}

OdomControl::OdomControl(bool use_control, PidGains pid_gains, int max, int min,
                         std::ofstream* fs)
    : MOTOR_MAX_(max),
      MOTOR_MIN_(min),
      MOTOR_DEADBAND_(9),
      MAX_ACCEL_CUTOFF_(5.0),
      MIN_VELOCITY_(0.03),
      MAX_VELOCITY_(3),
      fs_(fs),
      K_P_(pid_gains.Kp),
      K_I_(pid_gains.Ki),
      K_D_(pid_gains.Kd),
      velocity_filtered_history_(5, 0),
      velocity_history_(3, 0),
      use_control_(use_control),
      skip_measurement_(false),
      at_max_motor_speed_(false),
      at_min_motor_speed_(false),
      stop_integrating_(false),
      velocity_error_(0),
      integral_error_(0),
      differential_error_(0),
      velocity_commanded_(0),
      velocity_measured_(0),
      velocity_filtered_(0) {
  if (fs_ != nullptr && fs_->is_open()) {
    *fs_ << "time,Kp,Ki,Kd,error,integral_error,differential_error,error_"
            "filtered,meas_vel,filt_vel,cmd_vel,dt,motor_cmd\n";
    fs_->flush();
  }
}

OdomControl::OdomControl(bool use_control, PidGains pid_gains, int max, int min)
    : MOTOR_MAX_(max),
      MOTOR_MIN_(min),
      MOTOR_DEADBAND_(9),
      MAX_ACCEL_CUTOFF_(5.0),
      MIN_VELOCITY_(0.03),
      MAX_VELOCITY_(3),
      fs_(nullptr),
      K_P_(pid_gains.Kp),
      K_I_(pid_gains.Ki),
      K_D_(pid_gains.Kd),
      velocity_filtered_history_(5, 0),
      velocity_history_(3, 0),
      use_control_(use_control),
      skip_measurement_(false),
      at_max_motor_speed_(false),
      at_min_motor_speed_(false),
      stop_integrating_(false),
      velocity_error_(0),
      integral_error_(0),
      differential_error_(0),
      velocity_commanded_(0),
      velocity_measured_(0),
      velocity_filtered_(0) {}

unsigned char OdomControl::run(bool e_stop_on, bool control_on,
                               double commanded_vel, double measured_vel,
                               double dt, int firmwareBuildNumber) {
  velocity_commanded_ = commanded_vel;

  velocity_measured_ = measured_vel;

  /*
  Truncate the last two digits of the firmware version number,
  which returns in the format aabbcc. We divide by 100 to truncate
  cc since we don't care about the PATCH field of semantic versioning
  */
  int firmwareBuildNumberTrunc = firmwareBuildNumber / 100;
  velocity_filtered_ = filter(measured_vel, dt, firmwareBuildNumberTrunc);

  // If rover is E-Stopped, respond with NEUTRAL comman
  if (e_stop_on) {
    reset();
    return 125;
  }

  // If stopping, stop now when velocity has slowed.
  if ((commanded_vel == 0.0) && (fabs(velocity_filtered_) < 0.3)) {
    integral_error_ = 0;
    if (hasZeroHistory(velocity_filtered_history_)) {
      return 125;
    }
  }

  // If controller should be ON, run it.
  if (control_on) {
    velocity_error_ = commanded_vel - velocity_filtered_;
    if (!skip_measurement_) {
      motor_speed_ = PID(velocity_error_, dt);
    }
  } else {
    motor_speed_ = feedThroughControl();
  }

  motor_speed_ = boundMotorSpeed(motor_speed_, MOTOR_MAX_, MOTOR_MIN_);
  return (unsigned char)motor_speed_;
}

int OdomControl::feedThroughControl() {
  return (int)round(velocity_commanded_ + 125) % 250;
}

void OdomControl::reset() {
  integral_error_ = 0;
  velocity_error_ = 0;
  velocity_commanded_ = 0;
  velocity_measured_ = 0;
  velocity_filtered_ = 0;
  std::fill(velocity_filtered_history_.begin(),
            velocity_filtered_history_.end(), 0);
  motor_speed_ = 125;
  skip_measurement_ = false;
}

int OdomControl::PID(double error, double dt) {
  double p_val = P(error, dt);
  double i_val = I(error, dt);
  double d_val = D(error, dt);
  double pid_val = p_val + i_val + d_val;

  if (fabs(pid_val) > (MOTOR_MAX_ / 2.0))
  // Only integrate if the motor's aren't already at full speed
  {
    stop_integrating_ = true;
  } else {
    stop_integrating_ = false;
  }

  return (int)round(pid_val + 125.0);
}

double OdomControl::D(double error, double dt) {
  differential_error_ =
      (velocity_filtered_history_[0] - velocity_filtered_history_[1]) / dt;
  return K_D_ * differential_error_;
}

double OdomControl::I(double error, double dt) {
  if (!stop_integrating_) {
    integral_error_ += error * dt;
  }
  return K_I_ * integral_error_;
}

double OdomControl::P(double error, double dt) {
  double p_val = error * K_P_;
  return error * K_P_;
}

bool OdomControl::hasZeroHistory(const std::vector<double>& vel_history) {
  double avg =
      (fabs(vel_history[0]) + fabs(vel_history[1]) + fabs(vel_history[2])) /
      3.0;
  if (avg < 0.03)
    return true;
  else
    return false;
}

int OdomControl::boundMotorSpeed(int motor_speed, int max, int min) {
  int test_motor;
  int test_factor = 18;
  at_max_motor_speed_ = false;
  at_min_motor_speed_ = false;

  if (motor_speed > max) {
    motor_speed = max;
    at_max_motor_speed_ = true;
  }
  if (motor_speed < min) {
    motor_speed = min;
    at_min_motor_speed_ = true;
  }

  test_motor = motor_speed;

  return test_motor;
}

int OdomControl::deadbandOffset(int motor_speed, int deadband_offset) {
  // Compensate for deadband
  if (motor_speed > 125) {
    return (motor_speed + deadband_offset);
  } else if (motor_speed < 125) {
    return (motor_speed - deadband_offset);
  }
}

double OdomControl::filter(double velocity, double dt,
                           int firmwareBuildNumber) {
  static double time = 0;

  // Check for impossible acceleration, if it is impossible, ignore the
  // measurement.
  float accel = (velocity - velocity_filtered_history_[0]) / dt;
  velocity_history_.insert(velocity_history_.begin(), velocity);
  velocity_history_.pop_back();

  if (firmwareBuildNumber == 100) {
    velocity_filtered_ = 0.9 * velocity + 0.1 * velocity_filtered_history_[0];

  } else {
    float change_in_velocity = 0;

    if (accel > MAX_ACCEL_CUTOFF_) {
      change_in_velocity = 0.5 * dt * MAX_ACCEL_CUTOFF_;
      velocity = velocity_filtered_history_[0] + change_in_velocity;
    } else if (accel < -MAX_ACCEL_CUTOFF_) {
      change_in_velocity = -0.5 * dt * MAX_ACCEL_CUTOFF_;
      velocity = velocity_filtered_history_[0] + change_in_velocity;
    }

    velocity_filtered_ = 0.1 * velocity + 0.25 * velocity_filtered_history_[0] +
                         0.30 * velocity_filtered_history_[1] +
                         0.25 * velocity_filtered_history_[2] +
                         0.1 * velocity_filtered_history_[3];
  }

  velocity_filtered_history_.insert(velocity_filtered_history_.begin(),
                                    velocity_filtered_);
  velocity_filtered_history_.pop_back();

  return velocity_filtered_;
}  // class OdomControl

}  // namespace RoverRobotics
