#include <boost/asio.hpp>
#include <iostream>
#include <array>
#include <thread>
#include <mutex>

using boost::asio::ip::tcp;

const int max_length = 1024;
const int max_buffer_size = 10;

class CircularBuffer {
private:
    std::array<std::string, max_buffer_size> buffer;
    int head;
    int tail;
    int size;
    std::mutex mutex;

public:
    CircularBuffer() : head(0), tail(0), size(0) {}

    bool isEmpty() const {
        return size == 0;
    }

    bool isFull() const {
        return size == max_buffer_size;
    }

    void push(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);
        buffer[tail] = message;
        tail = (tail + 1) % max_buffer_size;
        if (isFull()) {
            head = (head + 1) % max_buffer_size;
        } else {
            size++;
        }
    }

    std::string pop() {
        std::lock_guard<std::mutex> lock(mutex);
        if (isEmpty()) {
            throw std::runtime_error("Buffer is empty");
        }
        std::string message = buffer[head];
        head = (head + 1) % max_buffer_size;
        size--;
        return message;
    }
};

void session(tcp::socket sock, CircularBuffer& message_buffer) {
    try {
        std::array<char, max_length> data;
        while (true) {
            boost::system::error_code error;
            size_t length = sock.read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof)
                break;
            else if (error)
                throw boost::system::system_error(error);

            std::string message(data.data(), length);
            message_buffer.push(message);

            boost::asio::write(sock, boost::asio::buffer("Received: " + message));

            std::cout << "Received from client: " << message << std::endl;
            std::cout << "Sent to client";
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        CircularBuffer message_buffer;

        while (true) {
            tcp::socket sock = acceptor.accept();
            std::thread(session, std::move(sock), std::ref(message_buffer)).detach();
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
