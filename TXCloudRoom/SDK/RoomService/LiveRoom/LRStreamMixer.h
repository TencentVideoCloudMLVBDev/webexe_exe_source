#ifndef __STREAMMIXER_H__
#define __STREAMMIXER_H__

#include "HttpClient.h"
#include "LRHttpRequest.h"

#include <functional>
#include <vector>

#include "json.h"

struct LRStreamMixParam
{
    std::string streamID;
    int width;
    int height;
};

class LRStreamMixer
{
public:
    LRStreamMixer(LRHttpRequest* httpRequest);
    ~LRStreamMixer();

    void setSdkAppID(int sdkAppID);
    void setRoomID(const std::string& roomID);
    void setUserID(const std::string& userID);
	void setPictureID(int picture_id);

    void setMainStream(const std::string& streamID, int width, int height);
    void addSubStream(const std::string& streamID, int width, int height);
    void removeSubStream(const std::string& streamID);
    void reset();
	void mergeStream(int retryCount);
private:
    Json::Value createJsonParams();
private:
    LRHttpRequest* m_httpRequest;

    int m_sdkAppID;
    std::string m_roomID;
    std::string m_userID;

    LRStreamMixParam m_mainStream;
    std::vector<LRStreamMixParam> m_subStream;
	int m_pictureID = -1;
};

#endif /* __STREAMMIXER_H__ */
