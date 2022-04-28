#pragma once

#ifndef MYPACKET_H_H_H
#define MYPACKET_H_H_H

#include <ntddk.h>

#include "packet_pool.h"

#define URL_LIMIT_SIZE 1024

BOOLEAN isIntranet(unsigned long ipv4);

void setDnsTarget(DWORD ip);



unsigned short __ntohs(unsigned short v);

unsigned int __ntohl(unsigned int v);

int isHttp(char* http);

char* stripHttpMethod(char* http);

int processMyPacket(const FWPS_INCOMING_VALUES* inFixedValues, void* layerData);

int getHostFromDns(unsigned char* data, char* name);

int getHostFromHttpPacket(char* pack, int packlen, char* host);

int getUrlFromHttpPacket(char* pack, int packlen, char* url);

int processDnsPacket(packet_t* packet);

char* mystrstr(char* data, int datalen, char* section, int sectionlen);

#endif