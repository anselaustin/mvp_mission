/*
    This file is part of MVP-Mission program.

    MVP-Mission is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MVP-Mission is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MVP-Mission.  If not, see <https://www.gnu.org/licenses/>.

    Author: Mingxi Zhou
    Email: mzhou@uri.edu
    Year: 2024

    Copyright (C) 2024 Smart Ocean Systems Laboratory
*/

#include "bhv_bathtub/bhv_bathtub.h"
#include "tf2/time.h"
#include <chrono>
#include <functional>
#include <memory>

using namespace helm;
using namespace std::chrono_literals;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


Bathtub::Bathtub() {
    std::cout << "A message from the DirectControl" << std::endl;
}

Bathtub::~Bathtub() {
}

void Bathtub::initialize(const rclcpp::Node::WeakPtr &parent) 
{
    /*************************************************************************/
    /* get the node */
    m_node = parent;
    auto node = m_node.lock();
    m_logger = node->get_logger();

    /*************************************************************************/
    /* Load Parameters for ROS2 */
    
    //setup tf buffer
    m_transform_buffer = std::make_unique<tf2_ros::Buffer>(node->get_clock());
    m_transform_listener = std::make_unique<tf2_ros::TransformListener>(*m_transform_buffer);

    //get vector params

    node->declare_parameter("no_depth_time", m_no_depth_time);
    node->get_parameter("no_depth_time", m_no_depth_time);

    node->declare_parameter("depth_list", m_depth_list);
    node->get_parameter("depth_list", m_depth_list);

    node->declare_parameter("depth_list", m_dive_ang_list);
    node->get_parameter("depth_list", m_dive_ang_list);

    node->declare_parameter("depth_list", m_climb_ang_list);
    node->get_parameter("depth_list", m_climb_ang_list);

    node->declare_parameter("depth_list", m_depth_time_list);
    node->get_parameter("depth_list", m_depth_time_list);

    std::string node_name = node->get_name();
    std::string ns = node->get_namespace();

    bhv_global_link = ns + "/world_ned";
    /**
     * @brief Declare the degree of freedoms to be controlled by the behavior
     *
     * @details This member variable dictates the DOFs that can be controllable
     * by the behavior. If this member is not initialized, behavior can only
     * trigger state changes. This vector gets values from enum type of
     * mvp_msgs/ControlMode enums.
     *
     */
    BehaviorBase::m_dofs = decltype(m_dofs){
        mvp_msgs::msg::ControlMode::DOF_Z,
        mvp_msgs::msg::ControlMode::DOF_PITCH,
    };
}


void Bathtub::activated() 
{
    rclcpp::Clock clock(RCL_SYSTEM_TIME);

    bhv_timer =  clock.now();
    m_depth_index = 0;
    std::cout << "Bathtub behavior is activated!" << std::endl;
}

void Bathtub::disabled() 
{
    std::cout << "Bathtub behavior is disabled!" << std::endl;
}


