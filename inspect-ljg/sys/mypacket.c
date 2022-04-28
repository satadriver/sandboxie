
#include "mypacket.h"
#include "inspect.h"
#include "packet_processor.h"
#include "packet.h"

#include "list.h"


NTSYSAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);

//A类地址：10.0.0.0--10.255.255.255
//B类地址：172.16.0.0--172.31.255.255
//C类地址：192.168.0.0--192.168.255.255
//169.254.1.0/16
BOOLEAN isIntranet(unsigned long ip) {
	unsigned long ipv4 = __ntohl(ip);
	if (ipv4 > 0x0a000000 && ipv4 <= 0x0affffff)
	{
		return TRUE;
	}
	else if (ipv4 > 0xac100000 && ipv4 <= 0xac1fffff)
	{
		return TRUE;
	}
	else if (ipv4 > 0xc0a80000 && ipv4 <= 0xc0a8ffff)
	{
		return TRUE;
	}
	else if (ipv4 > 0xa9fe0100 && ipv4 <= 0xfea9ffff)
	{
		return TRUE;
	}
	return FALSE;
}


unsigned short __ntohs(unsigned short v) {
	return ((v & 0xff) << 8) + ((v & 0xff00) >> 8);
}


unsigned int __ntohl(unsigned int v) {
	return ((v & 0xff) << 24) + ((v & 0xff00) << 8) + ((v & 0xff0000) >> 8) + ((v & 0xff000000) >>24);
}


int getHostFromDns(unsigned char* data,char * name) {
	char* lpname = name;
	unsigned char* d = (unsigned char*)data;
	while (1)
	{
		unsigned int size = *d;
		if (size > 0 && size < 64)
		{
			d++;
			RtlCopyMemory(lpname, d, size);
			d += size;

			lpname += size;

			*lpname = '.';
			lpname++;
		}
		else if (size == 0)
		{
			if (lpname - name > 0)
			{
				lpname--;
				*lpname = 0;
			}
			else {
				*name = 0;
			}
			
			break;
		}
		else {
			*name = 0;
			break;
		}
	}

	return (int)(lpname - name);
}


int isHttp(char* http) {
	if (memcmp(http,"GET ",4) == 0 ||
		memcmp(http, "POST ", 5) == 0
// 		||
// 		memcmp(http, "CONNECT ", 8) == 0 ||
// 		memcmp(http, "HEAD ", 5) == 0 ||
// 		memcmp(http, "PUT ", 4) == 0 ||
// 		memcmp(http, "DELETE ", 7) == 0 ||
// 		memcmp(http, "TRACE ", 6) == 0 ||
// 		memcmp(http, "OPTIONS ", 8) == 0 
		)
	{
		return TRUE;
	}

	return FALSE;
}


char * stripHttpMethod(char* http) {
	if (memcmp(http, "GET ", 4) == 0) {
		return http + 4;
	}
	else if (memcmp(http, "POST ", 5) == 0)
	{
		return http + 5;
	}
// 	else if (memcmp(http, "CONNECT ", 8) == 0)
// 	{
// 		return http + 8;
// 	}
// 	else if (memcmp(http, "HEAD ", 5) == 0)
// 	{
// 		return http + 5;
// 	}
// 	else if (memcmp(http, "PUT ", 4) == 0)
// 	{
// 		return http + 4;
// 	}
// 	else if (memcmp(http, "DELETE ", 7) == 0)
// 	{
// 		return http + 7;
// 	}
// 	else if (memcmp(http, "TRACE ", 6) == 0)
// 	{
// 		return http + 6;
// 	}
// 	else if (memcmp(http, "OPTIONS ", 8) == 0)
// 	{
// 		return http + 8;
// 	}

	return FALSE;
}


char* mystrstr(char* data, int datalen, char* section, int sectionlen) {

	for (int i = 0;i < datalen - sectionlen;i ++)
	{
		if (memcmp(data + i,section,sectionlen) == 0)
		{
			return data + i;
		}
	}
	return FALSE;
}


int getUrlFromHttpPacket(char* pack, int packlen,char * url) {

	char* http = stripHttpMethod(pack);
	int httplen = packlen - (int)(http - pack);
	char* flag = " HTTP/1.";
	char* urlend = mystrstr(http, httplen, flag, (int)strlen(flag));
	if (urlend)
	{
		int urllen = (int)(urlend - http);
		if (urllen < URL_LIMIT_SIZE)
		{
			RtlCopyMemory(url, http, urllen);
			*(url + urllen) = 0;
			return urllen;
		}
	}
	return FALSE;
}



