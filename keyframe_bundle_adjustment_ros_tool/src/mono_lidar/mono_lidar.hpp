#pragma once

#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/publisher.h>
#include <dynamic_reconfigure/server.h>
#include <ros/ros.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <matches_msg_depth_ros/MatchesMsgWithOutlierFlag.h>
#include <sensor_msgs/CameraInfo.h>

#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/exact_time.h>

#include <Eigen/Eigen>
#include <keyframe_bundle_adjustment/bundle_adjuster_keyframes.hpp>
#include <keyframe_bundle_adjustment/keyframe_selector.hpp>

#include "keyframe_bundle_adjustment_ros_tool/MonoLidarInterface.h"

// MC : Added for mkdir function
#include <sys/stat.h>

// Forward declaration camera model;
namespace image_geometry
{
    class PinholeCameraModel;
}

namespace keyframe_bundle_adjustment_ros_tool
{

    class MonoLidar
    {

        using Interface = MonoLidarInterface;
        using ReconfigureConfig = MonoLidarConfig;
        using ReconfigureServer = dynamic_reconfigure::Server<ReconfigureConfig>;

        using TrackletsMsg = matches_msg_depth_ros::MatchesMsgWithOutlierFlag;
        using CameraInfoMsg = sensor_msgs::CameraInfo;

        using SyncPolicy = message_filters::sync_policies::ExactTime<TrackletsMsg, CameraInfoMsg>;
        using Synchronizer = message_filters::Synchronizer<SyncPolicy>;

    public:
        MonoLidar(ros::NodeHandle, ros::NodeHandle);

        // Define destructor for writing down sequence with points for plotting.
        ~MonoLidar();

        // MC : Added
        // /////////////////////////////////////////////////////////////////////////////////////////////////////////
        // MC : Start Add
        //MC : Added
        int createPath_mc(mode_t mode, const std::string &rootPath, std::string &path)
        {
            struct stat st;

            for (std::string::iterator iter = path.begin(); iter != path.end();)
            {
                std::string::iterator newIter = std::find(iter, path.end(), '/');
                std::string newPath = rootPath + "/" + std::string(path.begin(), newIter);

                if (stat(newPath.c_str(), &st) != 0)
                {
                    if (mkdir(newPath.c_str(), mode) != 0 && errno != EEXIST)
                    {
                        std::cerr << "cannot create folder [" << newPath << "] : " << strerror(errno) << std::endl;
                        return -1;
                    }
                }
                else if (!S_ISDIR(st.st_mode))
                {
                    errno = ENOTDIR;
                    std::cerr << "path [" << newPath << "] not a dir " << std::endl;
                    return -1;
                }
                else
                    std::cerr << "path [" << newPath << "] already exists " << std::endl;

                iter = newIter;
                if (newIter != path.end())
                    ++iter;
            }
            return 0;
        }
        // MC : Finish add : createPath_mc()
        // /////////////////////////////////////////////////////////////////////////////////////////////////////////

        // /////////////////////////////////////////////////////////////////////////////////////////////////////////
        // MC : Start Add
        // MC : Added
        void createTrajectoryDirectories(std::string &FinalOutputDir)
        {

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char dateTimeBuf[100];
            std::string dateTime_mc;
            std::string TmpOutputDir_mc = "/home/chghaf/MC/Z_RESULTS/LIMO";

            // look if we can operate with given emplacement
            std::strftime(dateTimeBuf, sizeof(dateTimeBuf), "%y%m%d_%H%M%S", std::localtime(&in_time_t));
            dateTime_mc = dateTimeBuf;

            createPath_mc(0777, TmpOutputDir_mc, dateTime_mc);

            FinalOutputDir = TmpOutputDir_mc + std::string("/") + dateTime_mc;
        }
        // MC : Finish add : createTrajectoryDirectories()
        // /////////////////////////////////////////////////////////////////////////////////////////////////////////
        // //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // MC : Added
        std::string FinalOutputDir = "";
        std::string path_file = "";

        // MC : Finish Added
        // /////////////////////////////////////////////////////////////////////////////////////////////////////////

    private:
        void setupDiagnostics();
        void checkSensorStatus(diagnostic_updater::DiagnosticStatusWrapper &);

        ///@brief process the input from ros, execute whatever, publish it
        void callbackSubscriber(const TrackletsMsg::ConstPtr &, const CameraInfoMsg::ConstPtr &camera_info_msg);

        void reconfigureRequest(const ReconfigureConfig &, uint32_t);

        Interface interface_;
        ReconfigureServer reconfigure_server_;

        tf2_ros::Buffer tf_buffer_;
        tf2_ros::TransformListener tf_listener_;
        tf2_ros::TransformBroadcaster tf_broadcaster_;

        diagnostic_updater::Updater updater_; ///< Diagnostic updater
        // std::unique_ptr<diagnostic_updater::DiagnosedPublisher<std_msgs::Header>>
        // publisherDiagnosed_;
        diagnostic_msgs::DiagnosticStatus diagnostic_status_; ///< Current diagnostic status

        ///@brief sync subscribers
        std::unique_ptr<Synchronizer> sync_;

        ///////////////////////////////////////
        /// \brief bundle_adjuster_ptr_, class for doing the bundle adjustment
        ///
        keyframe_bundle_adjustment::BundleAdjusterKeyframes::Ptr bundle_adjuster_;

        //////////////////////////////////////////////////
        /// \brief keyframe_selector_ptr_, selection for keyframes
        ///
        keyframe_bundle_adjustment::KeyframeSelector keyframe_selector_;

        /**
     * @brief maybeSendPoseTf, if tf frame ids are set in interface, pusblish last keyframe pose in
     * tf
     * @param timestamp, timestamp at which it shall be published
     * @param last_pose, pose toi be published
     */
        void maybeSendPoseTf(ros::Time timestamp, Eigen::Isometry3d pose);

        /**
     * @brief getPrior, get prior from keyframe to origin (according to our convention).
     * @param model, camera model
     * @param tracklets
     * @return
     */
        Eigen::Isometry3d getPrior(const image_geometry::PinholeCameraModel &model,
                                   const ros::Time &cur_ts_ros,
                                   const keyframe_bundle_adjustment::Tracklets &tracklets);

    private:                                       // attributes
        Eigen::Isometry3d trf_camera_vehicle;      ///< extrinsic calibraion of camera defined from vechicle
                                                   /// frame to camera frame
        Eigen::Isometry3d last_pose_origin_camera; ///< Last pose
        Eigen::Isometry3d accumulated_motion;      ///< Accumulated motion
        ros::Time last_ts_solved_{0.0};            ///< Last timestamp when bundle_adjuster_.solve was called
    };
} // namespace keyframe_bundle_adjustment_ros_tool
