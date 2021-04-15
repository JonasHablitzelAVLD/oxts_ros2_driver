/**
 * \file driver.hpp
 * Defines node to take NCom data and publish it in ROS messages.
 */

#ifndef OXTS_DRIVER__DRIVER_HPP_
#define OXTS_DRIVER__DRIVER_HPP_

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>
#include <fstream>

// ROS includes
#include "rclcpp/rclcpp.hpp"
#include <oxts_msgs/msg/ncom.hpp>

// Boost includes
#include <boost/asio.hpp>

// gad-sdk includes
#include "oxts_driver/NComRxC.h"
#include "oxts_driver/nav_const.hpp"
#include "oxts_driver/udp_server_client.h"

using namespace std::chrono_literals;

namespace oxts_driver
{

/**
 * This class creates a subclass of Node designed to take NCom data from the 
 * NCom decoder and publish it to pre-configured ROS topics.
 * 
 * @todo Add config struct to hold data which will hold config parsed from the 
 *       .yaml file.
 */
class OxtsDriver : public rclcpp::Node
{
private:
  /*! Rate at which to sample NCom. Expected that this will typically match
    the rate of NCom itself, though can be set lower to save computation. */
  uint32_t ncom_rate;
  /*! IP address of the INS to connect to */
  std::string unit_ip;
  /*! Endpoint Port of the INS to be connected to. Default 3000 for NCom. */
  int unit_port;
  /*! File path to NCom file to be used as input. Not required if running
    in real time. */
  std::string ncom_path;
  /*! Function pointer to the necesary NCom file/socket callback */
  void (oxts_driver::OxtsDriver::*timer_ncom_callback)();
  /*! Function pointer to the necesary NCom updater */
  void (oxts_driver::OxtsDriver::*update_ncom)();
  /*! Whether ot not to wait for NCom initialisation before publishing messages. */
  bool wait_for_init;
  /*! Publishing rate for debug String message. */

  std::chrono::duration<uint64_t,std::milli> ncomInterval;
  double prevWeekSecond;

  rclcpp::TimerBase::SharedPtr timer_ncom_;

  /**
   * Callback function for NCom sampling. Receives data from chosen source
   * (UDP or file) and parses a packet to nrx.
   * 
   * @todo Refactor into input class
   */
  void timer_ncom_socket_callback();
  void timer_ncom_file_callback();
  void get_file_packet();
  void get_socket_packet();
  void publish_packet();

  /**
   * Publisher for std_msgs/msg/string. Only used for debugging, currently 
   * outputs lat, long, alt in string form.
   */
  rclcpp::Publisher<oxts_msgs::msg::Ncom>::SharedPtr pubNCom_;

public:
  /**
   * Default constructor for the OxtsDriver. Parses options from the 
   * .yaml params/config file, sets up UDP connection to unit.
   */
  explicit OxtsDriver(const rclcpp::NodeOptions & options) : Node("oxts_driver", options)
  {
    // Get parameters (from config, command line, or from default)
    // Initialise configurable parameters (all params should have defaults)
    ncom_rate               = this->declare_parameter("ncom_rate", 100);
    unit_ip                 = this->declare_parameter("unit_ip", "0.0.0.0");
    unit_port               = this->declare_parameter("unit_port", 3000);
    ncom_path               = this->declare_parameter("ncom", std::string(""));
    wait_for_init           = this->declare_parameter("wait_for_init", true);

    ncomInterval            = std::chrono::milliseconds(int(1000.0 / ncom_rate));
    prevWeekSecond          = -1;

    // Initialise publishers for each message - all are initialised, even if not
    // configured
    pubNCom_ = this->create_publisher<oxts_msgs::msg::Ncom> ("ncom", 10); 


    nrx = NComCreateNComRxC();

    if (!ncom_path.empty())
    {
      inFileNCom.open(ncom_path);
      if(!inFileNCom.is_open())
      {
        RCLCPP_ERROR(this->get_logger(), "Unable to open NCOM: %s", ncom_path.c_str());
        return;
      } else {
        RCLCPP_INFO(this->get_logger(), "Opened NCOM: %s", ncom_path.c_str());
      }
    }
    else 
    {
      unitEndpointNCom = boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string(this->unit_ip), this->unit_port);

      this->udpClient.set_local_port(this->unit_port);
      RCLCPP_INFO(this->get_logger(), "Connecting: %s:%d", this->unit_ip.c_str(), this->unit_port);
    }

    // Assign callback functions to timers (callbacks are called at a rate
    // dictated by the associated timer)
    if (!ncom_path.empty())
    {
      timer_ncom_callback = &OxtsDriver::timer_ncom_file_callback;
      update_ncom = &OxtsDriver::get_file_packet;
    }
    else 
    {
      timer_ncom_callback = &OxtsDriver::timer_ncom_socket_callback;
      update_ncom = &OxtsDriver::get_socket_packet;
    }

    // Wait for config to be populated in NCOM packets
    RCLCPP_INFO(this->get_logger(), "Waiting for INS config information...");
    while (nrx->mSerialNumber == 0 || nrx->mIsImu2VehHeadingValid == 0)
    {
      (*this.*update_ncom)();
    }
    RCLCPP_INFO(this->get_logger(), "INS config information received");

    // Wait for INS initialisation if option enabled
    if (wait_for_init)
    {
      RCLCPP_INFO(this->get_logger(), "Waiting for initialisation...");
      // Only block things that are required for 100% of OxTS navigation
      while (
        nrx->mInsNavMode != NAV_CONST::NAV_MODE::REAL_TIME &&
        nrx->mIsLatValid == 0 &&
        nrx->mIsLonValid == 0 &&
        nrx->mIsAltValid == 0 &&
        nrx->mIsHeadingValid == 0 &&
        nrx->mIsPitchValid == 0 &&
        nrx->mIsRollValid == 0
      )
      {
        (*this.*update_ncom)();
      }
      RCLCPP_INFO(this->get_logger(), "INS initialised");
    }
    else
    {
      RCLCPP_INFO(this->get_logger(), "Publishing before INS initialisation");
    }

    timer_ncom_ = this->create_wall_timer(
                  ncomInterval, std::bind(timer_ncom_callback, this));

    RCLCPP_INFO(this->get_logger(), "Publishing NCom packets at: %iHz", ncom_rate);

  }

  /** NCom decoder instance */
  NComRxC *nrx;
  /** Buffer for UDP data */
  unsigned char buff[1024];
  /** UDP Client to receive data from the device */
  networking_udp::client udpClient;
  /** Endpoint for the udpClient to receive data from */
  boost::asio::ip::udp::endpoint unitEndpointNCom;

  std::fstream inFileNCom;

  rclcpp::Time get_timestamp();
  /**
   * Get the IP address of the OxTS unit, as set in the .yaml params file
   * 
   * @returns IP address as a string
   */
  std::string get_unit_ip();
  /**
   * Get the endpoint port of the OxTS unit, as set in the .yaml params file
   * 
   * @returns Port as a short
   */
  short get_unit_port();

};

} // namespace oxts_driver

#endif //OXTS_DRIVER__DRIVER_HPP_
