/*
C++ MJPEG over HTTP Library
https://github.com/nadjieb/cpp-mjpeg-streamer

MIT License

Copyright (c) 2020-2022 Muhammad Kamal Nadjieb

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <nadjieb/utils/version.hpp>

#include <nadjieb/net/http_message.hpp>
#include <nadjieb/net/listener.hpp>
#include <nadjieb/net/publisher.hpp>
#include <nadjieb/net/socket.hpp>
#include <nadjieb/utils/non_copyable.hpp>

#include <string>

namespace nadjieb {
class MJPEGStreamer : public nadjieb::utils::NonCopyable {
   public:
    virtual ~MJPEGStreamer() { stop(); }

    void start(int port, int num_workers = 1) {
        publisher_.start(num_workers);
        listener_.withOnMessageCallback(on_message_cb_).withOnBeforeCloseCallback(on_before_close_cb_).runAsync(port);

        while (!isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void stop() {
        publisher_.stop();
        listener_.stop();
    }

    void publish(const std::string& path, const std::string& buffer) { publisher_.enqueue(path, buffer); }

    void setShutdownTarget(const std::string& target) { shutdown_target_ = target; }

    bool isRunning() { return (publisher_.isRunning() && listener_.isRunning()); }

    bool hasClient(const std::string& path) { return publisher_.hasClient(path); }

   private:
    nadjieb::net::Listener listener_;
    nadjieb::net::Publisher publisher_;
    std::string shutdown_target_ = "/shutdown";

    nadjieb::net::OnMessageCallback on_message_cb_ = [&](const nadjieb::net::SocketFD& sockfd,
                                                         const std::string& message) {
        nadjieb::net::HTTPMessage req(message);
        nadjieb::net::OnMessageCallbackResponse res;

        if (req.target() == shutdown_target_) {
            nadjieb::net::HTTPMessage shutdown_res;
            shutdown_res.start_line = "HTTP/1.1 200 OK";
            auto shutdown_res_str = shutdown_res.serialize();

            nadjieb::net::sendViaSocket(sockfd, shutdown_res_str.c_str(), shutdown_res_str.size(), 0);

            publisher_.stop();

            res.end_listener = true;
            return res;
        }

        if (req.method() != "GET") {
            nadjieb::net::HTTPMessage method_not_allowed_res;
            method_not_allowed_res.start_line = "HTTP/1.1 405 Method Not Allowed";
            auto method_not_allowed_res_str = method_not_allowed_res.serialize();

            nadjieb::net::sendViaSocket(
                sockfd, method_not_allowed_res_str.c_str(), method_not_allowed_res_str.size(), 0);

            res.close_conn = true;
            return res;
        }

        nadjieb::net::HTTPMessage init_res;
        init_res.start_line = "HTTP/1.1 200 OK";
        init_res.headers["Connection"] = "close";
        init_res.headers["Cache-Control"] = "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0";
        init_res.headers["Pragma"] = "no-cache";
        init_res.headers["Content-Type"] = "multipart/x-mixed-replace; boundary=boundarydonotcross";
        auto init_res_str = init_res.serialize();

        nadjieb::net::sendViaSocket(sockfd, init_res_str.c_str(), init_res_str.size(), 0);

        publisher_.add(req.target(), sockfd);

        return res;
    };

    nadjieb::net::OnBeforeCloseCallback on_before_close_cb_
        = [&](const nadjieb::net::SocketFD& sockfd) { publisher_.removeClient(sockfd); };
};
}  // namespace nadjieb
