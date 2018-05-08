#ifndef __TIM_MSG_H__
#define __TIM_MSG_H__

#include "tim_comm.h"
#include "tim_conv.h"

namespace imcore {

class TIMTextElem;
class TIMImageElem;
class TIMSoundElem;
class TIMFileElem;
class TIMCustomElem;
class TIMLocationElem;
class TIMFaceElem;


struct TIMAPNsInfo
{
	std::string sound;
	uint32_t badgeMode; // 0:正常显示 1:不计入badge
};

struct TIMAndroidOfflinePushInfo
{
	std::string title;
	std::string sound;
	uint32_t notifyMode; //推送通知模式：未设置或0-普通通知栏消息(点击通知消息后不会给应用回调); 0x1-自定义消息（点击通知消息后会给应用回调）
};

struct TIMOfflinePushInfo
{
	std::string desc;
	std::string ext;
	//std::string sound;
	TIMOfflinePushFlag flag;
	TIMAPNsInfo apns;
	TIMAndroidOfflinePushInfo androidInfo;
};

class TIM_DECL TIMElem {
public:
	TIMElemType type() const;
	void set_type(TIMElemType type);

	void Release();

	TIMTextElem *GetTextElem();
	TIMImageElem *GetImageElem();
	TIMSoundElem *GetSoundElem();
	TIMFileElem *GetFileElem();
	TIMCustomElem *GetCustomElem();
	TIMLocationElem *GetLocationElem();
	TIMFaceElem *GetFaceElem();


protected:
	TIMElemType type_;
protected:
	TIMElem(){}
};

class TIM_DECL TIMTextElem : public TIMElem {
public:
	TIMTextElem();
	void set_content(const std::string &content);
	const std::string &content() const;

private:
	std::string content_;
};


class TIM_DECL TIMImage
{
public:
	TIMImage(TIMImageType type, const std::string& url, const uint32_t size, const uint32_t height, const uint32_t width)
		:type_(type)
		,image_url_(url)
		,image_size_(size)
		,image_height_(height)
		,image_width_(width)
	{}
	~TIMImage(){};
	void GetImageFile(std::string& filename, TIMCallBack *cb);
	const std::string& GetUrl() const {return image_url_;}
	uint32_t GetSize() const{return image_size_;}
	uint32_t GetHeight() const {return image_height_;}
	uint32_t GetWidth() const {return image_width_;}
	TIMImageType GetType() const{return type_;}
private:
	TIMImageType type_;
	std::string image_url_;
	uint32_t image_size_;
	uint32_t image_height_;
	uint32_t image_width_;

friend class TIMMessageImpl;
};

class TIM_DECL TIMImageElem : public TIMElem {
public:
	TIMImageElem();
	const std::string &uuid() const;

	void set_path(const std::string &path);
	const std::string &path() const;

	void set_taskid(const int task_id);
	const int taskid() const;

	void set_level(const TIM_IMAGE_COMPRESS_TYPE type)
	{
		compress_type_ = type;
	}
	
	const TIM_IMAGE_COMPRESS_TYPE leval()
	{
		return compress_type_;
	}
	
	std::vector<TIMImage>& GetImages()  {return images_;}
	bool CancelUploading();
	int GetImageUploadProgrss();
	int GetImageFormat();
private:
	void SetImages(std::vector<TIMImage>& images) {images_ = images;}

	
	void set_uuid(const std::string &uuid);

	std::string uuid_;
	std::string path_;
	int task_id_;
	int format_;
	TIM_IMAGE_COMPRESS_TYPE compress_type_;
	std::vector<TIMImage> images_;

friend class TIMMessageImpl;
friend class TransImageElem;
};

class TIM_DECL TIMSoundElem : public TIMElem {
public:
	TIMSoundElem();
	void GetSound(TIMValueCallBack<const std::string &> *cb);

	const std::string &uuid() const;
	void set_data(const std::string &data);
	uint32_t data_size();
	void set_duration(uint32_t duration_);
	uint32_t duration();
	void set_path(const std::string& path);
	uint32_t task_id()const;;

	void set_business_id(uint32_t business_id);
	void set_download_flag(uint32_t download_flag);
	void set_urls(std::vector<std::string> urls);
private:
	const std::string &data() const;
	void set_uuid(const std::string &uuid);
	void set_data_size(uint32_t data_size);
	std::string uuid_;
	std::string data_;
	std::string path_;
	uint32_t data_size_;
	uint32_t duration_;
	uint32_t task_id_;

