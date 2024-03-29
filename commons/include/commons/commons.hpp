#ifndef COMMONS_HPP
#define COMMONS_HPP

#include <chrono>
#include <string>
#include <ros/ros.h>

namespace commons
{
    using clock = std::chrono::system_clock;
    static std::chrono::time_point<clock> now()
    {
        return clock::now();
    }

    static double elapsed(std::chrono::time_point<clock> stop, std::chrono::time_point<clock> start)
    {
        return std::chrono::duration<double, std::milli>(stop - start).count();
    }

    struct Timer
    {
        std::string function_name;
        std::chrono::time_point<clock> start;

        Timer(std::string function_name = "Function")
            : function_name(function_name)
        {
            start = now();
        }

        ~Timer()
        {
            auto duration = std::chrono::duration<double, std::milli>(now() - start);
            ROS_DEBUG_STREAM(function_name << " took: " << duration.count() << " ms");
        }
    };

    struct Topics
    {
        inline static std::string pains = "/pains";
        inline static std::string action = "/action";
        inline static std::string rgb = "/camera/rgb/image_raw";
        inline static std::string laser_scan = "/scan";
        inline static std::string joint_states = "/joint_states";
        inline static std::string model_states = "/gazebo/model_states";
    };

    struct Models
    {
        inline static std::string turtlebot3 = "turtlebot3_waffle";
    };

    struct GazeboServices
    {
        inline static std::string model_state = "/gazebo/set_model_state";  
    };

    // struct Frames
    // {
    //     inline static std::string robot = "robot";
    // };

} // namespace commons

#endif
