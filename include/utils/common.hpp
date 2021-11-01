#pragma once

#include "rclcpp/rclcpp.hpp"

namespace mppi::utils {

template <typename N, typename T>
T getParam(std::string const &param_name, T default_value,
           const std::shared_ptr<N> &node) {
  T param;

  if (!node->has_parameter(param_name)) {
    node->declare_parameter(param_name, rclcpp::ParameterValue(default_value));
  }
  node->get_parameter(param_name, param);

  return param;
}

} // namespace mppi::utils
