#include "../../../communications.h"
#include <thread>
#include <iostream>

namespace
{
  using namespace common::udp::multicast;
  class echo_client
    : public common::interface<echo_client>
  {
    public:
      echo_client(common::udp_multicast_params_t& a_params, boost::asio::io_service& a_io_service)
        : m_client(create_client(a_params, a_io_service))
      {
        m_client->set_on_data([](const char *a_data, std::size_t len)
         {
           std::cout << "recv: ";
           for(int i = 0; i < (int)len; i++)
            printf("%02X ", *(unsigned char*) (a_data + i));
           std::cout << std::endl;
         });

        m_client->run();
      }

      static echo_client::ref create_echo_client(common::udp_multicast_params_t& a_params, boost::asio::io_service& a_io_service)
      {
        return std::make_shared<echo_client>(a_params, a_io_service);
      }

    private:
      iclient::ref m_client;
  };
}

int main(int argc, char** argv)
{
  common::udp_multicast_params_t params;
  params.source_ip = "91.203.253.232";
  params.group_ip = "239.195.5.17";
  params.port = 36017;
  params.interface_name = "bond0.233";
  params.use_strand = true;

  boost::asio::io_service io_service;

  auto client = echo_client::create_echo_client(params, io_service);

  std::vector<std::thread> tgroup;
  int cores_count = std::thread::hardware_concurrency();
  for(int i = 0; i < cores_count; i++)
  {
    tgroup.emplace_back(std::thread([&io_service](){
      io_service.run();
    }));
  }

  for(auto &thr: tgroup)
    thr.join();

  return 0;
}
