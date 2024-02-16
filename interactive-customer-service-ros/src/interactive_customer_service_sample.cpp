#include <ros/ros.h>
#include <std_msgs/String.h>
#include <interactive_customer_service/Conversation.h>
#include <interactive_customer_service/RobotStatus.h>
#include <nodelet/nodelet.h>

class InteractiveCustomerServiceSample
{
private:
  enum Step
  {
    Initialize,
    Ready,
    WaitForInstruction,
    SendMessage,
    WaitForCustomerMessage,
    TakeItem,
    WaitToTakeItem,
    GiveItem,
    WaitForResult,
  };

  const std::string MSG_ARE_YOU_READY       = "Are_you_ready?";
  const std::string MSG_CUSTOMER_MESSAGE    = "customer_message";
  const std::string MSG_ROBOT_MSG_SUCCEEDED = "robot_message_succeeded";
  const std::string MSG_ROBOT_MSG_FAILED    = "robot_message_failed";
  const std::string MSG_TAKE_ITEM_SUCCEEDED = "take_item_succeeded";
  const std::string MSG_TAKE_ITEM_FAILED    = "take_item_failed";
  const std::string MSG_GIVE_ITEM_SUCCEEDED = "give_item_succeeded";
  const std::string MSG_GIVE_ITEM_FAILED    = "give_item_failed";
  const std::string MSG_TASK_SUCCEEDED      = "Task_succeeded";
  const std::string MSG_TASK_FAILED         = "Task_failed";
  const std::string MSG_MISSION_COMPLETE    = "Mission_complete";
  const std::string MSG_YES                 = "Yes";
  const std::string MSG_NO                  = "No";
  const std::string MSG_I_DONT_KNOW         = "I don't know";

  const std::string MSG_I_AM_READY     = "I_am_ready";
  const std::string MSG_ROBOT_MESSAGE  = "robot_message";
  const std::string MSG_TAKE_ITEM      = "take_item";
  const std::string MSG_GIVE_ITEM      = "give_item";
  const std::string MSG_GIVE_UP        = "Give_up";
  
  const std::string STATE_STANDBY              = "standby";
  const std::string STATE_IN_CONVERSATION      = "in_conversation";
  const std::string STATE_MOVING               = "moving";
  
  int step_;

  std::string instruction_msg_;
  std::string customer_msg_;
  std::string robot_state_;
  bool        speaking_;
  std::string grasped_item_;

  bool is_started_;
  bool is_succeeded_;
  bool is_failed_;

  void reset()
  {
	ROS_INFO("Reset Parameters");
	  
    instruction_msg_ = "";
    customer_msg_    = "";
    robot_state_     = "";
    speaking_        = false;
    grasped_item_    = "";
    is_started_   = false;
    is_succeeded_ = false;
    is_failed_    = false;
  }


  void messageCallback(const interactive_customer_service::Conversation::ConstPtr& message)
  {
    ROS_INFO("Subscribe message:%s, %s", message->type.c_str(), message->detail.c_str());

    if(message->type.c_str()==MSG_ARE_YOU_READY)
    {
      if(step_==Ready)
      {
        is_started_ = true;
      }
    }
    if(message->type.c_str()==MSG_CUSTOMER_MESSAGE)
    {
      if(step_==WaitForInstruction)
      {
        instruction_msg_ = message->detail.c_str();
      }
      if(step_==WaitForCustomerMessage)
      {
        customer_msg_ = message->detail.c_str();
      }
    }
    if(message->type.c_str()==MSG_TAKE_ITEM_FAILED || message->type.c_str()==MSG_GIVE_ITEM_FAILED)
    {
      is_failed_ = true;
    }
    if(message->type.c_str()==MSG_TASK_SUCCEEDED)
    {
      is_succeeded_ = true;
    }
    if(message->type.c_str()==MSG_TASK_FAILED)
    {
      is_failed_ = true;
    }
    if(message->type.c_str()==MSG_MISSION_COMPLETE)
    {
      exit(EXIT_SUCCESS);
    }
  }

