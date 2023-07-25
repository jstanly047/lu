#include <platform/socket/IDataHandler.h>
#include <platform/socket/IDataSocketCallback.h>

using namespace lu::platform::socket;


template<lu::common::NonPtrClassOrStruct DataSocketCallback>
IDataHandler<DataSocketCallback>::IDataHandler(DataSocketCallback& dataSocketCallback) :  
    m_dataSocketCallback(dataSocketCallback) 
{

}

template<lu::common::NonPtrClassOrStruct DataSocketCallback>
IDataHandler<DataSocketCallback>::IDataHandler(IDataHandler&& other) noexcept: 
    m_dataSocketCallback(other.m_dataSocketCallback) 
{}

template<lu::common::NonPtrClassOrStruct DataSocketCallback>
IDataHandler<DataSocketCallback>& IDataHandler<DataSocketCallback>::operator=(IDataHandler&& other)
{
    std::swap(m_dataSocketCallback, other.m_dataSocketCallback);
    return *this;
}

template class IDataHandler<IDataSocketCallback>;