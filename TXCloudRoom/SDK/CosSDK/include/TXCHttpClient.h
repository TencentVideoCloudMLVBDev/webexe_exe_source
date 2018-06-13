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

typedef std::function<void(DWORD code, std::string& resp, const std::vector<std::string>& respHeaders)> HttpResponseCallback;

enum class HttpAction {
	Request,
	Response
};

typedef std::function<void(HttpAction action, DWORD currentSize, DWORD totalSize, char *fragData, DWORD fragSize)> HttpPregressCallback;

class IHttpCallback
{
public:
	virtual void outputLogInfo(std::string log) = 0;
};

class TXCHttpClient : public std::enable_shared_from_this<TXCHttpClient>
{
public:
	TXCHttpClient(TXCTaskQueue *queue = nullptr);
	~TXCHttpClient();

    void setProxy(const std::string& ip, unsigned short port);

	void asyn_get(
		const std::string& url,
        const std::vector<std::string>& headers,
		HttpResponseCallback response_callback = nullptr,
		HttpPregressCallback progress_callback = nullptr);

	void asyn_post(
        const std::string& url,
        const std::vector<std::string>& headers,
		const std::string& body,
		HttpResponseCallback response_callback = nullptr,
		HttpPregressCallback progress_callback = nullptr);

	void asyn_put(
        const std::string& url,
        const std::vector<std::string>& headers,
		const std::string& body,
		HttpResponseCallback response_callback = nullptr,
		HttpPregressCallback progress_callback = nullptr);

    DWORD get(
        const std::string& url,
        const std::vector<std::string>& headers,
        std::string *resp,
        std::vector<std::string>* respHeaders,
		const std::string userAgent = "",
		HttpPregressCallback progress_callback = nullptr,
		IHttpCallback *http_callback = nullptr);

	DWORD post(
		const std::string& url,
        const std::vector<std::string>& headers,
		const std::string& body,
		std::string *resp,
        std::vector<std::string>* respHeaders,
		const std::string userAgent = "",
		HttpPregressCallback progress_callback = nullptr,
		IHttpCallback *http_callback = nullptr);

	DWORD put(
		const std::string& url,
        const std::vector<std::string>& headers,
		const std::string& body,
		std::string *resp,
        std::vector<std::string>* respHeaders,
		const std::string userAgent = "",
		HttpPregressCallback progress_callback = nullptr,
		IHttpCallback *http_callback = nullptr);

	void setCallback(IHttpCallback* callback);

	void stopTask();

    static std::wstring a2w(const std::string& str, unsigned int codePage = CP_ACP);
    static std::string w2a(const std::wstring& wstr, unsigned int codePage = CP_ACP);
private:
	TXCHttpClient(TXCHttpClient &other) = default;
	TXCHttpClient &operator=(const TXCHttpClient &other) = default;

private:
	TXCTaskQueue	*_queue;
	bool		_internalQueue;
	IHttpCallback*	m_callback;

    std::string _proxyIP;
    unsigned short _proxyPort;
};

typedef std::shared_ptr<TXCHttpClient> TXCHttpClientPtr;

#endif /* __HTTPCLIENT_H__ */
