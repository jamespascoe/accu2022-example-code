//
// echo_server_blocking.cpp
// ------------------------
//
// g++-11 -I/usr/local/boost_1_78_0/include -Wall -Werror
// echo_server_blocking.cpp -o blocking -l pthread
//

#include <boost/asio.hpp>
#include <iostream>

using boost::asio::buffer;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::system::error_code;

static const int buf_len = 1000;

int main() {
  try {
    io_context ctx;

    tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 6666));

    for (;;) {
      tcp::socket peer_socket(ctx);
      acceptor.accept(peer_socket);

      std::array<char, buf_len> buf;

      for (;;) {
        error_code error;
        std::size_t len =
            peer_socket.read_some(buffer(buf), error);
        if (error == boost::asio::error::eof)
          break;

        write(peer_socket, buffer(buf, len));
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}
