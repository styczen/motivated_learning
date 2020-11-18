#include "sensory_processing/sensory_processing.hpp"

namespace ml
{
    SensoryProcessing::SensoryProcessing(ros::NodeHandle &nh)
        : _nh(nh), _prev_left_wheel_joint_angle(0), _prev_right_wheel_joint_angle(0)
    {
        _pains_pub = _nh.advertise<robot_msgs::Pains>(commons::Topics::pains, 1, true);
        _pains = boost::make_shared<robot_msgs::Pains>();
        _pains->header.frame_id = "base_footprint";
        _prev_image = boost::make_shared<sensor_msgs::Image>();
        _prev_action = boost::make_shared<robot_msgs::Action>();
        CheckTopics();
    }

    void SensoryProcessing::ProcessSensoryInputs()
    {
        std::future<bool> wheels_state = std::async(std::launch::async, &SensoryProcessing::WheelsStateChanged, this);
        std::future<bool> close_to_obstacle = std::async(std::launch::async, &SensoryProcessing::CloseToObstacle, this);
        std::future<bool> curiosity = std::async(std::launch::async, &SensoryProcessing::Curiosity, this);

        // Low battery level pain
        if (wheels_state.get())
        {
            ROS_DEBUG("Wheels state changed. Pain of low battery level increases.");
            _pains->low_battery_level += PainIncrements::low_battery_level;
        }

        // Mechanical damage pain
        if (close_to_obstacle.get())
        {
            ROS_DEBUG("Robot is too close to some obstacle e.g. wall. Pain of mechanical damage increases.");
            _pains->mechanical_damage += PainIncrements::low_battery_level;
        }

        // Curiosity
        if (curiosity.get())
        {
            ROS_DEBUG("Something with curiosity happened. Do something boiii.");
            _pains->curiosity += PainIncrements::curiosity;
        }

        _pains->header.seq++;
        _pains->header.stamp = ros::Time::now();

        _pains_pub.publish(_pains);
    }

    bool SensoryProcessing::WheelsStateChanged()
    {
        commons::Timer timer("SensoryProcessing::_WheelsStateChanged");

        sensor_msgs::JointStateConstPtr wheels_state = ros::topic::waitForMessage<sensor_msgs::JointState>(
            commons::Topics::joint_states);

        auto left_wheel_joint_name_it = std::find(wheels_state->name.begin(), wheels_state->name.end(),
                                                  "wheel_left_joint");
        if (left_wheel_joint_name_it == wheels_state->name.end())
        {
            ROS_FATAL("Missing left wheel joint state. Exiting...");
            std::exit(EXIT_FAILURE);
        }

        auto right_wheel_joint_name_it = std::find(wheels_state->name.begin(), wheels_state->name.end(),
                                                   "wheel_right_joint");
        if (right_wheel_joint_name_it == wheels_state->name.end())
        {
            ROS_FATAL("Missing right wheel joint state. Exiting...");
            std::exit(EXIT_FAILURE);
        }

        float left_wheel_joint_angle = wheels_state->position[left_wheel_joint_name_it - wheels_state->name.begin()];
        float right_wheel_joint_angle = wheels_state->position[right_wheel_joint_name_it - wheels_state->name.begin()];
        // ROS_DEBUG_STREAM("\nl: " << left_wheel_joint_angle << "\nr: " << right_wheel_joint_angle);

        bool left_changed = std::fabs(left_wheel_joint_angle - _prev_left_wheel_joint_angle) > WHEEL_THRESHOLD;
        bool right_changed = std::fabs(right_wheel_joint_angle - _prev_right_wheel_joint_angle) > WHEEL_THRESHOLD;

        _prev_left_wheel_joint_angle = left_wheel_joint_angle;
        _prev_right_wheel_joint_angle = right_wheel_joint_angle;

        return left_changed || right_changed;
    }

    bool SensoryProcessing::CloseToObstacle()
    {
        commons::Timer timer("SensoryProcessing::_CloseToObstacle");

        sensor_msgs::LaserScanConstPtr scan = ros::topic::waitForMessage<sensor_msgs::LaserScan>(
            commons::Topics::laser_scan);
        if (scan == nullptr)
        {
            ROS_FATAL("There is no laser scan data. Exiting...");
            std::exit(EXIT_FAILURE);
        }

        bool too_close = false;
        for (float range : scan->ranges)
        {
            if (range != std::numeric_limits<float>::infinity())
            {
                if (range < RANGE_THRESHOLD)
                    too_close = true;
                break;
            }
        }
        return too_close;
    }

    bool SensoryProcessing::Curiosity()
    {
        commons::Timer timer("SensoryProcessing::_Curiosity");

        sensor_msgs::ImageConstPtr rgb = ros::topic::waitForMessage<sensor_msgs::Image>(commons::Topics::rgb);
        if (rgb == nullptr)
        {
            ROS_FATAL("There is no video feed. Exiting...");
            std::exit(EXIT_FAILURE);
        }

        auto current_cv_ptr = cv_bridge::toCvShare(rgb);
        cv::Mat current_gray;
        cv::cvtColor(current_cv_ptr->image, current_gray, cv::COLOR_RGB2GRAY);

        int ones = 0;
        if (!_prev_image->data.empty())
        {
            auto prev_cv_ptr = cv_bridge::toCvShare(_prev_image);
            cv::Mat prev_gray;
            cv::cvtColor(prev_cv_ptr->image, prev_gray, cv::COLOR_RGB2GRAY);

            cv::Mat diff;
            cv::absdiff(current_gray, prev_gray, diff);

            cv::Mat mask;
            cv::threshold(diff, mask, 1, 255, cv::THRESH_BINARY);

            ones = cv::countNonZero(mask);
            // cv::imshow("mask", mask);
            // cv::waitKey(1);
        }
        _prev_image = rgb;
        bool scene_changed = ones < MIN_NON_ZERO;

        // Check action
        bool action_changed = false;
        robot_msgs::ActionConstPtr action = ros::topic::waitForMessage<robot_msgs::Action>(commons::Topics::action,
                                                                                           ros::Duration(0.1));
        if (action != nullptr)
            action_changed = action->action != _prev_action->action;

        return scene_changed || action_changed;
    }

    bool SensoryProcessing::CheckTopics()
    {
        ROS_INFO("Checking for available topics...");
        bool ready = false;
        size_t iter = 0;
        while (!ready)
        {
            ros::master::V_TopicInfo topic_infos;
            ros::master::getTopics(topic_infos);
            for (auto e : topic_infos)
            {
                if (e.name == commons::Topics::joint_states ||
                    e.name == commons::Topics::rgb ||
                    e.name == commons::Topics::laser_scan)
                {
                    ready = true;
                    break;
                }
            }
            std::cout << iter++ << " " << ready << std::endl;
            if (iter > 5)
            {
                ROS_FATAL("Topics not available. Exiting...");
                std::exit(EXIT_FAILURE);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return true;
    }

}; // namespace ml
