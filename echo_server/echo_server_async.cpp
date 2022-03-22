//
// echo_server_async.cpp
// ---------------------
//
// g++-11 -I/usr/local/boost_1_78_0/include -fcoroutines -std=c++20
// -Wall -Werror echo_server_async.cpp -o async -l pthread
//

#include <boost/asio.hpp>

#include <iostream>

using boost::asio::buffer;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::system::error_code;

static const int buf_len = 1000;

void accept_handler(error_code const &error,
                    tcp::socket &peer_socket,
                    std::array<char, buf_len> &buf,
                    tcp::acceptor &acceptor);

void read_handler(error_code const &error,
                  std::size_t bytes_transferred,
                  tcp::socket &peer_socket,
                  std::array<char, buf_len> &buf,
                  tcp::acceptor &acceptor);

void write_handler(error_code const &error, std::size_t length,
                   tcp::socket &peer_socket,
                   std::array<char, buf_len> &buf,
                   tcp::acceptor &acceptor) {
  if (!error) {
    peer_socket.async_read_some(
        buffer(buf, length), [&](error_code const &error,
                                 std::size_t bytes_transferred) {
          read_handler(error, bytes_transferred, peer_socket, buf,
                       acceptor);
        });
  } else
    std::cerr << "Write handler error: " << error.message()
              << std::endl;
}

void read_handler(error_code const &error,
                  std::size_t bytes_transferred,
                  tcp::socket &peer_socket,
                  std::array<char, buf_len> &buf,
                  tcp::acceptor &acceptor) {
  if (!error) {
    async_write(peer_socket, buffer(buf, bytes_transferred),
                [&](error_code const &error, std::size_t length) {
                  write_handler(error, length, peer_socket, buf,
                                acceptor);
                });
  } else {
    if (error == boost::asio::error::eof) {
      peer_socket.close();

      acceptor.async_accept(
          peer_socket, [&](error_code const &error) {
            accept_handler(error, peer_socket, buf, acceptor);
          });
    } else
      std::cerr << "Read handler error: " << error.message()
                << std::endl;
  }
}

void accept_handler(error_code const &error,
                    tcp::socket &peer_socket,
                    std::array<char, buf_len> &buf,
                    tcp::acceptor &acceptor) {

  if (!error) {
    peer_socket.async_read_some(
        buffer(buf, buf_len), [&](error_code const &error,
                                  std::size_t bytes_transferred) {
          read_handler(error, bytes_transferred, peer_socket, buf,
                       acceptor);
        });
  } else
    std::cerr << "Accept handler error: " << error.message()
              << std::endl;
}

int main() {
  io_context ctx;
  tcp::socket peer_socket(ctx);
  tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 6666));

  std::array<char, buf_len> buf;

  acceptor.async_accept(peer_socket, [&](error_code const &error) {
    accept_handler(error, peer_socket, buf, acceptor);
  });

  try {
    ctx.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}
