/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Neobotix GmbH
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Neobotix nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/


#include <neo_PlatformCtrl/MecanumKinematics.h>
#include <geometry_msgs/Vector3.h>

#define DIFF_PI 3.14159265

Mecanum4WKinematics::Mecanum4WKinematics()
{
	m_dAxis1Length = 0.5;
	m_dAxis2Length = 0.7;
}

void Mecanum4WKinematics::execForwKin(const sensor_msgs::JointState& js, nav_msgs::Odometry& odom, OdomPose& cpose)
{
	current_time = ros::Time::now();

	/* See "Omnidirectional Mobile Robot -Design and Implementation": Ioan Doroftei, Victor Grosu and Veaceslav Spinu

		l = l1 + l2

		             |                      |   | w1 |
		| vx |       |  1     1    1    1   | . | w2 |
		| vy | = R/4 | -1     1    1   -1   |   | w3 |
		| wz |       | -1/l  1/l -1/l  1/l  |   | w4 |
	*/
	//velocities:
	odom.twist.twist.linear.x = (js.velocity[0] +  js.velocity[1] + js.velocity[2] +  js.velocity[3]) * m_dDiam / 8;
	odom.twist.twist.linear.y = (js.velocity[1] -  js.velocity[0] - js.velocity[3] +  js.velocity[2]) * m_dDiam / 8;
	odom.twist.twist.linear.z = 0;
	odom.twist.twist.angular.x = 0;
	odom.twist.twist.angular.y = 0;
	odom.twist.twist.angular.z = (-js.velocity[0]+ js.velocity[1]-js.velocity[2]+js.velocity[3]) * m_dDiam / 4 / (m_dAxis1Length + m_dAxis2Length);
	//positions:
	double dt = (current_time - last_time).toSec();
	cpose.xAbs += (odom.twist.twist.linear.x * cos(cpose.phiAbs) - odom.twist.twist.linear.y * sin(cpose.phiAbs)) * dt;
	cpose.yAbs += (odom.twist.twist.linear.x * sin(cpose.phiAbs) + odom.twist.twist.linear.y * cos(cpose.phiAbs)) * dt;
	cpose.phiAbs += odom.twist.twist.angular.z * dt;
	odom.pose.pose.position.x = cpose.xAbs;
	odom.pose.pose.position.y = cpose.yAbs;
	odom.pose.pose.position.z = 0;
	odom.pose.pose.orientation = tf::createQuaternionMsgFromYaw(cpose.phiAbs);
	last_time = current_time;
}

void Mecanum4WKinematics::execInvKin(const geometry_msgs::Twist& twist, trajectory_msgs::JointTrajectory& traj)
{
	//make sure that there's no old command in the message.
	traj.joint_names.clear();
	traj.points.clear();
	/* See "Omnidirectional Mobile Robot -Design and Implementation": Ioan Doroftei, Victor Grosu and Veaceslav Spinu

		| w1 |       | 1 -1 -(l1+l2) |   
		| w2 | = 1/R | 1  1  (l1+l2) | . | vx |
		| w3 |       | 1  1 -(l1+l2) |   | vy |
		| w4 |       | 1 -1  (l1+l2) |   | wz |
	*/
	trajectory_msgs::JointTrajectoryPoint point;
	point.velocities.resize(4);
	// w1:
	traj.joint_names.push_back("wheel_front_left_base_link");
	point.velocities[0] = 2 / m_dDiam * ( twist.linear.x - twist.linear.y - (m_dAxis1Length + m_dAxis2Length) / 2 * twist.angular.z); 
	// w2:
	traj.joint_names.push_back("wheel_front_right_base_link");
	point.velocities[1] = 2 / m_dDiam * ( twist.linear.x + twist.linear.y + (m_dAxis1Length + m_dAxis2Length) / 2 * twist.angular.z); 
	// w3:
	traj.joint_names.push_back("wheel_back_left_base_link");
	point.velocities[2] = 2 / m_dDiam * ( twist.linear.x + twist.linear.y - (m_dAxis1Length + m_dAxis2Length) / 2 * twist.angular.z); 
	// w4:
	traj.joint_names.push_back("wheel_back_right_base_link");
	point.velocities[3] = 2 / m_dDiam * ( twist.linear.x - twist.linear.y + (m_dAxis1Length + m_dAxis2Length) / 2 * twist.angular.z); 
	traj.points.push_back(point);
	
}



void Mecanum4WKinematics::setAxis1Length(double dLength)
{
	m_dAxis1Length = dLength;
}

void Mecanum4WKinematics::setAxis2Length(double dLength)
{
	m_dAxis2Length = dLength;
}


void Mecanum4WKinematics::setWheelDiameter(double dDiam)
{
	m_dDiam = dDiam;
}
