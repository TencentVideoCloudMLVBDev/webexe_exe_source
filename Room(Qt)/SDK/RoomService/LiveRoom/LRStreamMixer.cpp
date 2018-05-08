#include "LRStreamMixer.h"
#include "log.h"
#include "Base.h"

#include <ctime>
#include <assert.h>
#include <strstream>

static const int MAX_SUBSTREAM_COUNT = 3;

LRStreamMixer::LRStreamMixer(LRHttpRequest* httpRequest)
    : m_httpRequest(httpRequest)
    , m_sdkAppID(0)
    , m_roomID("")
    , m_userID("")
    , m_mainStream()
    , m_subStream()
{

}

LRStreamMixer::~LRStreamMixer()
{

}

void LRStreamMixer::setSdkAppID(int sdkAppID)
{
    m_sdkAppID = sdkAppID;
}

void LRStreamMixer::setRoomID(const std::string& roomID)
{
    m_roomID = roomID;
}

void LRStreamMixer::setUserID(const std::string& userID)
{
    m_userID = userID;
}

void LRStreamMixer::setMainStream(const std::string& streamID, int width, int height)
{
    assert(640 == width && 360 == height);      // todo 仅支持

    m_mainStream.streamID = streamID;
    m_mainStream.width = width;
    m_mainStream.height = height;

    LINFO(L"streamID: %s, width: %d, height: %d", Ansi2Wide(streamID).c_str(), width, height);
}

