#include "COSInfo.h"

std::string Req::GetCmd() const
{
	return cmd_;
}

std::string Req::GetSubCmd() const
{
	return subcmd_;
}

bool Req::GetVerify() const
{
	return verify_;
}

int Rsp::GetCode() const
{
	return error_code_;
}

std::string Rsp::GetMsg() const
{
	return error_msg_;
}

void* Rsp::GetData() const
{
	return data_;
}

std::string CosSigReq::GenReq() const
{
	Json::Value data;
	data["cmd"] = cmd_;
	data["sub_cmd"] = subcmd_;
	data["type"] = type_;
	data["file_path"] = path_;
	if (!bucket_.empty())
		data["bucket"] = bucket_;
	Json::FastWriter writer;
	return writer.write(data);
}

bool CosSigRsp::Parse(const std::string& buf)
{
	Json::Reader reader;
	Json::Value data;
	if (!reader.parse(buf, data))
	{
		return false;
	}
	else
	{
		error_code_ = data["error_code"].asInt();
		error_msg_ = data["error_msg"].asString();
		sig_ = data["sign"].asString();
		bucket_ = data["bucket"].asString();
		region_ = data["region"].asString();

		return true;
	}
}

std::string CosSigRsp::GetSig() const
{
	return sig_;
}

std::string CosSigRsp::GetBucket() const
{
	return bucket_;
}

std::string CosSigRsp::GetRegion() const
{
	if (region_ == "tj")
	{
		return "ap-beijing-1";
	}
	else if (region_ == "bj")
	{
		return "ap-beijing";
	}
	else if (region_ == "sh")
	{
		return "ap-shanghai";
	}
	else if (region_ == "gz")
	{
		return "ap-guangzhou";
	}
	else if (region_ == "cd")
	{
		return "ap-chengdu";
	}
	else if (region_ == "sgp")
	{
		return "ap-singapore";
	}
	else if (region_ == "hk")
	{
		return "ap-hongkong";
	}
	else if (region_ == "ca")
	{
		return "na-toronto";
	}
	else if (region_ == "ger")
	{
		return "eu-frankfurt";
	}
	else {
		return region_;
	}
}