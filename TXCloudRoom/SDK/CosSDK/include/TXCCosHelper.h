#pragma once
#pragma warning(disable:4996)
#include "TXCHttpClient.h"

typedef std::function<void(int code, bool done)> CosUploadCallback;

typedef std::function<void(std::string file_content)> CosDownloadCallback;

typedef std::function<void(int page_count, std::string file_content)> CosPreviewCallback;

class TXCCosHelper
{
public:
	TXCCosHelper();
	~TXCCosHelper();

	void setAppID(const std::wstring &appid);
	void setBucket(const std::wstring &bucket);
	void setRegion(const std::wstring &region);
	void setPath(const std::wstring &path);

	const std::wstring &appid() const;
	const std::wstring &bucket() const;
	const std::wstring &region() const;
	const std::wstring &path() const;

	bool uploadObject(
		const std::string &file_content, 
		const std::wstring &obj_name, 
		const std::string &sig, 
		CosUploadCallback callback) const;

	bool uploadObject(
		const std::wstring &file_path, 
		const std::wstring &obj_name, 
		const std::string &sig, 
		CosUploadCallback callback) const;

	bool downloadObject(
		const std::wstring &obj_name, 
		const std::wstring &file_path, 
		CosDownloadCallback callback) const;

	bool downloadFile(
		const std::wstring &file_url,
		const std::wstring &file_path,
		CosDownloadCallback callback) const;

	bool previewObject(
		const std::wstring &obj_name, 
		int page, 
		const std::wstring &file_path, 
		CosPreviewCallback callback) const;

	std::wstring getUploadUrl(const std::wstring &obj_name) const;

	std::wstring getDownloadUrl(const std::wstring &obj_name) const;

	std::wstring getPreviewUrl(const std::wstring &obj_name, int page) const;

	std::shared_ptr<TXCHttpClient> httpClient() const;

	static std::wstring getFileName(const std::wstring &file_path);

private:
	static bool readFile(const std::wstring &file_path, std::string &content);
	static bool writeFile(const std::wstring &file_path, std::string &content);

	TXCCosHelper(const TXCCosHelper& other) = default;
	TXCCosHelper &operator=(const TXCCosHelper& other) = default;

private:
	std::wstring _appid;
	std::wstring _bucket;
	std::wstring _region;
	std::wstring _path;

	std::shared_ptr<TXCHttpClient> _client;
};
