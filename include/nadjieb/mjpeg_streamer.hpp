/*
C++ MJPEG over HTTP Library
https://github.com/nadjieb/cpp-mjpeg-streamer

MIT License

Copyright (c) 2020-2023 Muhammad Kamal Nadjieb

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

#include <nadjieb/net/http_request.hpp>
#include <nadjieb/net/http_response.hpp>
#include <nadjieb/net/listener.hpp>
#include <nadjieb/net/publisher.hpp>
#include <nadjieb/net/socket.hpp>
#include <nadjieb/utils/non_copyable.hpp>

#include <string>
#include <unordered_map>
#include <ctime>
#include "sys/time.h"

namespace nadjieb {

double start_time1 = 0;
double seconds1() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return 1.*tp.tv_sec + 1.*tp.tv_usec / 1000000. - start_time1;
}

class MJPEGStreamer : public nadjieb::utils::NonCopyable {
   public:
    virtual ~MJPEGStreamer() { stop(); }

    void start(int port, int num_workers = std::thread::hardware_concurrency()) {
		struct timeval tp;
		gettimeofday(&tp, NULL);
		start_time1 = 1.*tp.tv_sec + 1.*tp.tv_usec / 1000000.;
		parameters_["requested_quality"] = 80;
		parameters_["requested_exposition"] = 100000;
        parameters_["requested_exposition_auto"] = 0;
		parameters_["requested_contrast"] = 50;
		parameters_["requested_brightness"] = 80;
		parameters_["requested_enhance"] = 1;
        parameters_["last_exposition"] = -1;
        parameters_["last_quality"] = -1;
        parameters_["last_brightness"] = -1;
        parameters_["last_brightness_red"] = -1;
        parameters_["last_brightness_green"] = -1;
        parameters_["last_brightness_blue"] = -1;
        parameters_["last_contrast"] = -1;
        parameters_["last_enhance"] = -1;

        last_heartbeat = seconds1();
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

    int getRequestedParameter(const std::string& key) {
        return parameters_[key];
        }

    double getLastHeartBeatAgo() {
        return seconds1() - last_heartbeat;
    }

    void getRequestedRoi(int& l, int& t, int& r, int& b) {

    }

    void setInfo(const std::string& key, const double val) {
        parameters_[key] = val;
    }

    void setParameterInHeader(const std::string& key, nadjieb::net::HTTPResponse& resp) {
        char buf[128]={0};
        char buf1[128]={0};
        sprintf(buf, "X-Parameter-%s", key.c_str());
        sprintf(buf1, "%d", parameters_[key]);
        resp.setValue(buf, buf1);
    }


    bool hasClient(const std::string& path) { return publisher_.hasClient(path); }

   private:
    nadjieb::net::Listener listener_;
    nadjieb::net::Publisher publisher_;
    std::string shutdown_target_ = "/shutdown";
    std::string parameter_ = "/parameter";
    std::string heartbeat_ = "/heartbeat";
    std::string roi_ = "/roi";
    std::unordered_map<std::string, int> parameters_;

    double last_heartbeat = 0.;


    nadjieb::net::OnMessageCallback on_message_cb_ = [&](const nadjieb::net::SocketFD& sockfd,
                                                         const std::string& message) {
        nadjieb::net::HTTPRequest req(message);
        std::string target = req.getTarget();
        nadjieb::net::OnMessageCallbackResponse cb_res;

        nadjieb::net::HTTPResponse ok_res;
        nadjieb::net::HTTPResponse param_res;

        ok_res.setVersion(req.getVersion());
        ok_res.setStatusCode(200);
        ok_res.setStatusText("OK");
        printf("OnMessageCallback. target=%s\n", target.c_str());
        auto ok_res_str = ok_res.serialize();

        if (target == shutdown_target_) {
            nadjieb::net::sendViaSocket(sockfd, ok_res_str.c_str(), ok_res_str.size(), 0);
            publisher_.stop();
            cb_res.end_listener = true;
            return cb_res;
        }

        if (target == heartbeat_) {
            last_heartbeat = seconds1();
            nadjieb::net::sendViaSocket(sockfd, ok_res_str.c_str(), ok_res_str.size(), 0);
            cb_res.close_conn = true;
            return cb_res;
        }

        if (target == roi_) {
            nadjieb::net::sendViaSocket(sockfd, ok_res_str.c_str(), ok_res_str.size(), 0);
            cb_res.close_conn = true;
            return cb_res;
        }

        if (target == parameter_) {
            std::string parameter_name;
            int parameter_value;

            try
            {
                req.getRequestParameter(parameter_name, parameter_value);
            }
            catch (...)
            {

            }

            if (parameter_name == "requested_quality" or parameter_name == "requested_exposition" or parameter_name == "requested_brightness" or parameter_name == "requested_contrast" or parameter_name == "requested_enhance" or parameter_name == "requested_exposition_auto") {
                parameters_[parameter_name] = parameter_value;
                }

            param_res.setVersion(req.getVersion());
            param_res.setStatusCode(200);
            param_res.setStatusText("OK");
            param_res.setValue("Connection", "close");
            param_res.setValue("Cache-Control", "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0");
            param_res.setValue("Pragma", "no-cache");
            param_res.setValue("Content-Type", "text/html; charset=utf-8");

            setParameterInHeader("requested_exposition_auto", param_res);
            setParameterInHeader("requested_exposition", param_res);
            setParameterInHeader("requested_quality", param_res);
            setParameterInHeader("requested_brightness", param_res);
            setParameterInHeader("requested_contrast", param_res);
            setParameterInHeader("requested_enhance", param_res);

            setParameterInHeader("last_exposition", param_res);
            setParameterInHeader("last_quality", param_res);
            setParameterInHeader("last_brightness", param_res);
            setParameterInHeader("last_brightness_red", param_res);
            setParameterInHeader("last_brightness_green", param_res);
            setParameterInHeader("last_brightness_blue", param_res);
            setParameterInHeader("last_contrast", param_res);
            setParameterInHeader("last_enhance", param_res);

            auto param_res_str = param_res.serialize();

            nadjieb::net::sendViaSocket(sockfd, param_res_str.c_str(), param_res_str.size(), 0);

//            nadjieb::net::sendViaSocket(sockfd, ok_res_str.c_str(), ok_res_str.size(), 0);
            cb_res.close_conn = true;
            return cb_res;
        }


        if (req.getMethod() != "GET") {
            nadjieb::net::HTTPResponse method_not_allowed_res;
            method_not_allowed_res.setVersion(req.getVersion());
            method_not_allowed_res.setStatusCode(405);
            method_not_allowed_res.setStatusText("Method Not Allowed");
            auto method_not_allowed_res_str = method_not_allowed_res.serialize();

            nadjieb::net::sendViaSocket(
                sockfd, method_not_allowed_res_str.c_str(), method_not_allowed_res_str.size(), 0);

            cb_res.close_conn = true;
            return cb_res;
        }

        if (!publisher_.pathExists(target)) {
            nadjieb::net::HTTPResponse not_found_res;
            not_found_res.setVersion(req.getVersion());
            not_found_res.setStatusCode(404);
            not_found_res.setStatusText("Not Found");
            auto not_found_res_str = not_found_res.serialize();

            nadjieb::net::sendViaSocket(sockfd, not_found_res_str.c_str(), not_found_res_str.size(), 0);

            cb_res.close_conn = true;
            return cb_res;
        }

        nadjieb::net::HTTPResponse init_res;
        init_res.setVersion(req.getVersion());
        init_res.setStatusCode(200);
        init_res.setStatusText("OK");
        init_res.setValue("Connection", "close");
        init_res.setValue("Cache-Control", "no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0");
        init_res.setValue("Pragma", "no-cache");
        init_res.setValue("Content-Type", "multipart/x-mixed-replace; boundary=nadjiebmjpegstreamer");
        auto init_res_str = init_res.serialize();

        nadjieb::net::sendViaSocket(sockfd, init_res_str.c_str(), init_res_str.size(), 0);

        publisher_.add(sockfd, target);

        return cb_res;
    };

    nadjieb::net::OnBeforeCloseCallback on_before_close_cb_
        = [&](const nadjieb::net::SocketFD& sockfd) { publisher_.removeClient(sockfd); };
};
}  // namespace nadjieb
