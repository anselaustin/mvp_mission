#pragma once

#include "behavior_interface/behavior_base.h"
#include "ros/ros.h"
#include "std_msgs/Float64.h"

namespace helm {

    class DepthTracking : public BehaviorBase {
    private:
        /**
         * @brief
         * @todo To be implemented
         */
        void initialize() override;

        /**
         * @brief trivial node handler
         */
        ros::NodeHandlePtr m_nh;

        /**
         * @brief requested depth
         */
        double m_requested_depth;

        /**
         * @brief Trivial subscriber object
         */
        ros::Subscriber m_sub;

        /**
         * @brief Trvial subscriber callback
         *
         * @param msg
         */
        void f_cb_sub(const std_msgs::Float64::ConstPtr& msg);

        double m_max_pitch;

        double m_fwd_distance;

        bool m_use_heave_velocity;

    public:

        /**
         * @brief Trivial constructor
         */
        DepthTracking();

        ~DepthTracking();

        bool request_set_point(seal_msgs::ControlProcess *msg) override;

    };
}