void Bathtub::transform_setpoint()
{
    Eigen::Vector3d xyz_helm;
    auto steady_clock = rclcpp::Clock();
    //transform desired depth
    try{
        //get tf from bhv world to helm world
        geometry_msgs::msg::TransformStamped tf_bw_hw = m_transform_buffer->lookupTransform(
            get_helm_global_link(),
            bhv_global_link,
            tf2::TimePointZero,
            10ms
        );
        auto tf_eigen = tf2::transformToEigen(tf_bw_hw);
        //xyz in the behavior world.
        xyz_helm = tf_eigen.rotation() * 
                        Eigen::Vector3d(0, 0, m_desired_depth)
                        + tf_eigen.translation();
        //got the desired depth
        m_desired_depth = xyz_helm.z();
    }catch (const tf2::TransformException & e) {
            RCLCPP_WARN_STREAM_THROTTLE(m_logger, steady_clock, 10, std::string("Can't compute thruster tf between ") + e.what());
            RCLCPP_INFO( m_logger, "Could not transform %s to %s: %s",
                         get_helm_global_link().c_str(), bhv_global_link.c_str(), e.what() ); 
          return;
    }

    //transform desired pitch
    try{
        auto tf_1 = m_transform_buffer->lookupTransform(
                bhv_global_link,
                get_helm_global_link(),
                tf2::TimePointZero,
                10ms
            );
        auto tf_1_eigen = tf2::transformToEigen(tf_1);
        Eigen::Matrix3d R;
        R = Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ()) *
            Eigen::AngleAxisd(m_desired_pitch, Eigen::Vector3d::UnitY()) *
            Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
        
        //find the total rotation matrix from the world link to the desired pose.
        Eigen::Matrix3d R_set_point =  tf_1_eigen.rotation() *R;

        //pitch angle
        m_desired_pitch = asin(-R_set_point(2, 0));


    }catch (const tf2::TransformException & e) {
            RCLCPP_WARN_STREAM_THROTTLE(m_logger, steady_clock, 10, std::string("Can't compute thruster tf between ") + e.what());
            RCLCPP_INFO( m_logger, "Could not transform %s to %s: %s",
                         get_helm_global_link().c_str(), bhv_global_link.c_str(), e.what() ); 
          return;
    }


}

bool Bathtub::request_set_point(
    mvp_msgs::msg::ControlProcess *set_point) 
{
    Eigen::Vector3d xyz_bw;
    auto steady_clock = rclcpp::Clock();
    rclcpp::Clock clock(RCL_SYSTEM_TIME);

    rclcpp::Time now = clock.now();
    if(m_depth_index == 0)
    {
        if(now.seconds() - bhv_timer.seconds() > m_no_depth_time )
        {
            m_depth_index = 1; //start depth segments
        }
        else
        {
            return false;
        }
    }
    //do depth segments
    else
    {
        if(now.seconds() - bhv_timer.seconds() > m_depth_time_list[m_depth_index-1] )
        {
            m_depth_index ++; //start depth segments
            //check if we are at the end? if so we start from the beginning
            if (m_depth_index  == static_cast<int>( m_depth_list.size())-1 )
            {
                m_depth_index = 0;
            }
        }
         m_desired_depth = m_depth_list[m_depth_index-1];
        //convert depth into the helm global link.

        try{
        //get tf from bhv world to helm world
        geometry_msgs::msg::TransformStamped tf_hw_bw = m_transform_buffer->lookupTransform(
            bhv_global_link,
            get_helm_global_link(),
            tf2::TimePointZero,
            10ms
        );
        auto tf_eigen = tf2::transformToEigen(tf_hw_bw);
        //xyz in the behavior world.
        xyz_bw = tf_eigen.rotation() * 
                        Eigen::Vector3d(BehaviorBase::m_process_values.position.x,
                                        BehaviorBase::m_process_values.position.y, 
                                        BehaviorBase::m_process_values.position.z)
                        + tf_eigen.translation();

        //determine we need to dive or climb in NED.
        if(m_desired_depth > xyz_bw.z())
        {
            m_desired_pitch = m_dive_ang_list[m_depth_index-1];
        }
        else if(m_desired_depth < xyz_bw.z())
        {
            m_desired_pitch = m_climb_ang_list[m_depth_index-1];
        }
        else
        {
            m_desired_pitch = 0;
        }

        transform_setpoint();
        set_point->position.z = m_desired_depth;
        set_point->orientation.y = m_desired_pitch;


        } catch (const tf2::TransformException & e) {
            RCLCPP_WARN_STREAM_THROTTLE(m_logger, steady_clock, 10, std::string("Can't compute thruster tf between ") + e.what());
            RCLCPP_INFO( m_logger, "Could not transform %s to %s: %s",
                         get_helm_global_link().c_str(), bhv_global_link.c_str(), e.what() ); 
          return false;

        }
    }

}


/**
 * @brief Behavior must export the class to the Plugin library.
 */
 #include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(helm::Bathtub, helm::BehaviorBase)