	uint32_t business_id_;
	uint32_t download_flag_;
	std::vector<std::string> urls_;
friend class TIMMessageImpl;
};

class TIM_DECL TIMFileElem : public TIMElem {
public:
	TIMFileElem();
	void GetFile(TIMValueCallBack<const std::string &> *cb);

	const std::string &uuid() const;
	void set_data(const std::string &data);
	uint32_t file_size();
	void set_file_name(const std::string &file_name);
	const std::string &file_name() const;
	void set_file_path(const std::string &path);
	uint32_t task_id() const;;

	void set_business_id(uint32_t business_id);
	void set_download_flag(uint32_t download_flag);
	void set_urls(std::vector<std::string> urls);
	static void GetFileByUUID(const std::string &uuid, const uint32_t business_id, TIMValueCallBack<const std::string &> *cb);

private:
	const std::string &data() const;
	void set_uuid(const std::string &uuid);
	void set_file_size(uint32_t file_size);
	std::string uuid_;
	std::string data_;
	uint32_t file_size_;
	std::string file_name_;
	std::string path_;
	uint32_t task_id_;

	uint32_t business_id_;
	uint32_t download_flag_;
	std::vector<std::string> urls_;

friend class TIMMessageImpl;
};

class TIM_DECL TIMCustomElem : public TIMElem {
public:
	TIMCustomElem();
	const std::string &data() const;
	void set_data(const std::string &data);
	const std::string &desc() const;
	void set_desc(const std::string &desc);
	void set_ext(const std::string& ext);
	const std::string &ext() const;
	const std::string& sound() const;
	void set_sound(const std::string& sound);

private:
	std::string data_;
	std::string desc_;
	std::string ext_;
	std::string sound_;
};

class TIM_DECL TIMLocationElem : public TIMElem {
public:
	TIMLocationElem();
	const std::string &desc() const;
	void set_desc(const std::string &desc);
	double longitude();
	void set_longitude(double longitude);
	double latitude();
	void set_latitude(double latitude);

private:
	std::string desc_;
	double longitude_;
	double latitude_;
};

class TIM_DECL TIMFaceElem : public TIMElem {
public:
	TIMFaceElem();
	const std::string &data() const;
	void set_data(const std::string &data);
	int index();
	void set_index(int index);

private:
	std::string data_;
	int index_;
};

class TIM_DECL TIMMessage {
public:
	TIMMessage();
	TIMMessage(const TIMMessage &o);
	~TIMMessage();
	int AddElem(const TIMElem *elem);
	int GetElemCount() const;
	TIMElem* GetElem(int i) const;
	TIMConversation conversation() const;
	uint32_t time() const;
	bool is_self() const;
	bool is_read() const;
	int status() const;
	uint64_t flag() const;
	const std::string GetSender()const;
	const std::string GetID()const;
	int GetCustomInt();
	const std::string GetCustomStr();
	bool SetCustomInt(int custom);
	bool SetCustomStr(const std::string& custom);
	bool Remove();
	bool DelFromStorage();
	uint64_t UniqueID() const;
	uint64_t GetRand() const;
	uint64_t GetSeq() const;

	bool ConvertToImportedMsg();
	bool SetTime(uint32_t time);
	bool SetSender(const std::string& id); 

	TIMMsgPriority Priority(); 
	void SetPriority(TIMMsgPriority priority);
	int GetRecvFlag();

	bool SetOfflinePushInfo(const TIMOfflinePushInfo& info);
	TIMOfflinePushInfo GetOfflinePushInfo();

	static std::string SerializeMessages(std::vector<TIMMessage> &msgs);
	static std::vector<TIMMessage> ParseMessageFromBuffer(std::string buff);

	void *impl();

private:
	void *impl_;

friend class TIMConversation;
};

class TIMDraft
{
public:
	std::vector<TIMElem> elems;
	std::string user_define;
	uint64_t time;
};

class TIM_DECL TIMMessageCallBack {
public:
	virtual ~TIMMessageCallBack() {}
	virtual void OnNewMessage(const std::vector<TIMMessage> &msgs) {}
};

class TIM_DECL TIMMessageOptResult {
public:
	TIMMessage message;
	int result;
};

}

#endif
