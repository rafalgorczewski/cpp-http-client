#ifndef HTTPCLIENT_HPP
#define HTTPCLIENT_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <curl/curl.h>

namespace hqrp {

  using bytes = std::vector<unsigned char>;
  using header = std::pair<std::string, std::string>;
  using headers = std::vector<header>;

  class HttpClient {
   public:
    enum class Method {
      GET = CURLOPT_HTTPGET,
      POST = CURLOPT_POST,
      PUT = CURLOPT_PUT
    };

    struct Request {
      Method http_method;
      std::string resource_path;
      std::optional<headers> headers = std::nullopt;
      std::optional<bytes> payload = std::nullopt;
    };

    struct Response {
      std::string status;
      headers headers;
      bytes data;
    };

   public:
    HttpClient() = default;
    HttpClient(const HttpClient& rhs);
    HttpClient& operator=(const HttpClient& rhs);
    HttpClient(HttpClient&& rhs);
    HttpClient& operator=(HttpClient&& rhs);
    ~HttpClient();

    bool init();
    HttpClient& set_request(const std::string& url,
                            const Request& request,
                            unsigned int port = 80);
    HttpClient& verbose();
    HttpClient& with_capath(const std::string& path);
    HttpClient& with_certificate(const std::string& path);
    std::optional<Response> execute();

   private:
    CURL* m_curl_handle{};

    void client_reset();
    void set_method(Method method);
    void set_target_address(const std::string& uri, unsigned int port);
    void set_headers(const headers& headers);
    void set_payload(const bytes& payload);
    void set_response_function();
    void change_buffer_to_write(Response* response);
  };

}  // namespace hqrp
#endif
