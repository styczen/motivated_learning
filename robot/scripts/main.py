#!/usr/bin/env python3
import rospy

from gazebo_msgs.msg import LinkStates
from robot_msgs.msg import PainsMsg

from robot.agent import Agent
from robot.pains import Pains
from robot.robot_pose_manager import RobotPoseManager
from environment.chargers_manager import ChargersManager
from robot.movement_manager import MovementManagerClient


def check_simulation_state():
    # if this function returns gazebo is running
    rospy.logdebug('check simulation state')
    link_states = rospy.wait_for_message('/gazebo/link_states', LinkStates)


if __name__ == '__main__':
    try:
        rospy.init_node('robot', anonymous=True, log_level=rospy.INFO)

        chargers_manager = ChargersManager()
        robot_pose_manager = RobotPoseManager()
        movement_mngr_client = MovementManagerClient()

        agent = Agent()
        pains = Pains()

        rospy.loginfo('Started node with agent state updaters.')
        rate = rospy.Rate(10)
        while not rospy.is_shutdown():
            check_simulation_state()  # This function only freeze running of loop when Gazebo simulation is paused

            agent.step()
            current_pains, dominant_pain = pains.step()
            # print(current_pains)
            print(dominant_pain)

            goal_coords = agent.action(dominant_pain)

            movement_mngr_client.send_goal(goal_coords[0], goal_coords[1])

            robot_pose_manager.step()
            chargers_manager.step()

            rate.sleep()
    except rospy.ROSInterruptException as e:
        rospy.logerr(e)

    # check current position - pains, get state
    #
