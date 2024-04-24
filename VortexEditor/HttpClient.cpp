#include "HttpClient.h"

using namespace std;

HttpClient::HttpClient(const string &userAgent)
  : userAgent(ConvertToWideString(userAgent))
{
}

string HttpClient::SendRequest(const string &host, const string &path, const string &method,
  const map<string, string> &headers, const string &requestData, const map<string, string> &queryParams)
{
  wstring responseW;
  string result;
  wstring hostW = ConvertToWideString(host);
  wstring pathW = BuildFullPath(ConvertToWideString(path), ConvertMapToWideString(queryParams));
  wstring methodW = ConvertToWideString(method);

  HINTERNET hSession = nullptr;
  HINTERNET hConnect = nullptr;
  HINTERNET hRequest = nullptr;

  hSession = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession) {
    throw runtime_error("Failed to open HTTP session");
  }

  hConnect = WinHttpConnect(hSession, hostW.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (!hConnect) {
    throw runtime_error("Failed to connect");
  }

  hRequest = WinHttpOpenRequest(hConnect, methodW.c_str(), pathW.c_str(),
    NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!hRequest) {
    throw runtime_error("Failed to open HTTP request");
  }

  for (const auto &pair : headers) {
    wstring fullHeader = ConvertToWideString(pair.first + ": " + pair.second);
    if (!WinHttpAddRequestHeaders(hRequest, fullHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD)) {
      throw runtime_error("Failed to add request header");
    }
  }

  vector<wchar_t> requestDataW(requestData.begin(), requestData.end());
  BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, requestDataW.data(), 
    requestDataW.size() * sizeof(wchar_t), requestDataW.size() * sizeof(wchar_t), 0);
  if (!bResults) {
    throw runtime_error("Failed to send request");
  }
  bResults = WinHttpReceiveResponse(hRequest, NULL);
  if (!bResults) {
    throw runtime_error("Failed to receive response");
  }

  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  do {
    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
      throw runtime_error("Failed to query data available");
    }

    vector<char> buffer(dwSize + 1);
    if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
      throw runtime_error("Failed to read data");
    }

    responseW.append(buffer.begin(), buffer.begin() + dwDownloaded);
  } while (dwSize > 0);

  // Convert wide string to UTF-8
  result = ConvertToNarrowString(responseW);

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  return result;
}

wstring HttpClient::BuildFullPath(const wstring &path, const map<wstring, wstring> &queryParams)
{
  if (queryParams.empty()) {
    return path;
  }

  wstringstream fullUrlStream;
  fullUrlStream << path;
  if (!queryParams.empty()) {
    fullUrlStream << L"?";
    for (auto iter = queryParams.begin(); iter != queryParams.end(); ++iter) {
      if (iter != queryParams.begin()) {
        fullUrlStream << L"&";
      }
      fullUrlStream << iter->first << L"=" << iter->second;
    }
  }
  return fullUrlStream.str();
}

wstring HttpClient::ConvertToWideString(const string &input)
{
  if (input.empty()) {
    return L"";
  }
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0);
  wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &input[0], (int)input.size(), &wstrTo[0], size_needed);
  return wstrTo;
}

string HttpClient::ConvertToNarrowString(const wstring &input)
{
  if (input.empty()) {
    return "";
  }
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0, NULL, NULL);
  string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
}

map<wstring, wstring> HttpClient::ConvertMapToWideString(const map<string, string> &inputMap)
{
  map<wstring, wstring> outputMap;
  for (const auto &kv : inputMap) {
    outputMap[ConvertToWideString(kv.first)] = ConvertToWideString(kv.second);
  }
  return outputMap;
}
