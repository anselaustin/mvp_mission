#include "path_following.h"

using namespace helm;


void PathFollowing::f_visualize_path(bool clear) {
    visualization_msgs::Marker marker;
    marker.header = m_transformed_waypoints.header;
    marker.header.stamp = ros::Time::now();
    marker.type = visualization_msgs::Marker::LINE_STRIP;
    if(clear) {
        marker.action = visualization_msgs::Marker::DELETEALL;
    } else {
        marker.action = visualization_msgs::Marker::MODIFY;
        for (const auto &i: m_transformed_waypoints.polygon.points) {
            geometry_msgs::Point p;
            p.x = i.x;
            p.y = i.y;
            p.z = i.z;
            marker.points.emplace_back(p);
        }
        marker.pose.position.x = 0;
        marker.pose.position.y = 0;
        marker.pose.position.z = 0;
        marker.pose.orientation.x = 0.0;
        marker.pose.orientation.y = 0.0;
        marker.pose.orientation.z = 0.0;
        marker.pose.orientation.w = 1.0;

        // Set the scale of the marker -- 1x1x1 here means 1m on a side
        marker.scale.x = 0.15;
        marker.scale.y = 0.15;
        marker.scale.z = 0.15;

        // Set the color -- be sure to set alpha to something non-zero!
        marker.color.r = 0.0f;
        marker.color.g = 1.0f;
        marker.color.b = 0.0f;
        marker.color.a = 1.0;
    }
    marker.lifetime = ros::Duration();
    m_full_trajectory_publisher.publish(marker);

}


void PathFollowing::f_visualize_segment(bool clear) {
    visualization_msgs::Marker marker;
    marker.header = m_transformed_waypoints.header;
    marker.header.stamp = ros::Time::now();
    marker.type = visualization_msgs::Marker::LINE_STRIP;

    if(clear) {
        marker.action = visualization_msgs::Marker::DELETEALL;
    } else {
        marker.action = visualization_msgs::Marker::MODIFY;
        for(const auto& i : {m_wpt_first, m_wpt_second}) {
            geometry_msgs::Point p;
            p.x = i.x;
            p.y = i.y;
            p.z = i.z;
            marker.points.emplace_back(p);
        }

        marker.pose.position.x = 0;
        marker.pose.position.y = 0;
        marker.pose.position.z = 0;
        marker.pose.orientation.x = 0.0;
        marker.pose.orientation.y = 0.0;
        marker.pose.orientation.z = 0.0;
        marker.pose.orientation.w = 1.0;

        // Set the scale of the marker -- 1x1x1 here means 1m on a side
        marker.scale.x = 0.2;
        marker.scale.y = 0.2;
        marker.scale.z = 0.2;

        // Set the color -- be sure to set alpha to something non-zero!
        marker.color.r = 1.0f;
        marker.color.g = 0.0f;
        marker.color.b = 0.0f;
        marker.color.a = 1.0;

    }
    marker.lifetime = ros::Duration();

    m_trajectory_segment_publisher.publish(marker);

}
