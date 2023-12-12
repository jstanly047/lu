#pragma once

namespace lu::platform::socket
{
    constexpr unsigned int TCP_MTU=1500;
    constexpr unsigned int RECV_BUFFER_SIZE=TCP_MTU * 4;
}

