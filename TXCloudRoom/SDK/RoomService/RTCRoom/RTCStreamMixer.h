#ifndef __STREAMMIXER_H__
#define __STREAMMIXER_H__

#include "HttpClient.h"
#include "RTCRHttpRequest.h"

#include <functional>
#include <vector>

#include "json.h"

struct RTCStreamMixParam
{
    std::string streamID;
    int width;
    int height;
};

class RTCStreamMixer
{
public:
    RTCStreamMixer(RTCRHttpRequest* httpRequest);
    ~RTCStreamMixer();

    void setSdkAppID(int sdkAppID);
    void setRoomID(const std::string& roomID);
    void setUserID(const std::string& userID);
	void setMixType(bool multi);

    void setMainStream(const std::string& streamID, int width, int height);
    void addSubStream(const std::string& streamID, int width, int height);
    void removeSubStream(const std::string& streamID);
	void mergeStream(int retryCount);
    void reset();
private:
    Json::Value createMultiJsonParams();
	Json::Value createDoubleJsonParams();
private:
    RTCRHttpRequest* m_httpRequest;

    int m_sdkAppID;
    std::string m_roomID;
    std::string m_userID;

    RTCStreamMixParam m_mainStream;
    std::vector<RTCStreamMixParam> m_subStream;
	bool m_multi = true;
};

#endif /* __STREAMMIXER_H__ */
