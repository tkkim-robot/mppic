// Copyright 2022 @artofnothingness Alexey Budyakov, Samsung Research
#include "mppic/critics/path_follow_critic.hpp"

#include <xtensor/xmath.hpp>
#include <xtensor/xsort.hpp>

namespace mppi::critics
{

void PathFollowCritic::initialize()
{
  auto getParam = parameters_handler_->getParamGetter(name_);

  getParam(
    max_path_ratio_,
    "max_path_ratio", 0.40f);

  getParam(offset_from_furthest_, "offset_from_furthest", 10);

  getParam(power_, "cost_power", 1);
  getParam(weight_, "cost_weight", 3.0);
}

void PathFollowCritic::score(CriticData & data)
{
  if (!enabled_) {
    return;
  }

  utils::setPathFurthestPointIfNotSet(data);
  if (utils::getPathRatioReached(data) > max_path_ratio_) {
    return;
  }

  auto offseted_idx = std::min(
    *data.furthest_reached_path_point + offset_from_furthest_, data.path.x.shape(0) - 1);

  const auto path_x = xt::view(data.path.x, offseted_idx);
  const auto path_y = xt::view(data.path.y, offseted_idx);

  const auto last_x = xt::view(data.trajectories.x, xt::all(), -1);
  const auto last_y = xt::view(data.trajectories.y, xt::all(), -1);

  auto dists = xt::sqrt(
    xt::pow(last_x - path_x, 2) +
    xt::pow(last_y - path_y, 2));

  data.costs += xt::pow(weight_ * std::move(dists), power_);
}

}  // namespace mppi::critics

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(
  mppi::critics::PathFollowCritic,
  mppi::critics::CriticFunction)
