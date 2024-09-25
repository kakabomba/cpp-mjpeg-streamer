#pragma once

#include <sstream>
#include <string>
#include <unordered_map>

// Reference https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages#http_requests

namespace nadjieb {
namespace net {
class HTTPRequest {
   public:
    HTTPRequest(const std::string& message) { parse(message); }

    void parse(const std::string& message) {
        std::istringstream iss(message);

        std::getline(iss, method_, ' ');

        std::string request_;
        std::getline(iss, request_, ' ');
        std::istringstream request(request_);
        printf("Request full request_: %s\n", request_.c_str());

        std::getline(request, target_, '?');
        printf("Request target_: %s\n", target_.c_str());
        std::getline(request, parameter_key_, '=');
        printf("Request parameter_key_: %s\n", parameter_key_.c_str());
		std::getline(request, parameter_value_, '&');
        printf("Request parameter_value_: %s\n", parameter_value_.c_str());

        std::getline(iss, version_, '\r');

        std::string line;
        std::getline(iss, line);

        while (true) {
            std::getline(iss, line);
            if (line == "\r") {
                break;
            }

            std::string key;
            std::string value;
            std::istringstream iss_header(line);
            std::getline(iss_header, key, ':');
            std::getline(iss_header, value, ' ');
            std::getline(iss_header, value, '\r');

            headers_[key] = value;
        }

        body_ = iss.str().substr(iss.tellg());
    }

    const std::string& getMethod() const { return method_; }

    const std::string& getTarget() const { return target_; }

    const std::string& getVersion() const { return version_; }

    const std::string& getHeaderValue(const std::string& key) { return headers_[key]; }

    void getRequestParameter(std::string &key, int &value) {
		key = parameter_key_;
		value = std::stoi(parameter_value_);
 		}

    const std::string& getBody() const { return body_; }

   private:
    std::string method_;
    std::string target_;
    std::string parameter_key_;
	std::string parameter_value_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};
}  // namespace net
}  // namespace nadjieb
