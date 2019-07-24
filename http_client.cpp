#include <algorithm>
#include <exception>
#include <new>
#include <sstream>
#include <utility>

#include <curl/curl.h>

#include "http_client.hpp"

namespace hqrp {

  static std::size_t write_callback(char* ptr,
                                    [[maybe_unused]] size_t size,
                                    size_t nmemb,
                                    void* userdata) {
    const auto vec = reinterpret_cast<bytes*>(userdata);
    const auto vec_original_size = vec->size();

    vec->resize(vec->size() + nmemb);

    const auto byte_arr = vec->data();
    std::copy(ptr, ptr + nmemb, byte_arr + vec_original_size);
    return nmemb;
  }

  static std::string& trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char ch) {
              return !std::isspace(ch);
            }));
    s.erase(std::find_if(
              s.rbegin(), s.rend(), [](char ch) { return !std::isspace(ch); })
              .base(),
            s.end());
    return s;
  }

  static std::size_t header_callback(char* buffer,
                                     [[maybe_unused]] size_t size,
                                     size_t nitems,
                                     void* userdata) {
    auto response = reinterpret_cast<HttpClient::Response*>(userdata);
    const auto header = std::string(buffer, nitems);
    auto header_stream = std::stringstream(header);
    if (const auto pos = header.find_first_of(" "); pos != std::string::npos) {
      if (pos > 0 && header[pos - 1] == ':') {
        auto header_name = std::string{};
        std::getline(header_stream, header_name, ':');
        auto header_value = std::string{};
        std::getline(header_stream, header_value, '\r');
        response->headers.push_back({ header_name, trim(header_value) });
      } else {
        std::getline(header_stream, response->status, '\r');
      }
    }
    return nitems;
  }

  HttpClient::HttpClient(const HttpClient& rhs)
      : m_curl_handle(curl_easy_duphandle(rhs.m_curl_handle)) {
  }
  HttpClient& HttpClient::operator=(const HttpClient& rhs) {
    if (this != &rhs) {
      curl_easy_cleanup(m_curl_handle);
      m_curl_handle = curl_easy_duphandle(rhs.m_curl_handle);
    }
    return *this;
  }
  HttpClient::HttpClient(HttpClient&& rhs) : m_curl_handle(rhs.m_curl_handle) {
    rhs.m_curl_handle = nullptr;
  }
  HttpClient& HttpClient::operator=(HttpClient&& rhs) {
    if (this != &rhs) {
      curl_easy_cleanup(m_curl_handle);
      m_curl_handle = rhs.m_curl_handle;
      rhs.m_curl_handle = nullptr;
    }
    return *this;
  }
  HttpClient::~HttpClient() {
    curl_easy_cleanup(m_curl_handle);
  }

  bool HttpClient::init() {
    return (m_curl_handle = curl_easy_init());
  }

  HttpClient& HttpClient::set_request(const std::string& url,
                                      const Request& request,
                                      unsigned int port) {
    if (!m_curl_handle) {
      throw std::runtime_error(
        "Setting request for an uninitialized client - consider "
        "calling HttpClient::init() first.");
    }

    client_reset();
    set_method(request.http_method);
    set_target_address(url + request.resource_path, port);
    if (request.headers) {
      set_headers(*request.headers);
    }
    if (request.payload) {
      set_payload(*request.payload);
    } else {
      curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDSIZE, 0);
    }
    set_response_function();
    return *this;
  }

  HttpClient& HttpClient::verbose() {
    curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 1);
    return *this;
  }

  HttpClient& HttpClient::with_capath(const std::string& path) {
    curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYHOST, 1);
    curl_easy_setopt(m_curl_handle, CURLOPT_SSL_VERIFYPEER, 1);
    curl_easy_setopt(m_curl_handle, CURLOPT_CAPATH, path.c_str());
    return *this;
  }

  HttpClient& HttpClient::with_certificate(const std::string& path) {
    curl_easy_setopt(m_curl_handle, CURLOPT_CAINFO, path.c_str());
    return *this;
  }

  std::optional<HttpClient::Response> HttpClient::execute() {
    if (!m_curl_handle) {
      throw std::runtime_error(
        "Execution of an uninitialized client - consider calling "
        "HttpClient::init() first.");
    }
    auto response = Response{};
    change_buffer_to_write(std::addressof(response));
    return curl_easy_perform(m_curl_handle) ? std::nullopt
                                            : std::optional{ response };
  }

  void HttpClient::client_reset() {
    curl_easy_reset(m_curl_handle);
  }

  void HttpClient::set_method(Method method) {
    curl_easy_setopt(m_curl_handle, static_cast<CURLoption>(method), 1);
  }

  void HttpClient::set_target_address(const std::string& uri,
                                      unsigned int port) {
    curl_easy_setopt(m_curl_handle, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_PORT, port);
  }

  void HttpClient::set_headers(const headers& headers) {
    auto header_list = static_cast<curl_slist*>(nullptr);
    for (const auto& [property, value] : headers) {
      header_list =
        curl_slist_append(header_list, (property + ": " + value).c_str());
    }
    curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, header_list);
  }

  void HttpClient::set_payload(const bytes& payload) {
    curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDSIZE, payload.size());
    curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDS, payload.data());
  }

  void HttpClient::set_response_function() {
    curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, &header_callback);
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, &write_callback);
  }

  void HttpClient::change_buffer_to_write(Response* response) {
    curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, response);
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &response->data);
  }

}  // namespace hqrp
