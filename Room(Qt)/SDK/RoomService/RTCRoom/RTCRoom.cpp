#include "RTCRoom.h"
#include "RTCRBussiness.h"

RTCRoom::RTCRoom()
    : m_impl(new RTCRBussiness())
{

}

RTCRoom::~RTCRoom()
{
    if (m_impl)
    {
        delete m_impl;
        m_impl = nullptr;
    }
}

RTCRoom * RTCRoom::instance()
{
	static RTCRoom * Live_room = new RTCRoom;
	return Live_room;
}

void RTCRoom::setCallback(IRTCRoomCallback * callback)
{
    if (m_impl)
    {
        m_impl->setCallback(callback);
    }
}

void RTCRoom::setProxy(const std::string& ip, unsigned short port)
{
    if (m_impl)
    {
        m_impl->setProxy(ip, port);
    }
}

void RTCRoom::login(const std::string & serverDomain, const RTCAuthData & authData, ILoginRTCCallback* callback)
{
    if (m_impl)
    {
        m_impl->login(serverDomain, authData, callback);
    }
}

void RTCRoom::recordVideo(bool multi)
{
	if (m_impl)
	{
		m_impl->recordVideo(multi);
	}
}

void RTCRoom::logout()
{
    if (m_impl)
    {
        m_impl->logout();
    }
}

void RTCRoom::getRoomList(int index, int count, IGetRTCRoomListCallback* callback)
{
    if (m_impl)
    {
        m_impl->getRoomList(index, count, callback);
    }
}

void RTCRoom::createRoom(const std::string& roomID, const std::string& roomInfo)
{
    if (m_impl)
    {
        m_impl->createRoom(roomID, roomInfo);
    }
}

void RTCRoom::enterRoom(const std::string& roomID)
{
    if (m_impl)
    {
        m_impl->enterRoom(roomID);
    }
}

void RTCRoom::leaveRoom()
{
    if (m_impl)
    {
        m_impl->leaveRoom();
    }
}

void RTCRoom::sendRoomTextMsg(const char * msg)
{
    if (m_impl)
    {
        m_impl->sendRoomTextMsg(msg);
    }
}

void RTCRoom::sendRoomCustomMsg(const char * cmd, const char * msg)
{
    if (m_impl)
    {
        m_impl->sendRoomCustomMsg(cmd, msg);
    }
}

void RTCRoom::sendC2CCustomMsg(const char* userID, const char * cmd, const char * msg)
{
    if (m_impl)
    {
        m_impl->sendC2CCustomMsg(userID, cmd, msg);
    }
}

void RTCRoom::startLocalPreview(HWND rendHwnd, const RECT & rect)
{
    if (m_impl)
    {
        m_impl->startLocalPreview(rendHwnd, rect);
    }
}

void RTCRoom::updateLocalPreview(HWND rendHwnd, const RECT & rect)
{
    if (m_impl)
    {
        m_impl->updateLocalPreview(rendHwnd, rect);
    }
}

void RTCRoom::stopLocalPreview()
{
    if (m_impl)
    {
        m_impl->stopLocalPreview();
    }
}

bool RTCRoom::startScreenPreview(HWND rendHwnd, HWND captureHwnd, const RECT & renderRect, const RECT & captureRect)
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

void RTCRoom::stopScreenPreview()
{
    if (m_impl)
    {
        m_impl->stopScreenPreview();
    }
}

void RTCRoom::addRemoteView(HWND rendHwnd, const RECT & rect, const char * userID)
{
    if (m_impl)
    {
        m_impl->addRemoteView(rendHwnd, rect, userID);
    }
}

void RTCRoom::updateRemotePreview(HWND rendHwnd, const RECT & rect, const char * userID)
{
    if (m_impl)
    {
        m_impl->updateRemotePreview(rendHwnd, rect, userID);
    }
}

void RTCRoom::removeRemoteView(const char * userID)
{
    if (m_impl)
    {
        m_impl->removeRemoteView(userID);
    }
}

void RTCRoom::setMute(bool mute)
{
    if (m_impl)
    {
        m_impl->setMute(mute);
    }
}

void RTCRoom::setBeautyStyle(RTCBeautyStyle beautyStyle, int beautyLevel, int whitenessLevel)
{
    if (m_impl)
    {
        m_impl->setBeautyStyle(beautyStyle, beautyLevel, whitenessLevel);
    }
}

int RTCRoom::enumCameras(wchar_t ** camerasName, size_t capacity)
{
	if (m_impl)
	{
		return m_impl->enumCameras(camerasName, capacity);
	}
	return 0;
}

void RTCRoom::switchCamera(int cameraIndex)
{
	if (m_impl)
	{
		return m_impl->switchCamera(cameraIndex);
	}
}

int RTCRoom::micDeviceCount()
{
	if (m_impl)
	{
		return m_impl->micDeviceCount();
	}
	return 0;
}

bool RTCRoom::micDeviceName(unsigned int index, char name[MIC_DEVICE_NAME_MAX_SIZE])
{
	if (m_impl)
	{
		return m_impl->micDeviceName(index, name);
	}
	return false;
}

void RTCRoom::selectMicDevice(unsigned int index)
{
	if (m_impl)
	{
		m_impl->selectMicDevice(index);
	}
}

unsigned int RTCRoom::micVolume()
{
	if (m_impl)
	{
		return m_impl->micVolume();
	}
	return 0;
}

void RTCRoom::setMicVolume(unsigned int volume)
{
	if (m_impl)
	{
		return m_impl->setMicVolume(volume);
	}
}
