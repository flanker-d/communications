#pragma once

namespace common
{
  const int BUF_LENGTH = 32768;
  using buf_t = std::array<char, BUF_LENGTH>;
  using pbuf_t = std::unique_ptr<buf_t>;

  enum class read_func_type_e
  {
      completion_eol,
      read_until_eol,
      async_read_some_eol
  };

  struct tcp_server_params_t
  {
    std::string ip = "0.0.0.0";
    std::uint16_t port;
    read_func_type_e do_read_type;
    bool use_strand = false;
  };

  struct tcp_client_params_t
  {
    std::string ip = "127.0.0.1";
    std::uint16_t port;
    read_func_type_e do_read_type;
    bool use_strand;
  };

  struct udp_multicast_params_t
  {
    std::string source_ip;
    std::string group_ip;
    std::uint16_t port;
    std::string interface_name;
    bool use_strand = false;
  };

} //namespace common
