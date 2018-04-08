#include "../../communications.h"
#include <functional>
#include <iostream>

namespace common
{
  namespace tcp
  {
    class client
     : public iclient
    {
      public:
        client(tcp_client_params_t& a_params, boost::asio::io_service& a_io_service);
        void run() override;
        void send_message(const std::string& a_data) override;
        void set_on_connected(std::function<void()> a_on_connected) override;
        void set_on_disconnected(std::function<void()> a_on_disconnected) override;
        void set_on_message(std::function<void(const std::string&)> a_on_message) override;

      private:
        void do_connect() override;
        void init_read_function();
        void do_receive_completion_eol();
        void do_receive_read_until_eol();
        void do_receive_async_read_some_eol();

      private:
        boost::asio::io_service& m_io_service;
        boost::asio::io_service::strand m_strand;
        boost::asio::ip::tcp::endpoint m_ep;
        boost::asio::ip::tcp::socket m_sock;
        bool m_is_connected;
        pbuf_t m_buffer;
        boost::asio::streambuf m_streambuf;
        std::string m_buffer_str;
        tcp_client_params_t m_params;

        std::function<void()> m_do_receive_func;
        std::function<void()> m_on_connected_func;
        std::function<void()> m_on_disconnected_func;
        std::function<void(const std::string&)> m_on_message_func;
    };

    client::client(tcp_client_params_t& a_params, boost::asio::io_service& a_io_service)
     : m_io_service(a_io_service)
     , m_strand(a_io_service)
     , m_ep(boost::asio::ip::address::from_string(a_params.ip), a_params.port)
     , m_sock(a_io_service)
     , m_is_connected(false)
     , m_buffer(std::make_unique<buf_t>())
     , m_params(a_params)
    {
      init_read_function();
    }

    void client::run()
    {
      do_connect();
    }

    void client::send_message(const std::string& a_data)
    {
      if(m_is_connected)
      {
        auto async_write_handler = [](const boost::system::error_code& /*err*/, size_t /*bytes*/)
        {
        };
      
        m_sock.async_write_some(boost::asio::buffer(a_data.c_str(), a_data.length()), m_strand.wrap(async_write_handler));
      }
    }

    void client::set_on_connected(std::function<void()> a_on_connected)
    {
      m_on_connected_func = a_on_connected;
    }

    void client::set_on_disconnected(std::function<void()> a_on_disconnected)
    {
      m_on_disconnected_func = a_on_disconnected;
    }

    void client::set_on_message(std::function<void(const std::string&)> a_on_message)
    {
      m_on_message_func = a_on_message;
    }

    void client::do_connect()
    {
      m_sock.async_connect(m_ep, m_strand.wrap([this](const boost::system::error_code& a_ec)
      {
        if(!a_ec)
        {
          m_is_connected = true;
          m_sock.set_option(boost::asio::ip::tcp::socket::reuse_address(true));

          if(m_do_receive_func != nullptr)
            m_do_receive_func();

          if(m_on_connected_func != nullptr)
            m_on_connected_func();
        }
        else
        {
          m_is_connected = false;

          if(m_on_disconnected_func != nullptr)
            m_on_disconnected_func();
        }
      }));
    }

    void client::init_read_function()
    {
      switch (m_params.do_read_type)
      {
        case read_func_type_e::completion_eol:
          m_do_receive_func = std::bind(&client::do_receive_completion_eol, this);
          break;
        case read_func_type_e::read_until_eol:
          m_do_receive_func = std::bind(&client::do_receive_read_until_eol, this);
          break;
        case read_func_type_e::async_read_some_eol:
          m_do_receive_func = std::bind(&client::do_receive_async_read_some_eol, this);
          break;
        default:
          m_do_receive_func = std::bind(&client::do_receive_async_read_some_eol, this);
          break;
      }
    }

    void client::do_receive_completion_eol()
    {
      m_buffer->fill(0);

      auto async_read_completion_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)->std::size_t
      {
        if(a_ec)
          return 0;
        if(a_len > 0)
        {
          bool cond = (m_buffer->data()[a_len - 1] == '\n');
          return cond ? 0 : 1;
        }
        return 1;
      };

      auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
      {
        if(a_len == 0)
        {
          if(m_on_disconnected_func != nullptr)
            m_on_disconnected_func();
        }

        if (!a_ec)
        {
          if(m_on_message_func != nullptr)
            m_on_message_func({m_buffer->data(), a_len});

          do_receive_completion_eol();
        }
        else
        {
          m_is_connected = false;
          do_connect();
        }
      };

      if(m_params.use_strand)
        boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_completion_handler, m_strand.wrap(async_read_handler));
      else
        boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_completion_handler, async_read_handler);
    }

    void client::do_receive_read_until_eol()
    {
      auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
      {
        if(a_len == 0)
        {
          if(m_on_disconnected_func != nullptr)
            m_on_disconnected_func();
        }

        if (!a_ec)
        {
          std::istream is(&m_streambuf);
          std::string cmd;
          std::getline(is, cmd);

          if(m_on_message_func != nullptr)
            m_on_message_func(cmd);

          do_receive_read_until_eol();
        }
        else
        {
          m_is_connected = false;
          do_connect();
        }
      };
      if(m_params.use_strand)
        boost::asio::async_read_until(m_sock, m_streambuf, '\n', m_strand.wrap(async_read_handler));
      else
        boost::asio::async_read_until(m_sock, m_streambuf, '\n', async_read_handler);
    }

    void client::do_receive_async_read_some_eol()
    {
      m_buffer->fill(0);
      auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
      {
        if(a_len == 0)
        {
          if(m_on_disconnected_func != nullptr)
            m_on_disconnected_func();
        }

        if (!a_ec)
        {
          m_buffer_str += {m_buffer->data(), a_len};

          while(true)
          {
            auto term_pos = m_buffer_str.find('\n');
            if (term_pos != std::string::npos)
            {
              std::string cmd(m_buffer_str.begin(), m_buffer_str.begin() + term_pos);
              if (m_on_message_func != nullptr)
                m_on_message_func({m_buffer->data(), a_len});
              m_buffer_str.erase(m_buffer_str.begin(), m_buffer_str.begin() + term_pos + 1);
            }
            else
              break;
          }
          do_receive_async_read_some_eol();
        }
        else
        {
          m_is_connected = false;
          do_connect();
        }
      };

      if(m_params.use_strand)
        m_sock.async_read_some(boost::asio::buffer(m_buffer->data(), BUF_LENGTH), m_strand.wrap(async_read_handler));
      else
        m_sock.async_read_some(boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_handler);
    }
  } //namespace tcp
} //namespace common

namespace common
{
  namespace tcp
  {
    iclient::ref create_client(tcp_client_params_t& a_params, boost::asio::io_service& a_io_service)
    {
      return std::make_shared<client>(a_params, a_io_service);
    }   
  } //namespace tcp
} //namespace common