void LRStreamMixer::addSubStream(const std::string& streamID, int width, int height)
{
    LINFO(L"streamID: %s, width: %d, height: %d, m_subStream.size: %lu", Ansi2Wide(streamID).c_str(), width, height, m_subStream.size());

    for (std::vector<LRStreamMixParam>::iterator it = m_subStream.begin()
        ; m_subStream.end() != it; )
    {
        if (streamID == it->streamID)
        {
            it = m_subStream.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (m_subStream.size() > MAX_SUBSTREAM_COUNT) {
        return;
    }

    m_subStream.push_back({ streamID, width, height });

    mergeStream(5);
}

void LRStreamMixer::removeSubStream(const std::string& streamID)
{
    for (std::vector<LRStreamMixParam>::iterator it = m_subStream.begin()
        ; m_subStream.end() != it; ++it)
    {
        if (streamID == it->streamID)
        {
            m_subStream.erase(it);
            mergeStream(1);

            break;
        }
    }
}

void LRStreamMixer::reset()
{
    m_mainStream.streamID.clear();
    m_mainStream.width = 0;
    m_mainStream.height = 0;;
    m_subStream.clear();
}

void LRStreamMixer::mergeStream(int retryCount)
{
    LINFO(L"retryCount: %d", retryCount);

    if (m_httpRequest && retryCount > 0)
    {
        Json::Value mergeParams = createJsonParams();
        m_httpRequest->mergeStream(m_roomID, m_userID, mergeParams, [=](const LRResult& res) {
            if (LIVEROOM_SUCCESS == res.ec)
            {
                LINFO(L"Merge stream success");
            }
            else
            {
                LINFO(L"Merge stream failed: %d, msg: %s", res.ec, Ansi2Wide(res.msg).c_str());
                this->mergeStream(retryCount - 1);
            }
        });
    }
}

Json::Value LRStreamMixer::createJsonParams()
{
    // input_stream_list
    Json::Value inputStreamList;
    int index = 1;

    int subWidth = (m_mainStream.width / MAX_SUBSTREAM_COUNT) / 2 * 2;
    int subHeight = subWidth / 2 * 2 ;        // 1 : 1

    // 背景画布
    if (true/*m_subStream.size() > 0*/)
    {
        Json::Value layoutParam;
        layoutParam["image_layer"] = index++;
        layoutParam["input_type"] = 3;

        layoutParam["image_width"] = m_mainStream.width;
        layoutParam["image_height"] = m_mainStream.height + subHeight;

        Json::Value canvasStream;
        canvasStream["input_stream_id"] = m_mainStream.streamID;
        canvasStream["layout_params"] = layoutParam;

        inputStreamList.append(canvasStream);
    }

    // 大主播
    {
        Json::Value layoutParam;
        layoutParam["image_layer"] = index++ ;
        layoutParam["image_width"] = m_mainStream.width;
        layoutParam["image_height"] = m_mainStream.height;
        layoutParam["location_x"] = 0;
        layoutParam["location_y"] = 0;

        Json::Value mainStream;
        mainStream["input_stream_id"] = m_mainStream.streamID;
        mainStream["layout_params"] = layoutParam;

        inputStreamList.append(mainStream);
    }

    int subLocationX = 0;
    int subLocationY = m_mainStream.height;

    // 小主播
    for (int i = 0; i < m_subStream.size(); ++i)
    {
        int actualSubWidth = subWidth;
        int actualSubHeight = subHeight;

        if (MAX_SUBSTREAM_COUNT - 1 == i)       // 最后一个小主播画面占满剩余空间
        {
            actualSubWidth = m_mainStream.width - subLocationX;
        }

        int srcWidth = m_subStream[i].width;
        int srcHeight = m_subStream[i].height;


        int cropWidth = srcWidth;
        int cropHeight = srcHeight;

        if (srcWidth * actualSubHeight > actualSubWidth * srcHeight)
        {
            cropHeight = srcHeight;
            cropWidth = (cropHeight * actualSubWidth / actualSubHeight) / 2 * 2;
        }
        else
        {
            cropWidth = srcWidth;
            cropHeight = (cropWidth * actualSubHeight / actualSubWidth) / 2 * 2;
        }

        int cropX = (srcWidth > cropWidth ? (srcWidth - cropWidth) / 4 * 2 : 0);;
        int cropY = (srcHeight > cropHeight ? (srcHeight - cropHeight) / 4 * 2 : 0);;

        Json::Value cropParams;
        cropParams["crop_x"] = cropX;
        cropParams["crop_y"] = cropY;
        cropParams["crop_width"] = cropWidth;
        cropParams["crop_height"] = cropHeight;

        Json::Value layoutParam;
        layoutParam["image_layer"] = index++;
        layoutParam["image_width"] = actualSubWidth;
        layoutParam["image_height"] = actualSubHeight;
        layoutParam["location_x"] = subLocationX;
        layoutParam["location_y"] = subLocationY;

        Json::Value subStream;
        subStream["input_stream_id"] = m_subStream[i].streamID;
        subStream["layout_params"] = layoutParam;
        subStream["crop_params"] = cropParams;

        inputStreamList.append(subStream);

        subLocationX += actualSubWidth;
    }

	//背景图
	for (int i = 0; i < MAX_SUBSTREAM_COUNT - m_subStream.size(); ++i)
	{
		int actualSubWidth = subWidth;
		int actualSubHeight = subHeight;

		if (MAX_SUBSTREAM_COUNT - m_subStream.size() - 1 == i)       // 最后一个背景画面占满剩余空间
		{
			actualSubWidth = m_mainStream.width - subLocationX;
		}

		Json::Value layoutParam;
		layoutParam["image_layer"] = index++;
		layoutParam["image_width"] = actualSubWidth;
		layoutParam["image_height"] = actualSubHeight;
		layoutParam["location_x"] = subLocationX;
		layoutParam["location_y"] = subLocationY;
		layoutParam["input_type"] = 2;
		layoutParam["picture_id"] = 33261;

		Json::Value bgStream;
		bgStream["layout_params"] = layoutParam;
		bgStream["input_stream_id"] = m_mainStream.streamID;

		inputStreamList.append(bgStream);

		subLocationX += actualSubWidth;
	}
    // para
    Json::Value para;
    para["app_id"] = m_sdkAppID;
    para["interface"] = "mix_streamv2.start_mix_stream_advanced";
    para["mix_stream_session_id"] = m_mainStream.streamID;
    para["output_stream_id"] = m_mainStream.streamID;
    para["input_stream_list"] = inputStreamList;

    // interface
    Json::Value interfaceObj;
    interfaceObj["interfaceName"] = "Mix_StreamV2";
    interfaceObj["para"] = para;

    time_t currentTime = ::time(NULL) + 30;

    // requestParam
    Json::Value requestParam;
    requestParam["timestamp"] = (long)currentTime;
    requestParam["eventId"] = (long)currentTime;
    requestParam["interface"] = interfaceObj;

    return requestParam;
}
