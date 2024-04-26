#include <boost/asio.hpp>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>

using boost::asio::ip::tcp;

int main() {
    boost::asio::io_context io_context;

    std::vector<std::thread> ts;
    for (int i = 0; i < 100; i++) {
        ts.emplace_back([&io_context]() {
            tcp::socket s(io_context);
            boost::asio::connect(s, tcp::resolver(io_context).resolve("localhost", "12345"));
            tcp::iostream conn(std::move(s));
            std::cout << "Connected " << conn.socket().local_endpoint() << " --> "
                      << conn.socket().remote_endpoint() << "\n";

            std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution cmd_choice(0, 2);
            std::uniform_int_distribution letter_choice('a', 'b');
            for (int i = 0; i < 5; i++) {
                static std::string commands[]{"add", "del", "count"};
                conn << commands[cmd_choice(gen)] << " ";
                for (int j = 0; j < 10; j++) {
                    conn << letter_choice(gen);
                }
                conn << "\n";

                std::string response;
                std::getline(conn, response);
                if (i % 100 == 0) {
                    std::cout << response << "\n";
                }
            }
        });
    }
    for (auto &t : ts) {
        t.join();
    }
    return 0;
}
