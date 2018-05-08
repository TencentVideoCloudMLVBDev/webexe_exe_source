#include "GetPushUrlInfo.h"
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "log.h"

#define DEFAULT_HOST L"https://lvb.qcloud.com/weapp/utils"

GetPushUrlInfo::GetPushUrlInfo()
	: m_business_host(DEFAULT_HOST)
	, m_http_client(L"User-Agent")
{
	moveToThread(&m_thread);
	qRegisterMetaType<txfunction>("txfunction");
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
	m_thread.start();
}

GetPushUrlInfo::~GetPushUrlInfo()
{
	m_thread.quit();
	m_thread.wait();
}

void GetPushUrlInfo::handle(txfunction func)
{
	func();
}

GetPushUrlInfo * GetPushUrlInfo::shared_instance()
{
	static GetPushUrlInfo GetPushUrlInfo;
	return &GetPushUrlInfo;
}

void GetPushUrlInfo::getPushUrl()
{
	qDebug() << QStringLiteral("正在获取PushUrl信息...");

	emit dispatch([=]() {

		std::wstring url = m_business_host + L"/get_test_pushurl";

		std::vector<std::wstring> headers;
		headers.push_back(L"Content-Type: application/json; charset=utf-8");

		std::string resp_data;
		DWORD ret = m_http_client.http_get(url, headers, resp_data);
		LINFO(L"%s", QString::fromUtf8(resp_data.c_str()).toStdWString().c_str());
		if (resp_data.empty())
		{
			emit getPushUrl_finished(-1, QString::fromLocal8Bit("new服务暂时不可用!"), "");
			return;
		}

		on_getPushUrl_finished(resp_data);
	});
}

void GetPushUrlInfo::setProxy(const std::string& ip, unsigned short port)
{
    m_http_client.setProxy(ip, port);
}

void GetPushUrlInfo::on_getPushUrl_finished(const std::string & data_str)
{
	int error_code = -1;
	QString error_info = QStringLiteral("获取PushUrl失败!");
	QString push_url;

	QByteArray data(data_str.c_str(), data_str.size());

	do {
		int len = data.size();
		QJsonParseError json_error;
		QJsonDocument parse_doucment = QJsonDocument::fromJson(data, &json_error);
		if (json_error.error != QJsonParseError::NoError)
		{
			break;
		}
		if (!parse_doucment.isObject())
		{
			break;
		}

		QJsonObject obj = parse_doucment.object();

		if (obj.contains("url_push"))
		{
			push_url = obj.value("url_push").toString().toUtf8().data();
			qDebug() << QStringLiteral("获取PushUrl信息成功");
			emit getPushUrl_finished(0, "", push_url);
			return;
		}

	} while (0);

	emit getPushUrl_finished(error_code, error_info, "");
	qDebug() << QStringLiteral("获取PushUrl信息失败");
}