int getHostFromHttpPacket(char* pack,int packlen,char * host) {
	char* flag = "\r\nHost: ";
	int flaglen = (int)strlen(flag);
	char* hdr = mystrstr(pack, packlen, flag, flaglen);
	if (hdr)
	{
		hdr += flaglen;
		int contentlen = (int)(packlen - (hdr - pack));
		char* end = mystrstr(hdr, contentlen, "\r\n", 2);
		if (end)
		{
			int hostlen = (int)(end - hdr);
			if (hostlen < DOMAIN_LIMIT_SIZE)
			{
				RtlCopyMemory(host, hdr, hostlen);
				*(host + hostlen) = 0;
				return hostlen;
			}
		}	
	}
	return FALSE;
}


int processHttpPacket(packet_t* packet) {

	int result = 0;
	char* pack = (char*)(packet->lppayload);
	int packlen = packet->payloadlen;
	//"GET / HTTP/0.9\r\n" 16 bytes
	//"GET / HTTP/1.0\r\n\r\n" 18 bytes
	if (packlen >  32)
	{
		result = isHttp(pack);
		if (result)
		{
			char host[DOMAIN_LIMIT_SIZE];
			int hostlen = getHostFromHttpPacket(pack, packlen,host);

			char url[URL_LIMIT_SIZE];
			int urllen = getUrlFromHttpPacket(pack, packlen, url);

			DNS_ATTACK_LIST* list = searchDnslist(url, IOCTL_WFP_SDWS_ADD_URL);
			if (list) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

int processDnsPacket(packet_t* packet) {

	DNS_REQUEST* req = (DNS_REQUEST*)(packet->lppayload);
	int packlen = packet->payloadlen;
	if (packlen > sizeof(DNS_REQUEST) + 4 + sizeof(DNS_REQUEST_TYPECLASS) + sizeof(DNS_ANSWER_ADDRESS))
	{
		char name[DOMAIN_LIMIT_SIZE];

		int hostlen = getHostFromDns(req->queries, name);
		if (hostlen <= 0)
		{
			return FALSE;
		}

		DNS_ATTACK_LIST* list = searchDnslist(name, IOCTL_WFP_SDWS_ADD_DNS);
		if (list) {
			if (packet->ip_version == 4)
			{
				int reqsize = sizeof(DNS_REQUEST) + hostlen + 2 + sizeof(DNS_REQUEST_TYPECLASS);
				//int answersize = packlen - reqsize;

				DNS_ANSWER_HEAD* answer = (DNS_ANSWER_HEAD*)(packet->lppayload + reqsize);

				int cnt = __ntohs(req->answerRRS);
				for (int i = 0; i < cnt; i++)
				{
					if (answer->type == 0x0500)	//cname
					{

					}
					else if (answer->type == 0x0100)
					{
						DNS_ANSWER_ADDRESS* addr = (DNS_ANSWER_ADDRESS*)answer;

						//RtlCopyMemory((char*)answer, (char*)&g_dstDsnAnswer, sizeof(DNS_ANSWER_ADDRESS));
						//47.116.51.29
						addr->address = 0;
					}
					answer = (DNS_ANSWER_HEAD*)((unsigned char*)answer + sizeof(DNS_ANSWER_HEAD) + __ntohs(answer->datelen));
				}
			}
			else if (packet->ip_version == 6)
			{

			}
		}
	}
	return FALSE;
}

 




int processMyPacket(const FWPS_INCOMING_VALUES* inFixedValues,void * layerData) {
	int result = 0;

	UCHAR tmpdata[MAX_PACKET_SIZE + 16];

	packet_t* mypacket = (packet_t*)tmpdata;

	result = FillPacket(inFixedValues, layerData, mypacket);
	if (result )
	{
		//ProcessPacket(mypacket);
		//return 0;
		//__debugbreak();

		PEPROCESS peproc = PsGetCurrentProcess();
		UCHAR* pefp = PsGetProcessImageFileName(peproc);
		
		if (pefp && *pefp)
		{
			upperstr((char*)pefp);
			PROCESS_NAME_LIST* proclist = searchProcesslist((char*)pefp);
			if (proclist)
			{
				DbgPrint("find process:%s", pefp);
				return TRUE;
			}
		}

		
		if ( FWP_DIRECTION_OUTBOUND == mypacket->direction && isIntranetEnable())
		{
			if (mypacket->ip_version == 4)
			{
				//__debugbreak();
				DWORD remoteip = *(unsigned long*)mypacket->remote_ip;
				if ( isIntranet(remoteip) == FALSE)
				{
					return TRUE;
				}
			}	
		}

		if (FWP_DIRECTION_INBOUND == mypacket->direction &&  mypacket->protocol == IPPROTO_UDP )
		{
			if (mypacket->remote_port == 53)
			{
				result = processDnsPacket(mypacket);
				return result;
			}
		}
		else if (FWP_DIRECTION_OUTBOUND == mypacket->direction && mypacket->protocol == IPPROTO_TCP )
		{
			result = filterPacket(mypacket);
			if (result)
			{
				return result;
			}

			result = processHttpPacket(mypacket);
			if (result)
			{
				return result;
			}
		}
	}

	return FALSE;
}