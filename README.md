# cpp-http-client
Curl-based HTTP client for simple requests.

Example usage:
```cpp
#include <iostream>
#include <string>

#include "http_client.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  hqrp::HttpClient client{};
  if (!client.init()) {
    std::cerr << "HTTP Client unsuccessful init!" << std::endl;
  }
  const auto response =
    client
      .set_request(
        "https://www.gazeta.pl/0,0.html",
        { hqrp::HttpClient::Method::GET,
          "/WU/",
          hqrp::headers{ { "User-Agent", "HQRP" }, { "Date", "2018-10-18" } } },
        443)
      .verbose()
      .execute();
  for (auto x : response->data) {
    std::cout << static_cast<char>(x);
  }
}
```
