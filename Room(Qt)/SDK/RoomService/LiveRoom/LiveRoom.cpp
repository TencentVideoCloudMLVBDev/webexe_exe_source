#include "LiveRoom.h"
#include "LRBussiness.h"

LiveRoom::LiveRoom()
    : m_impl(new LRBussiness())
{

}

LiveRoom::~LiveRoom()
{
    if (m_impl)
    {
        delete m_impl;
        m_impl = nullptr;
    }
}

LiveRoom * LiveRoom::instance()
{
	static LiveRoom * Live_room = new LiveRoom;
	return Live_room;
}

void LiveRoom::setCallback(ILiveRoomCallback * callback)
{
    if (m_impl)
    {
        m_impl->setCallback(callback);
    }
}

void LiveRoom::setProxy(const std::string& ip, unsigned short port)
{
    if (m_impl)
    {
        m_impl->setProxy(ip, port);
    }
}

void LiveRoom::login(const std::string & serverDomain, const LRAuthData & authData, ILoginLiveCallback* callback)
{
    if (m_impl)
    {
        m_impl->login(serverDomain, authData, callback);
    }
}

void LiveRoom::recordVideo()
{
	if (m_impl)
	{
		m_impl->recordVideo();
	}
}

void LiveRoom::logout()
{
    if (m_impl)
    {
        m_impl->logout();
    }
}

void LiveRoom::getRoomList(int index, int count, IGetLiveRoomListCallback* callback)
{
    if (m_impl)
    {
        m_impl->getRoomList(index, count, callback);
    }
}

void LiveRoom::getAudienceList(const std::string& roomID)
{
    if (m_impl)
    {
        m_impl->getAudienceList(roomID);
    }
}

void LiveRoom::createRoom(const std::string& roomID, const std::string& roomInfo)
{
    if (m_impl)
    {
        m_impl->createRoom(roomID, roomInfo);
    }
}

void LiveRoom::enterRoom(const std::string& roomID, HWND rendHwnd, const RECT & rect)
{
    if (m_impl)
    {
        m_impl->enterRoom(roomID, rendHwnd, rect);
    }
}

void LiveRoom::leaveRoom()
{
    if (m_impl)
    {
        m_impl->leaveRoom();
    }
}

void LiveRoom::sendRoomTextMsg(const char * msg)
{
    if (m_impl)
    {
        m_impl->sendRoomTextMsg(msg);
    }
}

void LiveRoom::sendRoomCustomMsg(const char * cmd, const char * msg)
{
    if (m_impl)
    {
        m_impl->sendRoomCustomMsg(cmd, msg);
    }
}

void LiveRoom::sendC2CCustomMsg(const char* userID, const char * cmd, const char * msg)
{
    if (m_impl)
    {
        m_impl->sendC2CCustomMsg(userID, cmd, msg);
    }
}

void LiveRoom::startLocalPreview(HWND rendHwnd, const RECT & rect)
{
    if (m_impl)
    {
        m_impl->startLocalPreview(rendHwnd, rect);
    }
}

void LiveRoom::updateLocalPreview(HWND rendHwnd, const RECT & rect)
{
    if (m_impl)
    {
        return m_impl->updateLocalPreview(rendHwnd, rect);
    }
}

void LiveRoom::stopLocalPreview()
{
    if (m_impl)
    {
        m_impl->stopLocalPreview();
    }
}

void LiveRoom::addRemoteView(HWND rendHwnd, const RECT & rect, const char * userID)
{
    if (m_impl)
    {
        m_impl->addRemoteView(rendHwnd, rect, userID);
    }
}

void LiveRoom::removeRemoteView(const char * userID)
{
    if (m_impl)
    {
        m_impl->removeRemoteView(userID);
    }
}

bool LiveRoom::startScreenPreview(HWND rendHwnd, HWND captureHwnd, const RECT & renderRect, const RECT & captureRect)
{
    if (m_impl)
    {
        return m_impl->startScreenPreview(rendHwnd, captureHwnd, renderRect, captureRect);
    }
    else
    {
        return false;
    }
}

void LiveRoom::stopScreenPreview()
{
    if (m_impl)
    {
        m_impl->stopScreenPreview();
    }
}

void LiveRoom::setMute(bool mute)
{
    if (m_impl)
    {
        m_impl->setMute(mute);
    }
}

void LiveRoom::setVideoQuality(LRVideoQuality quality, LRAspectRatio ratio)
{
    if (m_impl)
    {
        m_impl->setVideoQuality(quality, ratio);
    }
}

void LiveRoom::setBeautyStyle(LRBeautyStyle beautyStyle, int beautyLevel, int whitenessLevel)
{
    if (m_impl)
    {
        m_impl->setBeautyStyle(beautyStyle, beautyLevel, whitenessLevel);
    }
}

void LiveRoom::requestJoinPusher()
{
    if (m_impl)
    {
        m_impl->requestJoinPusher();
    }
}

void LiveRoom::acceptJoinPusher(const std::string& userID)
{
    if (m_impl)
    {
        m_impl->acceptJoinPusher(userID);
    }
}

void LiveRoom::rejectJoinPusher(const std::string& userID, const std::string& reason)
{
    if (m_impl)
    {
        m_impl->rejectJoinPusher(userID, reason);
    }
}

void LiveRoom::kickoutSubPusher(const std::string& userID)
{
    if (m_impl)
    {
        m_impl->kickoutSubPusher(userID);
    }
}

int LiveRoom::enumCameras(wchar_t ** camerasName, size_t capacity)
{
	if (m_impl)
	{
		return m_impl->enumCameras(camerasName, capacity);
	}
	return 0;
}

void LiveRoom::switchCamera(int cameraIndex)
{
	if (m_impl)
	{
		return m_impl->switchCamera(cameraIndex);
	}
}

int LiveRoom::micDeviceCount()
{
	if (m_impl)
	{
		return m_impl->micDeviceCount();
	}
	return 0;
}

bool LiveRoom::micDeviceName(unsigned int index, char name[MIC_DEVICE_NAME_MAX_SIZE])
{
	if (m_impl)
	{
		return m_impl->micDeviceName(index, name);
	}
	return false;
}

void LiveRoom::selectMicDevice(unsigned int index)
{
	if (m_impl)
	{
		m_impl->selectMicDevice(index);
	}
}

unsigned int LiveRoom::micVolume()
{
	if (m_impl)
	{
		return m_impl->micVolume();
	}
	return 0;
}

void LiveRoom::setMicVolume(unsigned int volume)
{
	if (m_impl)
	{
		return m_impl->setMicVolume(volume);
	}
}
