//
//  TXCHttpClient.h
//
//  Created by alderzhang on 2017/11/01.
//  Copyright © 2017年 tencent. All rights reserved.
//

#ifndef __TXCHTTPCLIENT_H__
#define __TXCHTTPCLIENT_H__

#include <Windows.h>
#include <map>
#include <winhttp.h>
#include "TXCTaskQueue.h"

/**************************************************************************/

typedef std::function<void(DWORD code, std::string& resp)> HttpResponseCallback;

enum class HttpAction {
	Request,
	Response
};

typedef std::function<void(HttpAction action, DWORD currentSize, DWORD totalSize, char *fragData, DWORD fragSize)> HttpPregressCallback;

class HttpHeaders : public std::map<std::wstring, std::wstring>
{
public:
	HttpHeaders();
	~HttpHeaders();

	bool has_header(const std::wstring &key) const;
	void set_header(const std::wstring &key, const std::wstring &value);
	std::wstring get_header(const std::wstring &key);
	int get_header_as_int(const std::wstring &key);

	void set_date();
	void set_authorization(std::wstring authorization);
	void set_content_type(std::wstring contentType = L"application/json");
	void set_content_length(uint64_t contentLength = 0);
	void set_connection(bool keepAlive = false);
};

typedef std::shared_ptr<HttpHeaders> HttpHeadersPtr;

class TXCHttpRequest
{
public:
	TXCHttpRequest(
		const std::wstring& url, 
		const std::wstring& method, 
		const std::wstring userAgent = L"", 
		int resolveTimeout = 0, 
		int connectTimeout = 60000,
		int sendTimeout = 30000,
		int recvTimeout = 30000);
	~TXCHttpRequest();

	DWORD error_code() const;
	DWORD status_code() const;
	bool send_headers(HttpHeadersPtr headers);
	bool send_request(const std::string &body = "", DWORD content_length = 0);
	DWORD send_data(const std::string &body, HttpPregressCallback progress_callback = nullptr);
	bool recv_response();
	bool recv_headers(HttpHeadersPtr headers);
	bool recv_data(std::string *resp, HttpPregressCallback progress_callback = nullptr);

	static std::wstring a2w(const std::string &str, unsigned int codePage = CP_ACP);

	static std::string w2a(const std::wstring &wstr, unsigned int codePage = CP_ACP);

private:
	TXCHttpRequest(const TXCHttpRequest &other) = default;
	TXCHttpRequest &operator=(const TXCHttpRequest &other) = default;

private:
	DWORD _errorCode;
	DWORD _statusCode;
	HINTERNET _session;
	HINTERNET _connect;
	HINTERNET _request;
	std::wstring _method;
};

typedef std::shared_ptr<TXCHttpRequest> TXCHttpRequestPtr;

class TXCHttpClient : public std::enable_shared_from_this<TXCHttpClient>
{
public:
    TXCHttpClient(const std::wstring& user_agent, TXCTaskQueue *queue = nullptr);
	~TXCHttpClient();

	void asyn_get(
		const std::wstring& url, 
		HttpHeadersPtr headers,
		HttpResponseCallback response_callback = nullptr,
		HttpPregressCallback progress_callback = nullptr);

	void asyn_post(
		const std::wstring& url, 
		HttpHeadersPtr headers,
		const std::string& body, 
		HttpResponseCallback response_callback = nullptr,
		HttpPregressCallback progress_callback = nullptr);

	void asyn_put(
		const std::wstring& url, 
		HttpHeadersPtr headers,
		const std::string& body, 
		HttpResponseCallback response_callback = nullptr,
		HttpPregressCallback progress_callback = nullptr);

    DWORD get(
		const std::wstring& url, 
		HttpHeadersPtr headers,
		std::string *resp, 
		HttpPregressCallback progress_callback = nullptr) const;

    DWORD post(
		const std::wstring& url, 
		HttpHeadersPtr headers,
		const std::string& body, 
		std::string *resp, 
		HttpPregressCallback progress_callback = nullptr) const;

	DWORD put(
		const std::wstring& url, 
		HttpHeadersPtr headers,
		const std::string& body, 
		std::string *resp, 
		HttpPregressCallback progress_callback = nullptr) const;

private:
    DWORD request(
		const std::wstring& url, 
		const std::wstring& method, 
		HttpHeadersPtr headers,
		const std::string& body, 
		std::string *resp, 
		HttpPregressCallback progress_callback = nullptr) const;

	TXCHttpClient(TXCHttpClient &other) = default;
	TXCHttpClient &operator=(const TXCHttpClient &other) = default;

private:
    std::wstring        _userAgent;
	TXCTaskQueue		*_queue;
	bool				_internalQueue;
};

typedef std::shared_ptr<TXCHttpClient> TXCHttpClientPtr;

#endif /* __HTTPCLIENT_H__ */
