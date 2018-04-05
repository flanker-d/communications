#pragma once

namespace common
{
  const int BUF_LENGTH = 32768;
  using buf_t = std::array<char, BUF_LENGTH>;
  using pbuf_t = std::unique_ptr<buf_t>;

  enum class read_func_type_e
  {
      custom_eol,
      custom_eol_std_find,
      read_until_eol,
      async_read
  };

  struct tcp_server_params_t
  {
    read_func_type_e do_read_type;
    bool use_strand;
#ifdef USE_TCP_SERVER_DEBUG_COUNTER
    int read_counter = -1;
#endif
  };

} //namespace common
