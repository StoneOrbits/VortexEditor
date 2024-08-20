#include <windows.h>
#include <winhttp.h>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>

class HttpClient
{
public:
  HttpClient(const std::string &userAgent);
  std::string SendRequest(const std::string &host, const std::string &path,
    const std::string &method = "GET",
    const std::map<std::string, std::string> &headers = {},
    const std::string &requestData = "",
    const std::map<std::string, std::string> &queryParams = {});

private:
  std::wstring userAgent;
  bool useHttps;

  std::wstring BuildFullPath(const std::wstring &path, const std::map<std::wstring, std::wstring> &queryParams);
  static std::wstring ConvertToWideString(const std::string &input);
  static std::string ConvertToNarrowString(const std::wstring &input);
  static std::map<std::wstring, std::wstring> ConvertMapToWideString(const std::map<std::string, std::string> &inputMap);
};
