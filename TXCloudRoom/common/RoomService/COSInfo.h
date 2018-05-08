#pragma once
#include <string>
#include <memory>
#include "json.h"

class Req
{
public:
	Req(const std::string &cmd, const std::string &subcmd, const bool verify) : cmd_(cmd), subcmd_(subcmd), verify_(verify) {}
	std::string GetCmd() const;
	std::string GetSubCmd() const;
	bool GetVerify() const;
	virtual std::string GenReq() const = 0;
	virtual ~Req() {}
protected:
	std::string cmd_;
	std::string subcmd_;
	bool verify_;
};

class Rsp
{
public:
	Rsp() : error_code_(-1), error_msg_("")
	{
	}

	Rsp(const int code, const std::string& msg) : error_code_(code), error_msg_(msg)
	{
	}

	int GetCode() const;
	std::string GetMsg() const;
	void* GetData() const;
	virtual bool Parse(const std::string& buf) = 0;

	virtual ~Rsp()
	{
	}

	void* data_;
protected:
	int error_code_;
	std::string error_msg_;
};

class CosSigReq : public Req
{
public:
	CosSigReq(const std::string &path, const std::string &bucket)
		: Req("open_cos_svc", "get_cos_sign", true), type_(1), path_(path), bucket_(bucket) {}
	virtual std::string GenReq() const override;
private:
	int type_;
	std::string path_;
	std::string bucket_;
};

class CosSigRsp : public Rsp
{
public:
	virtual bool Parse(const std::string& buf) override;
	std::string GetSig() const;
	std::string GetBucket() const;
	std::string GetRegion() const;
private:
	std::string sig_;
	std::string bucket_;
	std::string region_;
};