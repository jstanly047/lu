#pragma once

#include "BaseSocket.h"
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <string>
#include <platform/defs.h>
#include <utils/Utils.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <string>

namespace lu::platform::socket
{
    template <lu::common::NonPtrClassOrStruct DataSocketVCallback, lu::common::NonPtrClassOrStruct DataHandler>
    class DataSocketV : public lu::platform::IFDEventHandler
    {
    public:
        DataSocketV(const DataSocketV &) = delete;
        DataSocketV &operator=(const DataSocketV &) = delete;
        DataSocketV(DataSocketV &&other) = delete;
        DataSocketV &operator=(DataSocketV &&other) = delete;

        DataSocketV(DataSocketVCallback &dataSocketCallback, BaseSocket &&baseSocket) : m_baseSocket(std::move(baseSocket)),
                                                                                      m_dataSocketCallback(dataSocketCallback),
                                                                                      m_dataHandler(),
                                                                                      m_headerSize(m_dataHandler.getHeaderSize()),
                                                                                      m_numberOfBytesLeftToRecv(m_dataHandler.getReceiveBufferSize())

        {
        }

        virtual ~DataSocketV() {}

        bool Receive()
        {
            for (;;)
            {
                ssize_t numberOfBytesRead = 0;
                //m_dataHandler.getReceiveBufferToFill() + m_readOffset + m_numberOfBytesLeftToRead;
                auto writeableOffset = m_readOffset + m_numberOfBytesLeftToRead;
                int numbOfDataBuffer = m_dataHandler.getNumberOfBuffers(writeableOffset);

                if (lu::utils::Utils::readDataSocket(m_baseSocket.getFD(), m_dataHandler.getIOVect(), numbOfDataBuffer, numberOfBytesRead) == false)
                {
                    return false;
                }

                if (numberOfBytesRead == 0)
                {
                    return true;
                }

                m_numberOfBytesLeftToRead += numberOfBytesRead;
                
                readMessages();

                if (m_numberOfBytesLeftToRead == 0)
                {
                    m_readOffset = 0;
                    m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize();
                }
                else if (m_readOffset >=  m_dataHandler.getSingleBufferSize())
                {
                    m_dataHandler.swap();
                    m_readOffset -= m_dataHandler.getSingleBufferSize();
                    m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize() - m_numberOfBytesLeftToRead;
                    assert(m_numberOfBytesLeftToRecv >= 0u);
                }
            }

            return true;
        }

        int sendMsg(void *buffer, ssize_t size)
        {
            if (m_baseSocket.getFD() == nullptr)
            {
                return false;
            }

            ssize_t totalSent = 0;
            auto uint8_tBuffer = reinterpret_cast<uint8_t *>(buffer);

            while (totalSent < size)
            {
                // TODO we can try replace this by ::writev (scatter/gather IO)
                ssize_t numBytesSend = ::send(m_baseSocket.getFD(), uint8_tBuffer + totalSent, size, MSG_DONTWAIT);

                if (numBytesSend < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    return totalSent;
                }
                else if (numBytesSend != size)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOBUFS)
                    {
                        totalSent += numBytesSend;
                        size -= numBytesSend;
                        continue;
                    }

                    return totalSent;
                }

                totalSent += numBytesSend;
            }

            return totalSent;
        }

        int sendFile(int fileDescriptor, int size)
        {
            off_t offset = 0;
            ssize_t sendBytes = ::sendfile(m_baseSocket.getFD(), fileDescriptor, &offset, size);

            if (sendBytes == -1)
            {
                return false;
            }

            int totalSent = 0;

            while (totalSent < size)
            {
                ssize_t numBytesSend = ::sendfile(m_baseSocket.getFD(), fileDescriptor, &offset, size);

                if (numBytesSend < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    return totalSent;
                }
                else if (numBytesSend != (ssize_t)size)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ENOBUFS)
                    {
                        totalSent += numBytesSend;
                        size -= numBytesSend;
                        continue;
                    }

                    return totalSent;
                }

                totalSent += numBytesSend;
            }

            return sendBytes;
        }

        BaseSocket &getBaseSocket() { return m_baseSocket; }
        const std::string &getIP() const { return m_baseSocket.getIP(); }
        int getPort() const { return m_baseSocket.getPort(); }

    private:
        inline void readMessages()
        {
            ssize_t expectedMsgSize = 0;

            for (;;)
            {
                if (m_numberOfBytesLeftToRead >= m_headerSize)
                {
                    // TODO the reader function must handle the alignment and endianess
                    expectedMsgSize = m_dataHandler.readHeader(m_readOffset);
                    // updateForDataRead(m_headerSize);
                }
                else
                {
                    return;
                }

                if (m_numberOfBytesLeftToRead >= expectedMsgSize)
                {
                    m_dataSocketCallback.onData(*this, m_dataHandler.readMessage(m_readOffset, expectedMsgSize));
                    updateForDataRead(expectedMsgSize);
                    continue;
                }

                return;
            }
        }

        inline void updateForDataRead(ssize_t size)
        {
            m_readOffset += size;
            m_numberOfBytesLeftToRead -= size;
        }

        void onEvent(struct ::epoll_event &event) override final
        {
            if ((event.events & EPOLLHUP || event.events & EPOLLERR))
            {
                m_dataSocketCallback.onClientClose(*this);
                return;
            }
            else if (!(event.events & EPOLLIN))
            {
                return;
            }

            if (Receive() == false)
            {
                // TODO : Instead of calling close better call different callback
                m_dataSocketCallback.onClientClose(*this);
            }
        }

        const lu::platform::FileDescriptor &getFD() const override final { return m_baseSocket.getFD(); }

    protected:
        BaseSocket m_baseSocket;
        DataSocketVCallback &m_dataSocketCallback;
        DataHandler m_dataHandler;
        ssize_t m_headerSize{};
        ssize_t m_numberOfBytesInBuffer{};
        ssize_t m_readOffset{};
        ssize_t m_numberOfBytesLeftToRead{};
        ssize_t m_numberOfBytesLeftToRecv{};
    };
}
