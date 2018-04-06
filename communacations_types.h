#pragma once

namespace common
{
  const int BUF_LENGTH = 32768;
  using buf_t = std::array<char, BUF_LENGTH>;
  using pbuf_t = std::unique_ptr<buf_t>;

  enum class read_func_type_e
  {
      custom_eol,
      read_until_eol,
      async_read_some_eol
  };

  struct tcp_server_params_t
  {
    read_func_type_e do_read_type;
    bool use_strand;
  };

} //namespace common
