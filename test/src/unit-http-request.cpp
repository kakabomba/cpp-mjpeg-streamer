#include <doctest/doctest.h>

#include <nadjieb/net/http_request.hpp>

TEST_SUITE("HTTPRequest") {
    TEST_CASE("HTTP Request Message") {
        GIVEN("A raw request message") {
            const char* raw
                = "POST /cgi-bin/process.cgi HTTP/1.1\r\n"
                  "User-Agent: Mozilla/4.0\r\n"
                  "Server: Apache/2.2.14\r\n"
                  "Host: www.tutorialspoint.com\r\n"
                  "Content-Type: application/x-www-form-urlencoded\r\n"
                  "Content-Length: 50\r\n"
                  "Accept-Language: en-us\r\n"
                  "Accept-Encoding: gzip, deflate\r\n"
                  "Connection: Keep-Alive\r\n"
                  "\r\n"
                  "licenseID=string&content=string&/paramsXML=string\n\n\n";
            WHEN("HTTPRequest parse a raw request message") {
                nadjieb::net::HTTPRequest req(raw);

                THEN("The HTTPRequest equal to the raw message") {
                    CHECK(req.getMethod() == "POST");
                    CHECK(req.getTarget() == "/cgi-bin/process.cgi");
                    CHECK(req.getVersion() == "HTTP/1.1");
                    CHECK(req.getHeaderValue("User-Agent") == "Mozilla/4.0");
                    CHECK(req.getHeaderValue("Server") == "Apache/2.2.14");
                    CHECK(req.getHeaderValue("Host") == "www.tutorialspoint.com");
                    CHECK(req.getHeaderValue("Content-Type") == "application/x-www-form-urlencoded");
                    CHECK(req.getHeaderValue("Content-Length") == "50");
                    CHECK(req.getHeaderValue("Accept-Language") == "en-us");
                    CHECK(req.getHeaderValue("Accept-Encoding") == "gzip, deflate");
                    CHECK(req.getHeaderValue("Connection") == "Keep-Alive");
                    CHECK(req.getHeaderValue("abc") == "");
                    CHECK(req.getBody() == "licenseID=string&content=string&/paramsXML=string\n\n\n");
                }
            }
        }
    }
}
