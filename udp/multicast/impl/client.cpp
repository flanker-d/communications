#include "../../../communications.h"
#include <functional>
#include <iostream>

namespace common
{
  namespace udp
  {
    namespace multicast
    {
      using boost::asio::ip::udp;
      using group_source_req_t = struct group_source_req;

      struct mcast_join_source_group
      {
          mcast_join_source_group()
            : m_data()
          {}

          mcast_join_source_group(std::string &source_ip, std::string &group_ip, int port, std::string &interface_name)
            : mcast_join_source_group()
          {
            int ifname_number = if_nametoindex(interface_name.c_str());

            struct sockaddr_in group_addr;
            group_addr.sin_addr.s_addr = inet_addr(group_ip.c_str());
            group_addr.sin_port = htons(port);
            group_addr.sin_family = AF_INET;

            struct sockaddr_in source_addr;
            source_addr.sin_addr.s_addr = inet_addr(source_ip.c_str());
            source_addr.sin_port = htons(port);
            source_addr.sin_family = AF_INET;

            memcpy(&(m_data.gsr_source), (struct sockaddr_storage *) &source_addr, sizeof(struct sockaddr_in));
            memcpy(&(m_data.gsr_group), (struct sockaddr_storage *) &group_addr, sizeof(struct sockaddr_in));

            m_data.gsr_interface = ifname_number;
          }

          template<typename Protocol>
          int level(const Protocol&) const
          {
            return IPPROTO_IP;
          }

          template<typename Protocol>
          int name(const Protocol&) const
          {
            return MCAST_JOIN_SOURCE_GROUP;
          }

          template<typename Protocol>
          group_source_req_t* data(const Protocol&)
          {
            return &m_data;
          }

          template<typename Protocol>
          const group_source_req_t* data(const Protocol&) const
          {
            return &m_data;
          }

          template<typename Protocol>
          std::size_t size(const Protocol&) const
          {
            return sizeof(m_data);
          }

          template<typename Protocol>
          void resize(const Protocol&, std::size_t s)
          {
            if (s != sizeof(m_data))
            {
              std::length_error ex("mcast_join_source_group socket option resize");
              boost::asio::detail::throw_exception(ex);
            }
          }

        private:
          group_source_req_t m_data;
      };

      using so_timestamp = boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_TIMESTAMP>;
      using so_recvttl = boost::asio::detail::socket_option::boolean<IPPROTO_IP, IP_RECVTTL>;

      const int buff_size =	16384;
      using buf_array_t = std::array<char, buff_size>;

      class client
        : public iclient
      {
        public:
          client(udp_multicast_params_t& a_params, boost::asio::io_service& a_io_service);
          void run() override;
          void set_on_data(std::function<void(const char *a_data, std::size_t a_len)> a_on_data) override;

        protected:
          void do_receive() override;

        private:
          udp_multicast_params_t m_params;
          boost::asio::io_service::strand m_strand;
          boost::asio::ip::udp::socket m_sock;
          boost::asio::ip::udp::endpoint m_ep;
          std::unique_ptr<buf_array_t> m_buffer = std::make_unique<buf_array_t>();
          std::function<void(const char *a_data, std::size_t a_len)> m_on_data_func;
      };

      client::client(udp_multicast_params_t& a_params, boost::asio::io_service& a_io_service)
        : m_params(a_params)
        , m_strand(a_io_service)
        , m_sock(m_strand.get_io_service())
        , m_ep(boost::asio::ip::address::from_string(a_params.group_ip.c_str()), a_params.port)
      {
        m_sock.open(m_ep.protocol());
        m_sock.set_option(boost::asio::ip::udp::socket::reuse_address(true));
        m_sock.set_option(so_recvttl(true));
        m_sock.set_option(so_timestamp(true));
        m_sock.bind(m_ep);
        m_sock.set_option(mcast_join_source_group(m_params.source_ip, m_params.group_ip, m_params.port, m_params.interface_name));
      }

      void client::run()
      {
        do_receive();
      }

      void client::set_on_data(std::function<void(const char *a_data, std::size_t a_len)> a_on_data)
      {
        m_on_data_func = a_on_data;
      }

      void client::do_receive()
      {
        boost::asio::ip::udp::endpoint ep;

        auto read_handler = [this](boost::system::error_code ec, std::size_t bytes_recvd)
        {
          if(!ec && bytes_recvd > 0)
          {
            if(m_on_data_func != nullptr)
              m_on_data_func(m_buffer.get()->data(), bytes_recvd);
          }
          do_receive();
        };

        if(m_params.use_strand)
          m_sock.async_receive_from(boost::asio::buffer(m_buffer.get()->data(), buff_size), ep, m_strand.wrap(read_handler));
        else
          m_sock.async_receive_from(boost::asio::buffer(m_buffer.get()->data(), buff_size), ep, read_handler);
      }

      iclient::ref create_client(const std::string& a_group_ip, const std::string& a_source_ip, const int a_port, const std::string& a_interface, boost::asio::io_service::strand& a_strand);
    } //namespace multicast
  } //namespace udp
} //namespace common

namespace common
{
  namespace udp
  {
    namespace multicast
    {
      iclient::ref create_client(udp_multicast_params_t& a_params, boost::asio::io_service& a_io_service)
      {
        return std::make_shared<common::udp::multicast::client>(a_params, a_io_service);
      }
    } //namespace multicast
  } //namespace udp
} //namespace common