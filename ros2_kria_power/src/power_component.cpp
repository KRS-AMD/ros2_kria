/*
      ____  ____
     /   /\/   /
    /___/  \  /   Copyright (c) 2021, Xilinx®.
    \   \   \/    Author: Víctor Mayoral Vilches <victorma@xilinx.com>
     \   \
     /   /
    /___/   /\
    \   \  /  \
     \___\/\___\

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ros2_kria_power/power_component.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>
#include <string>

#include "tracetools/tracetools.h"
#include "tracetools_acceleration/tracetools.h"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/clock.hpp"
#include <ros2_kria_msgs/msg/power.hpp>
#include <vitis_common/common/utilities.hpp>

using namespace std::chrono_literals;

namespace composition
{

// Create a Power "component" that subclasses the generic rclcpp::Node base class.
// Components get built into shared libraries and as such do not write their own main functions.
// The process using the component's shared library will instantiate the class as a ROS node.
Power::Power(const rclcpp::NodeOptions & options)
: Node("power", options), count_(0)
{
  // Create a publisher of "std_mgs/String" messages on the "chatter" topic.
  pub_ = create_publisher<ros2_kria_msgs::msg::Power>("power", 10);

  // define differential time value (in ms)
  dt_ = 10ms;

  // Use a timer to schedule periodic message publishing.
  timer_ = create_wall_timer(dt_, std::bind(&Power::on_timer, this));

  // initialize power readings
  initialize();

}

void Power::on_timer()
{
  FILE *fp;

  // Simulate readings - debugging in workstation
  // float power_reading = 1500.0 / 1000.0;  // Watts

  // Fetch the power measurement from ina260
  fp = fopen(filename_,"r");
  if(fp == NULL)
  {
    printf("unable to open %s\n",filename_);
    return;
  }
  fscanf(fp,"%ld",&power_reading_);
  float power_reading = ((float) power_reading_) / (1000*1000);

  // Log in stdout
  std::flush(std::cout);

  auto msg = std::make_unique<ros2_kria_msgs::msg::Power>();
  // msg->stamp = this->now();
  msg->power = power_reading;

  // Log in stdout
  // RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", msg->data.c_str());
  // std::flush(std::cout);

  // Put the message into a queue to be processed by the middleware.
  // This call is non-blocking.
  pub_->publish(std::move(msg));

  // Log for tracing
  // TRACEPOINT(kria_power, power_reading);
  TRACEPOINT(kria_power_dt, power_reading, dt_.count()/1000.0);

  fclose(fp);

}

/* @brief Initialize the Power node to make readings
 *
 * @return None
 */
void Power::initialize()
{
  int hwmon_id;
  char hwmon_id_str[255];
  char base_filepath[] = "/sys/class/hwmon/hwmon";
  char const *device_name = "ina260_u14";

  hwmon_id = get_device_hwmon_id(0, device_name);  // set first argument to 1
                                                   // to debug hardware files
  if(hwmon_id == -1)
  {
  	printf("no hwmon device found for ina260_u14 under /sys/class/hwmon\n");
  	return;
  }

  sprintf(hwmon_id_str,"%d",hwmon_id);
  strcat(base_filepath,hwmon_id_str);
  strcpy(filename_,base_filepath);
  strcat(filename_,"/power1_input");
}

}  // namespace composition

#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
RCLCPP_COMPONENTS_REGISTER_NODE(composition::Power)
