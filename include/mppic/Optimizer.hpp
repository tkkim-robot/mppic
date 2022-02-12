#pragma once

#include <tf2/utils.h>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <nav2_costmap_2d/costmap_2d_ros.hpp>
#include <nav_msgs/msg/path.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>

#include "mppic/impl/ControlSequence.hpp"
#include "mppic/impl/CriticScorer.hpp"
#include "mppic/impl/State.hpp"

namespace mppi::optimization {

template <typename T>
class Optimizer {
public:
  using model_t =
      xt::xtensor<T, 2>(const xt::xtensor<T, 2> &);

  Optimizer() = default;

  void on_configure(rclcpp_lifecycle::LifecycleNode *const parent,
                    const std::string &node_name,
                    nav2_costmap_2d::Costmap2DROS *const costmap_ros,
                    model_t &&model);

  void
  on_cleanup() {}
  void
  on_activate() {}
  void
  on_deactivate() {}

  geometry_msgs::msg::TwistStamped evalNextBestControl(
      const geometry_msgs::msg::PoseStamped &robot_pose,
      const geometry_msgs::msg::Twist &robot_speed,
      const nav_msgs::msg::Path &plan);

  xt::xtensor<T, 3>
  getGeneratedTrajectories() const {
    return generated_trajectories_;
  }

  xt::xtensor<T, 2> evalTrajectoryFromControlSequence(
      const geometry_msgs::msg::PoseStamped &robot_pose,
      const geometry_msgs::msg::Twist &robot_speed) const;

private:
  void getParams();
  void reset();
  void configureComponents();

  /**
   * @brief Invoke generateNoisedControlBatches, assign result tensor to
   * batches_ controls dimensions and integrate recieved controls in
   * trajectories
   *
   * @return trajectories: tensor of shape [ batch_size_, time_steps_, 3 ]
   * where 3 stands for x, y, yaw
   */
  xt::xtensor<T, 3> generateNoisedTrajectories(
      const geometry_msgs::msg::PoseStamped &robot_pose,
      const geometry_msgs::msg::Twist &robot_speed);

  /**
   * @brief Generate random controls by gaussian noise with mean in
   * control_sequence_
   *
   * @return Control batches tensor of shape [ batch_size_, time_steps_, 2]
   * where 2 stands for v, w
   */
  xt::xtensor<T, 3> generateNoisedControlBatches() const;

  void applyControlConstraints();

  /**
   * @brief Invoke setBatchesInitialVelocities and
   * propagateBatchesVelocitiesFromInitials
   *
   * @param twist current robot speed
   */
  void evalBatchesVelocities(
      auto &state, const geometry_msgs::msg::Twist &robot_speed) const;

  void setBatchesInitialVelocities(
      auto &state, const geometry_msgs::msg::Twist &robot_speed) const;

  /**
   * @brief predict and propagate velocities in batches_ using model
   * for time horizont equal to time_steps_
   */
  void propagateBatchesVelocitiesFromInitials(auto &state) const;

  xt::xtensor<T, 3> integrateBatchesVelocities(
      const auto &state,
      const geometry_msgs::msg::PoseStamped &robot_pose) const;

  /**
   * @brief Update control_sequence_ with batch controls weighted by costs
   * using softmax function
   *
   * @param costs batches costs, tensor of shape [ batch_size ]
   */
  void updateControlSequence(const xt::xtensor<T, 1> &costs);

  std::vector<geometry_msgs::msg::Point> getOrientedFootprint(
      const std::array<double, 3> &robot_pose,
      const std::vector<geometry_msgs::msg::Point> &footprint_spec) const;

  /**
   * @brief Get offseted control from control_sequence_
   *
   */
  auto getControlFromSequence(unsigned int);

  rclcpp_lifecycle::LifecycleNode *parent_;
  std::string node_name_;
  nav2_costmap_2d::Costmap2DROS *costmap_ros_;
  nav2_costmap_2d::Costmap2D *costmap_;
  std::function<model_t> model_;

  unsigned int batch_size_;
  unsigned int time_steps_;
  unsigned int iteration_count_;
  double model_dt_;
  double v_limit_;
  double w_limit_;
  double temperature_;
  T v_std_;
  T w_std_;
  static constexpr unsigned int batches_last_dim_size_ = 5;
  static constexpr unsigned int control_dim_size_ = 2;

  bool approx_reference_cost_;

  State<T> state_;
  ControlSequence<T> control_sequence_;
  CriticScorer<T> critic_scorer_;
  xt::xtensor<T, 3> generated_trajectories_;

  rclcpp::Logger logger_{rclcpp::get_logger("MPPI Optimizer")};
};

}  // namespace mppi::optimization
