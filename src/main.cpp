#include <ros/ros.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>

#include <boost/foreach.hpp>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>    
#include <message_filters/sync_policies/approximate_time.h> 

#include <sensor_msgs/Image.h>
#include <sensor_msgs/CameraInfo.h>
              
/**
 * Inherits from message_filters::SimpleFilter<M>
 * to use protected signalMessage function 
 */

typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::Image,                         
                                                        sensor_msgs::Image,
                                                        sensor_msgs::CameraInfo,
                                                        sensor_msgs::CameraInfo> NoCloudSyncPolicy;

template <class M>
class BagSubscriber : public message_filters::SimpleFilter<M>         
{   
  public:
  void newMessage(const boost::shared_ptr<M const> &msg) 
  {
    this->signalMessage(msg); //"this->" is required as of ros groovy
  } 
};  

// Callback for synchronized messages
void callback(const sensor_msgs::Image::ConstPtr &l_img, 
              const sensor_msgs::Image::ConstPtr &r_img, 
              const sensor_msgs::CameraInfo::ConstPtr &l_info,
              const sensor_msgs::CameraInfo::ConstPtr &r_info)
{
  ROS_INFO("CALLBACK");
  //StereoData sd(l_img, r_img, l_info, r_info);

  // Stereo dataset is class variable to store data
  //stereo_dataset_.push_back(sd);
}
 
// Load bag
void loadBag(const std::string &filename)
{
  
  rosbag::Bag bag;
  bag.open(filename, rosbag::bagmode::Read);
  
 // std::string l_cam = image_ns_ + "/left";
 // std::string r_cam = image_ns_ + "/right";
  std::string l_cam_image =  "/camera/rgb/image_color";   
  std::string r_cam_image ="/camera/depth/image";
  std::string l_cam_info = "/camera/rgb/camera_info";
  std::string r_cam_info = "/camera/depth/camera_info";

  // Image topics to load
  std::vector<std::string> topics;
  topics.push_back(l_cam_image);
  topics.push_back(r_cam_image);
  topics.push_back(l_cam_info);
  topics.push_back(r_cam_info);
  
  rosbag::View view(bag, rosbag::TopicQuery(topics));
  
  // Set up fake subscribers to capture images
  BagSubscriber<sensor_msgs::Image> l_img_sub, r_img_sub;
  BagSubscriber<sensor_msgs::CameraInfo> l_info_sub, r_info_sub;
  message_filters::Synchronizer<NoCloudSyncPolicy>* no_cloud_sync_;
  no_cloud_sync_ = new message_filters::Synchronizer<NoCloudSyncPolicy>(NoCloudSyncPolicy(4),  l_img_sub, r_img_sub, l_info_sub, r_info_sub);
  // Use time synchronizer to make sure we get properly synchronized images
  
  no_cloud_sync_->registerCallback(boost::bind(&callback, _1, _2, _3,_4));  

  
  // Load all messages into our stereo dataset
  BOOST_FOREACH(rosbag::MessageInstance const m, view)
  {
    usleep(10);
    if(!ros::ok()) return;
    if (m.getTopic() == l_cam_image || ("/" + m.getTopic() == l_cam_image))
    {
      sensor_msgs::Image::ConstPtr l_img = m.instantiate<sensor_msgs::Image>();
      if (l_img != NULL)
        l_img_sub.newMessage(l_img);
    }
    
    if (m.getTopic() == r_cam_image || ("/" + m.getTopic() == r_cam_image))
    {
      sensor_msgs::Image::ConstPtr r_img = m.instantiate<sensor_msgs::Image>();
      if (r_img != NULL)
        r_img_sub.newMessage(r_img);
    }
    
    if (m.getTopic() == l_cam_info || ("/" + m.getTopic() == l_cam_info))
    {
      sensor_msgs::CameraInfo::ConstPtr l_info = m.instantiate<sensor_msgs::CameraInfo>();
      if (l_info != NULL)
        l_info_sub.newMessage(l_info);
    }
    
    if (m.getTopic() == r_cam_info || ("/" + m.getTopic() == r_cam_info))
    {
      sensor_msgs::CameraInfo::ConstPtr r_info = m.instantiate<sensor_msgs::CameraInfo>();
      if (r_info != NULL)
        r_info_sub.newMessage(r_info);
    }
   
  }
  bag.close();
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "listener");

  ros::NodeHandle nh;

  std::string filename = "/home/leone/Documents/bechmark/rgbd_dataset_freiburg1_desk.bag";
  loadBag(filename);
  
  
  ros::spin();

  return 0;
}
