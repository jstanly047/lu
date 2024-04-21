#pragma once

#include "BaseSocket.h"
#include <common/TemplateConstraints.h>
#include <platform/IFDEventHandler.h>
#include <string>
#include <platform/Defs.h>
#include <utils/Utils.h>

#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <string>

#include <glog/logging.h>

namespace lu::platform::socket
{
    template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ClientThread;

    template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
    class ConnectionThread;

    template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
    class ServerSingleThread;

    template <lu::common::NonPtrClassOrStruct DataSocketVCallback, lu::common::NonPtrClassOrStruct DataHandler, lu::common::NonPtrClassOrStruct SocketType=BaseSocket, typename CustomObjectPtrType=void>
    class DataSocketV : public lu::platform::IFDEventHandler
    {
        template<lu::common::NonPtrClassOrStruct ClientThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ClientThread;

        template<lu::common::NonPtrClassOrStruct ConnectionThreadCallback,  lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ConnectionThread;

        template<lu::common::NonPtrClassOrStruct ServerThreadCallback, lu::common::NonPtrClassOrStruct DataSocketType>
        friend class ServerSingleThread;
        
    public:
        DataSocketV(const DataSocketV &) = delete;
        DataSocketV &operator=(const DataSocketV &) = delete;
        DataSocketV(DataSocketV &&other) = delete;
        DataSocketV &operator=(DataSocketV &&other) = delete;

        DataSocketV(DataSocketVCallback &dataSocketCallback, SocketType &&baseSocket) : IFDEventHandler(IFDEventHandler::DataSocket),
                                                                                      m_socket(std::move(baseSocket)),
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

                if (m_socket.readDataSocket(m_dataHandler.getIOVect(), numbOfDataBuffer, numberOfBytesRead) == false)
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
            return m_socket.send(buffer, size);
        }

        int sendFile(int fileDescriptor, int size)
        {
            return m_socket.sendfile(fileDescriptor, size);
        }

        BaseSocket &getBaseSocket() { return m_socket; }
        SocketType &getSocket() { return m_socket; }
        const std::string &getIP() const { return m_socket.getIP(); }
        int getPort() const { return m_socket.getPort(); }

        int stop(ShutSide shutSide) { return m_socket.stop(shutSide); }
        void setCustomObjectPtr(CustomObjectPtrType * ptr) { m_customObjectPtr = ptr; }
        CustomObjectPtrType* getCustomObjectPtr() const { return m_customObjectPtr; }

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
                // TODO : Instead of calling close better call different callback
                LOG(ERROR) << "Read failed for data socket FD[" << (int) m_socket.getFD() << "]!";
                m_dataSocketCallback.onClientClose(*this);
                return false;
            }

            return true;
        }

        const lu::platform::FileDescriptor &getFD() const override final { return m_socket.getFD(); }

    protected:
        SocketType m_socket;
        DataSocketVCallback &m_dataSocketCallback;
        DataHandler m_dataHandler;
        ssize_t m_headerSize{};
        ssize_t m_numberOfBytesInBuffer{};
        ssize_t m_readOffset{};
        ssize_t m_numberOfBytesLeftToRead{};
        ssize_t m_numberOfBytesLeftToRecv{};
        CustomObjectPtrType* m_customObjectPtr;
    };
}
