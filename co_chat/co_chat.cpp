//
// co_chat.cpp
// ~~~~~~~~~~~
//
// Build (Linux Mint 19):
//
// g++-11 -I/usr/local/boost_1_78_0/include -fcoroutines -std=c++20
// -Wall -Werror -o co_chat co_chat.cpp -l pthread
//
// Build (Mac OS Big Sur 11.6.5):
//
// g++-11 -I /usr/local/Cellar/boost/1.76.0/include
// -L /usr/local/Cellar/boost/1.76.0/lib -fcoroutines -std=c++20 -Wall
// -Werror -o co_chat co_chat.cpp -l pthread -l boost_regex
//
// terminal 1> ./co_chat 127.0.0.1 6666 127.0.0.1 7777
// terminal 2> ./co_chat 127.0.0.1 7777 127.0.0.1 6666

#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/regex.hpp>

#include <iostream>

using boost::asio::awaitable;
using boost::asio::buffer;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::dynamic_buffer;
using boost::asio::io_context;
using boost::asio::steady_timer;
using boost::asio::use_awaitable;
using boost::asio::experimental::as_tuple;
using boost::asio::ip::basic_resolver_entry;
using boost::asio::ip::tcp;
using boost::asio::ip::tcp::resolver::passive;

using boost::system::error_code;

class chat {

public:
  chat(char *addr, char *port, char *remote_addr,
       char *remote_port) {

    tcp::resolver resolver(ctx);
    basic_resolver_entry<tcp> listen_endpoint;
    basic_resolver_entry<tcp> remote_endpoint;

    listen_endpoint = *resolver.resolve(addr, port, passive);
    remote_endpoint = *resolver.resolve(remote_addr, remote_port);

    co_spawn(ctx, sender(remote_endpoint), detached);
    co_spawn(ctx, receiver(listen_endpoint), detached);

    ctx.run();
  }

private:
  awaitable<void> sender(tcp::endpoint remote) {

    for (;;) {
      tcp::socket remote_sock(ctx);

      auto [error] = co_await remote_sock.async_connect(
          remote, as_tuple(use_awaitable));
      if (!error) {
        std::cout << "Connected to: " << remote << std::endl;
        connected = true;
      } else {
        std::cout << "Could not connect to: " << remote
                  << " - retrying in 1s" << std::endl;

        steady_timer timer(
            co_await boost::asio::this_coro::executor);
        timer.expires_after(std::chrono::seconds(1));
        co_await timer.async_wait(use_awaitable);
        continue;
      }

      std::string data;
      while (connected) {
        // Read a string from stdin (non-blocking)
        struct pollfd input[1] = {{.fd = 0, .events = POLLIN}};
        if (poll(input, 1, 100 /* timeout in ms */)) {
          char c;
          while (std::cin.get(c) && c != '\n')
            data += c;

          data += "\r\n";
        }

        co_await async_write(remote_sock, buffer(data),
                             as_tuple(use_awaitable));

        data.clear();
      }
    }
  }

  awaitable<void> receiver(tcp::endpoint listen) {

    tcp::acceptor acceptor(ctx, listen);

    for (;;) {
      auto [error, client] =
          co_await acceptor.async_accept(as_tuple(use_awaitable));

      if (!error) {
        std::string data;

        for (;;) {
          auto [error, len] = co_await async_read_until(
              client, dynamic_buffer(data), boost::regex("\r\n"),
              as_tuple(use_awaitable));

          if (error == boost::asio::error::eof) {
            // remote has disconnected
            connected = false;
            break;
          }

          std::cout << client.remote_endpoint() << "> " << data;
          data.clear();
        }
      } else {
        std::cerr << "Accept failed: " << error.message() << "\n";
      }
    }
  }

  // Member variables (shared between coroutines)
  io_context ctx;
  bool connected = false;
};

int main(int argc, char *argv[]) {

  try {
    if (argc != 5) {
      std::cerr << "Usage: " << argv[0];
      std::cerr << " <listen_address> <listen_port>";
      std::cerr << " <remote_address> <remote_port>\n";
      return 1;
    }

    chat(argv[1], argv[2], argv[3], argv[4]);

  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}