  void robotStatusCallback(const interactive_customer_service::RobotStatus::ConstPtr& status)
  {
    ROS_DEBUG("Subscribe robot status:%s, %d, %s", status->state.c_str(), status->speaking, status->grasped_item.c_str());

    robot_state_  = status->state.c_str();
    speaking_     = status->speaking;
    grasped_item_ = status->grasped_item.c_str();
  }

  void sendMessage(ros::Publisher &publisher, const std::string &type, const std::string &detail="")
  {
    ROS_INFO("Send message:%s, %s", type.c_str(), detail.c_str());

    interactive_customer_service::Conversation message;
    message.type   = type;
    message.detail = detail;
    publisher.publish(message);
  }


public:
  int run(int argc, char **argv)
  {
    ros::NodeHandle node_handle;

    ros::Rate loop_rate(10);

    std::string sub_customer_msg_topic_name;
    std::string pub_robot_msg_topic_name;
    std::string sub_robot_status_topic_name;

    node_handle.param<std::string>("sub_customer_msg_topic_name", sub_customer_msg_topic_name, "/interactive_customer_service/message/customer");
    node_handle.param<std::string>("pub_robot_msg_topic_name",    pub_robot_msg_topic_name,    "/interactive_customer_service/message/robot");
    node_handle.param<std::string>("sub_robot_status_topic_name", sub_robot_status_topic_name, "/interactive_customer_service/robot_status");

    ros::Time waiting_start_time;

    ROS_INFO("Interactive Customer Service sample start!");

    ros::Subscriber sub_msg   = node_handle.subscribe<interactive_customer_service::Conversation>(sub_customer_msg_topic_name, 100, &InteractiveCustomerServiceSample::messageCallback, this);
    ros::Publisher  pub_msg   = node_handle.advertise<interactive_customer_service::Conversation>(pub_robot_msg_topic_name, 10);
    ros::Subscriber sub_state = node_handle.subscribe<interactive_customer_service::RobotStatus >(sub_robot_status_topic_name, 100, &InteractiveCustomerServiceSample::robotStatusCallback, this);

    reset();
    step_ = Initialize;

    while (ros::ok())
    {
      if(is_succeeded_)
      {
        ROS_INFO("Task succeeded!");
        step_ = Initialize;
      }

      if(is_failed_)
      {
        ROS_INFO("Task failed!");
        step_ = Initialize;
      }

      switch(step_)
      {
        case Initialize:
        {
          reset();
          step_++;
          break;
        }
        case Ready:
        {
          if(is_started_)
          {
            sendMessage(pub_msg, MSG_I_AM_READY);

            ROS_INFO("Task start!");

            step_++;
          }
          break;
        }
        case WaitForInstruction:
        {
          if(instruction_msg_!="")
          {
            ROS_INFO("Instruction: %s", instruction_msg_.c_str());

            step_++;
          }
          break;
        }
        case SendMessage:
        {
		  if(robot_state_==STATE_IN_CONVERSATION)
          {
            sendMessage(pub_msg, MSG_ROBOT_MESSAGE, "There are several candidates. Is it blue?");
            step_++;
          }
          break;
        }
        case WaitForCustomerMessage:
        {
          if(customer_msg_!="")
          {
            step_++;
          }

          break;
        }
        case TakeItem:
        {
          if(robot_state_==STATE_IN_CONVERSATION)
          {
            if(customer_msg_==MSG_YES)
            {
              sendMessage(pub_msg, MSG_TAKE_ITEM, "xylitol_freshmint");
              step_++;
            }
            else
            {
              sendMessage(pub_msg, MSG_TAKE_ITEM, "pie_no_mi");
              step_++;
            }			  
          }

          break;
        }
        case WaitToTakeItem:
        {
          if(robot_state_==STATE_IN_CONVERSATION && grasped_item_!="")
          {
            step_++;
          }

          break;
        }
        case GiveItem:
        {
          sendMessage(pub_msg, MSG_GIVE_ITEM);
          step_++;

          break;
        }
        case WaitForResult:
        {
          break;
        }
      }

      ros::spinOnce();

      loop_rate.sleep();
    }

    return EXIT_SUCCESS;
  }
};


int main(int argc, char **argv)
{
  ros::init(argc, argv, "interactive_customer_service_sample");

  InteractiveCustomerServiceSample sample;
  return sample.run(argc, argv);
};
