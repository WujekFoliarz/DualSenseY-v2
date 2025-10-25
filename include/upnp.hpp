#pragma once
#include <cstdint>
#include <cstdio>
#include <miniupnpc.h>
#include <upnpcommands.h>
#include <portlistingparse.h>
#include <upnperrors.h>

bool ForwardPort(const char* Port, const char* AppName);
bool DeletePort(const char* Port);