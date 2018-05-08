#pragma once
#include "HttpClient.h"
#include <QObject>
#include <functional>
#include <QtCore>

typedef std::function<void(void)>     txfunction;

class GetPushUrlInfo : public QObject
{
	Q_OBJECT

public:
	GetPushUrlInfo();
	~GetPushUrlInfo();
	static GetPushUrlInfo* shared_instance();
	void getPushUrl();

    void setProxy(const std::string& ip, unsigned short port);
private:
	HttpClient m_http_client;
	std::wstring m_business_host;
	QThread m_thread;
private:
	void on_getPushUrl_finished(const std::string& data_str);
protected slots:
	void handle(txfunction func);       // 执行函数
signals:
	void dispatch(txfunction func);     // 投递线程队列
	void getPushUrl_finished(int errorCode, QString errorInfo, QString pushUrl);
};
