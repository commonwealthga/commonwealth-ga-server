#pragma once

#include <windows.h>
// #include <stdio.h>
// #include <time.h>
#include <math.h>
// #include <string>
// #include <fstream>
// #include <sstream>
// #include <iomanip>
// #include <chrono>
#include <stdarg.h>
#include <cstdio>
// #include <unordered_set>
// #include <unordered_map>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <unistd.h>
#define ASIO_DISABLE_IOCP
#define ASIO_NO_WIN32_LEAN_AND_MEAN
#include "lib/asio-1.34.2/include/asio.hpp"
#ifndef __C_ASSERT__
#define __C_ASSERT__(x) typedef char __C_ASSERT__[(x) ? 1 : -1]
#endif
#include "lib/detours/detours.h"
#include <map>
#include "src/SDK/SdkHeaders.h"

