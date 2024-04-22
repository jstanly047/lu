#pragma once

#include "BaseSocket.h"
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <platform/Defs.h>

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <string>

#include <glog/logging.h>

namespace lu::platform::socket
{
    template <lu::common::NonPtrClassOrStruct DataSocketCallback, lu::common::NonPtrClassOrStruct DataHandler, lu::common::NonPtrClassOrStruct SocketType=BaseSocket, typename CustomObjectPtrType=void>
    class DataSocket : public lu::platform::IFDEventHandler
    {
    public:
        DataSocket(const DataSocket &) = delete;
        DataSocket &operator=(const DataSocket &) = delete;
        DataSocket(DataSocket &&other) = delete;
        DataSocket &operator=(DataSocket &&other) = delete;

        DataSocket(DataSocketCallback &dataSocketCallback, SocketType &&baseSocket) : IFDEventHandler(IFDEventHandler::DataSocket),
                                                                                      m_socket(std::move(baseSocket)),
                                                                                      m_dataSocketCallback(dataSocketCallback),
                                                                                      m_dataHandler(),
                                                                                      m_headerSize(m_dataHandler.getHeaderSize()),
                                                                                      m_receiveBufferShiftSize(m_dataHandler.getReceiveBufferSize() / 4),
                                                                                      m_numberOfBytesLeftToRecv(m_dataHandler.getReceiveBufferSize())

        {
        }

        virtual ~DataSocket() {}
       
        bool Receive()
        {
            for(;;)
            {
                ssize_t numberOfBytesRead = 0;

                if (m_socket.readDataSocket(m_dataHandler.getReceiveBufferToFill() + m_readOffset + m_numberOfBytesLeftToRead, m_numberOfBytesLeftToRecv, numberOfBytesRead) == false)
                {
                    return false;
                }

                if (numberOfBytesRead == 0)
                {
                    break;
                }

                m_numberOfBytesLeftToRead += numberOfBytesRead;
                readMessages();

                if (m_numberOfBytesLeftToRead == 0)
                {
                    m_readOffset = 0;
                    m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize();
                }
                else if (m_numberOfBytesLeftToRead <= m_receiveBufferShiftSize)
                {
                    std::memcpy(m_dataHandler.getReceiveBufferToFill(), m_dataHandler.getReceiveBufferToFill() + m_readOffset, m_numberOfBytesLeftToRead);
                    m_readOffset = 0;
                    m_numberOfBytesLeftToRecv = m_dataHandler.getReceiveBufferSize() - m_numberOfBytesLeftToRead;
                    assert(m_numberOfBytesLeftToRecv > 0u);
                }
            }

            return true;
        }

        int sendMsg(void *buffer, ssize_t size)
        {
            return m_socket.send(buffer, size);
        }

        int sendFile(int fileDescriptor, ssize_t size)
        {
            return m_socket.sendFile(fileDescriptor, size);
        }

        BaseSocket &getBaseSocket() { return m_socket; }
        SocketType &getSocket() { return m_socket; }
        const std::string &getIP() const { return m_socket.getIP(); }
        int getPort() const { return m_socket.getPort(); }

        int close()
        {
            int retVal = m_socket.close();

            if (retVal = 0)
            {
                delete this;
            }

            return retVal;
        }

        int stop(ShutSide shutSide) { return m_socket.stop(shutSide); }
        void setCustomObjectPtr(CustomObjectPtrType * ptr) { m_customObjectPtr = ptr; }
        CustomObjectPtrType* getCustomObjectPtr() const { return m_customObjectPtr; }
        bool isWebSocket() const { return false;  }

    private:
        inline void readMessages()
        {
            unsigned int expectedMsgSize = 0;

            for (;;)
            {
                if (m_numberOfBytesLeftToRead >= m_headerSize)
                {
                    // TODO the reader function must handle the alignment and endianess
                    expectedMsgSize = m_dataHandler.readHeader(m_readOffset);
                    // updateForDataRead(m_headerSize);

                    if (expectedMsgSize == 0U)
                    {
                        LOG(ERROR) << "Invalid message size 0 in [" <<  m_socket.getIP() << "]";
                        this->stop(ShutdownReadWrite);
                    }
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

        bool onEvent(struct ::epoll_event &event) override final
        {
            if ((event.events & EPOLLHUP || event.events & EPOLLERR))
            {
                m_dataSocketCallback.onClientClose(*this);
                return false;
            }
            else if (!(event.events & EPOLLIN))
            {
                return true;
            }

            if (Receive() == false)
            {
                // TODO : May be different callback since this some IO read error
                LOG(ERROR) << "Read failed for data socket FD[" << (int) m_socket.getFD() << "]!";
                m_dataSocketCallback.onClientClose(*this);
                return false;
            }

            return true;
        }

        const lu::platform::FileDescriptor &getFD() const override final { return m_socket.getFD(); }

    protected:
        SocketType m_socket;
        DataSocketCallback &m_dataSocketCallback;
        DataHandler m_dataHandler;
        ssize_t m_headerSize{};
        ssize_t m_receiveBufferShiftSize{};
        ssize_t m_numberOfBytesInBuffer{};
        ssize_t m_readOffset{};
        ssize_t m_numberOfBytesLeftToRead{};
        ssize_t m_numberOfBytesLeftToRecv{};
        CustomObjectPtrType* m_customObjectPtr;
    };
}
