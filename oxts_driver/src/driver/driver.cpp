#include "oxts_driver/driver.hpp"

namespace oxts_driver
{

void OxtsDriver::timer_ncom_socket_callback()
{
  // Read from open socket
  std::size_t size = this->udpClient.receive_from(this->buff, NCOM_PACKET_LENGTH, this->unitEndpointNCom);
  // Add data to decoder
  if (NComNewChars(this->nrx, this->buff, size) == COM_NEW_UPDATE)
  {
    auto msg = oxts_msgs::msg::Ncom();
    msg.header.stamp = this->get_clock()->now();
    msg.header.frame_id = "oxts_sn" + std::to_string(nrx->mSerialNumber);
    for (int i=0; i < NCOM_PACKET_LENGTH ;++i)
      msg.raw_packet[i] = nrx->mInternal->mCurPkt[i];
    this->pubNCom_->publish(msg);
  }
}

void OxtsDriver::timer_ncom_file_callback()
{
  char c;

  while(NComNewChar(this->nrx, (unsigned char) c) != COM_NEW_UPDATE)
    this->inFileNCom.get(c);

  auto msg = oxts_msgs::msg::Ncom();
  msg.header.stamp = this->get_clock()->now();
  msg.header.frame_id = "oxts_sn" + std::to_string(nrx->mSerialNumber);
  for (int i=0; i < NCOM_PACKET_LENGTH ;++i)
    msg.raw_packet[i] = nrx->mInternal->mCurPkt[i];
  this->pubNCom_->publish(msg);
  
}

std::string OxtsDriver::get_unit_ip()
{
  return this->unit_ip;
}

short       OxtsDriver::get_unit_port()
{
  return this->unit_port;
}

} // namespace oxts_driver