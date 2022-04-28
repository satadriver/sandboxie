
#include "list.h"
#include "packet_processor.h"
#include "packet.h"
#include "tl_drv.h"

KSPIN_LOCK g_spinlock = 0;

DNS_ATTACK_LIST* g_dnsAttackList = 0;

IPPORT_ATTACK_LIST* g_ipportAttackList = 0;

PROCESS_NAME_LIST* g_processList = 0;

DNS_ANSWER_ADDRESS g_dstDnsAnswer = { 0 };

BOOLEAN g_intranet = FALSE;



void setDnsTarget(DWORD ip) {
	g_dstDnsAnswer.address = ip;
}


void initAttackList() {

	g_dstDnsAnswer.address = 0;
	g_dstDnsAnswer.hdr.datelen = 0x0400;
	g_dstDnsAnswer.hdr.name = 0x0cc0;
	g_dstDnsAnswer.hdr.time2live = 0x1000;
	g_dstDnsAnswer.hdr.type = 0x0100;
	g_dstDnsAnswer.hdr.cls = 0x0100;

	KzInitializeSpinLock(&g_spinlock);
}


void uninitAttackList() {

}

void setIntranet(BOOLEAN enable) {
	g_intranet = enable;
}


BOOLEAN isIntranetEnable() {
	return g_intranet;
}

DNS_ATTACK_LIST* searchDnslist(char* dnsname,int type) {
	DNS_ATTACK_LIST* resultlist = 0;
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	DNS_ATTACK_LIST* list = g_dnsAttackList;
	do
	{
		if (list && list->host )
		{
			if (type == list->type && strstr(dnsname, list->host))
			{
				resultlist = list;
				break;
			}
			list = list->next;
		}
		else {
			break;
		}
	} while (list != g_dnsAttackList);

	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return resultlist;
}


int addDnsRule(char* dns, int structsize) {
	int result = FALSE;
	KLOCK_QUEUE_HANDLE lock_handle;
	
	unsigned int type = *(unsigned int*)dns;

	int size = structsize - sizeof(int) - 1;

	char data[DOMAIN_LIMIT_SIZE];
	memcpy(data, dns + sizeof(int), size);
	*(data + size) = 0;

	if (g_dnsAttackList == 0)
	{
		KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);
		g_dnsAttackList = (DNS_ATTACK_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(DNS_ATTACK_LIST), PACKET_POOL_TAG);
		if (g_dnsAttackList)
		{
// 			g_dnsAttackList->next = g_dnsAttackList;
// 			g_dnsAttackList->prev = g_dnsAttackList;
			g_dnsAttackList->next = 0;
			g_dnsAttackList->prev = 0;
			g_dnsAttackList->type = type;
			RtlCopyMemory(g_dnsAttackList->host, data, size);
			*(g_dnsAttackList->host + size) = 0;
			result = TRUE;
		}
		KeReleaseInStackQueuedSpinLock(&lock_handle);
	}
	else {
		DNS_ATTACK_LIST* list = searchDnslist(data,type);
		if (list == 0)
		{
			list = (DNS_ATTACK_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(DNS_ATTACK_LIST), PACKET_POOL_TAG);
			if (list)
			{
				KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);
				list->prev = g_dnsAttackList;
				list->next = g_dnsAttackList->next;
				list->type = type;

				DNS_ATTACK_LIST* next = g_dnsAttackList->next;
				g_dnsAttackList->next = list;
				if (next)
				{
					next->prev = list;
				}

				RtlCopyMemory(list->host, data, size);
				*(list->host + size) = 0;
				
				KeReleaseInStackQueuedSpinLock(&lock_handle);

				result = TRUE;
			}
		}
	}
	
	return result;
}


int clearDnsList() {
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	DNS_ATTACK_LIST* list = g_dnsAttackList;
	while (1)
	{
		if (list)
		{
			DNS_ATTACK_LIST* next = list->next;

			ExFreePoolWithTag(list, PACKET_POOL_TAG);

			list = next;
		}
		else {
			break;
		}
	}

	g_dnsAttackList = 0;
	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return FALSE;
}

int delDnsRule(char* data, int type) {
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	DNS_ATTACK_LIST* list = searchDnslist(data,type);
	if (list)
	{
		DNS_ATTACK_LIST* next = list->next;
		DNS_ATTACK_LIST* prev = list->prev;
		prev->next = next;
		next->prev = prev;
		ExFreePoolWithTag(list, PACKET_POOL_TAG);
		if (list == g_dnsAttackList)
		{
			g_dnsAttackList = 0;
		}
	}

	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return FALSE;
}



IPPORT_ATTACK_LIST* searchIPPortList(char* data,int type) {

	IPPORT_ATTACK_LIST* resultlist = 0;

	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	IPPORT_ATTACK_LIST* list = g_ipportAttackList;
	do
	{
		if (list )
		{
			if (type == IOCTL_WFP_SDWS_ADD_IPV4)
			{
				IPV4_PARAMS* ipv4 = (IPV4_PARAMS*)data;

				if (list->ipv4 == ipv4->ipv4 && list->ipv4mask == ipv4->mask && ipv4->dir == list->direction)
				{
					resultlist = list;
					break;
				}
			}else if (type == IOCTL_WFP_SDWS_ADD_PORT)
			{
				PORT_PARAMS* port = (PORT_PARAMS*)data;

				if (list->port == port->port && port->dir == list->direction)
				{
					resultlist = list;
					break;
				}
			}else if (type == IOCTL_WFP_SDWS_ADD_IPV6)
			{
				IPV6_PARAMS* ipv6 = (IPV6_PARAMS*)data;

				if ( memcmp(list->ipv6,ipv6->ipv6,16) == 0 && ipv6->dir == list->direction)
				{
					resultlist = list;
					break;
				}
			}
			else {
				break;
			}

			list = list->next;
		}
		else {
			break;
		}
	} while (list != g_ipportAttackList);

	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return resultlist;
}






int addIPPortRule(char* data, int type) {
	int result = FALSE;
	KLOCK_QUEUE_HANDLE lock_handle;	

	if (g_ipportAttackList == 0)
	{
		KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);
		g_ipportAttackList = (IPPORT_ATTACK_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(DNS_ATTACK_LIST), PACKET_POOL_TAG);
		if (g_ipportAttackList)
		{
// 			g_ipportAttackList->next = g_ipportAttackList;
// 			g_ipportAttackList->prev = g_ipportAttackList;
			g_ipportAttackList->next = 0;
			g_ipportAttackList->prev = 0;
			
			if (type == IOCTL_WFP_SDWS_ADD_IPV4)
			{
				IPV4_PARAMS* ipv4 = (IPV4_PARAMS*)data;
				g_ipportAttackList->direction = ipv4->dir;
				g_ipportAttackList->ipv4 = ipv4->ipv4;
				g_ipportAttackList->ipv4mask = ipv4->mask;
				g_ipportAttackList->type = type;
			}else if (type == IOCTL_WFP_SDWS_ADD_IPV6)
			{
				IPV6_PARAMS* ipv6 = (IPV6_PARAMS*)data;
				memcpy(g_ipportAttackList->ipv6, ipv6->ipv6, 16);
				g_ipportAttackList->direction = ipv6->dir;
				g_ipportAttackList->type = type;
			}else if (type == IOCTL_WFP_SDWS_ADD_PORT)
			{
				PORT_PARAMS * port = (PORT_PARAMS*)data;
				g_ipportAttackList->port = port->port;
				g_ipportAttackList->direction = port->dir;
				g_ipportAttackList->type = type;
			}

			result = TRUE;
		}
		KeReleaseInStackQueuedSpinLock(&lock_handle);
	}
	else {
		IPPORT_ATTACK_LIST* oldlist = searchIPPortList(data,type);
		if (oldlist == 0)
		{
			IPPORT_ATTACK_LIST * newlist = (IPPORT_ATTACK_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(DNS_ATTACK_LIST), PACKET_POOL_TAG);
			if (newlist)
			{
				KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

				newlist->prev = g_ipportAttackList;
				newlist->next = g_ipportAttackList->next;

				IPPORT_ATTACK_LIST* next = g_ipportAttackList->next;
				g_ipportAttackList->next = newlist;
				if (next)
				{
					next->prev = newlist;
				}

				if (type == IOCTL_WFP_SDWS_ADD_IPV4)
				{
					IPV4_PARAMS* ipv4 = (IPV4_PARAMS*)data;
					newlist->direction = ipv4->dir;
					newlist->ipv4 = ipv4->ipv4;
					newlist->ipv4mask = ipv4->mask;
					newlist->type = type;
				}
				else if (type == IOCTL_WFP_SDWS_ADD_IPV6)
				{
					IPV6_PARAMS* ipv6 = (IPV6_PARAMS*)data;
					memcpy(newlist->ipv6, ipv6->ipv6, 16);
					newlist->direction = ipv6->dir;
					newlist->type = type;
				}
				else if (type == IOCTL_WFP_SDWS_ADD_PORT)
				{
					PORT_PARAMS* port = (PORT_PARAMS*)data;
					newlist->port = port->port;
					newlist->direction = port->dir;
					newlist->type = type;
				}

				result = TRUE;

				KeReleaseInStackQueuedSpinLock(&lock_handle);
			}	
		}
	}

	return result;
}


int delIPPortRule(char* data, int type) {
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	IPPORT_ATTACK_LIST* list = searchIPPortList(data,type);
	if (list)
	{
		IPPORT_ATTACK_LIST* next = list->next;
		IPPORT_ATTACK_LIST* prev = list->prev;
		prev->next = next;
		next->prev = prev;
		ExFreePoolWithTag(list, PACKET_POOL_TAG);
		if (list == g_ipportAttackList)
		{
			g_ipportAttackList = 0;
		}
	}

	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return FALSE;
}



int clearIPPortList() {
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	IPPORT_ATTACK_LIST* list = g_ipportAttackList;
	while (1)
	{
		if (list)
		{
			IPPORT_ATTACK_LIST* next = list->next;

			ExFreePoolWithTag(list, PACKET_POOL_TAG);

			list = next;
		}
		else {
			break;
		}
	}

	g_ipportAttackList = 0;
	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return FALSE;
}


PROCESS_NAME_LIST* searchProcesslist(char* procname) {
	PROCESS_NAME_LIST* resultlist = 0;
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	PROCESS_NAME_LIST* list = g_processList;
	do
	{
		if (list && list->pname && list->pname[0])
		{
			NTSTATUS status = strcmp(procname, list->pname);
			if (status == 0)
			{
				resultlist = list;
				break;
			}
			list = list->next;
		}
		else {
			break;
		}
	} while (list != g_processList);

	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return resultlist;
}


int addProcessRule(char* buffer, int type) {
	int result = FALSE;
	KLOCK_QUEUE_HANDLE lock_handle;

	//__debugbreak();

	char* data = buffer + sizeof(DWORD);
	upperstr(data);

	DbgPrint("find process:%s", data);

	if (g_processList == 0)
	{
		KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);
		g_processList = (PROCESS_NAME_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_NAME_LIST), PACKET_POOL_TAG);
		if (g_processList)
		{
// 			g_processList->next = g_processList;
// 			g_processList->prev = g_processList;
			g_processList->next = 0;
			g_processList->prev = 0;

			if (type == IOCTL_WFP_SDWS_ADD_PROCESS)
			{
				strcpy(g_processList->pname, data);
			}

			result = TRUE;
		}
		KeReleaseInStackQueuedSpinLock(&lock_handle);
	}
	else {
		PROCESS_NAME_LIST* oldlist = searchProcesslist(data);
		if (oldlist == 0)
		{
			PROCESS_NAME_LIST* newlist = (PROCESS_NAME_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_NAME_LIST), PACKET_POOL_TAG);
			if (newlist)
			{
				KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

				newlist->prev = g_processList;
				newlist->next = g_processList->next;

				PROCESS_NAME_LIST* next = g_processList->next;
				g_processList->next = newlist;
				if (next)
				{
					next->prev = newlist;
				}

				if (type == IOCTL_WFP_SDWS_ADD_PROCESS)
				{
					strcpy(newlist->pname, data);
				}

				result = TRUE;

				KeReleaseInStackQueuedSpinLock(&lock_handle);
			}
		}
	}

	return result;
}



int clearProcessList() {
	KLOCK_QUEUE_HANDLE lock_handle;
	KeAcquireInStackQueuedSpinLock(&g_spinlock, &lock_handle);

	PROCESS_NAME_LIST* list = g_processList;
	while (1)
	{
		if (list)
		{
			PROCESS_NAME_LIST* next = list->next;

			ExFreePoolWithTag(list, PACKET_POOL_TAG);

			list = next;
		}
		else {
			break;
		}
	}

	g_processList = 0;
	KeReleaseInStackQueuedSpinLock(&lock_handle);
	return FALSE;
}


int upperstr(char* str) {
	int len = (int)strlen(str);
	for (int i = 0; i < len; i++)
	{
		if (str[i] >= 'a' && str[i] <= 'z')
		{
			str[i] -= 0x20;
		}
	}
	return len;
}


int lowerstr(char* str) {
	int len = (int)strlen(str);
	for (int i = 0; i < len; i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] += 0x20;
		}
	}
	return len;
}






int filterPacket(packet_t* packet) {

	IPPORT_ATTACK_LIST* list = g_ipportAttackList;
	do
	{
		if (list)
		{
			if (list->type == IOCTL_WFP_SDWS_ADD_IPV4 && packet->ip_version == 4)
			{
				if (list->ipv4 == *(DWORD*)packet->remote_ip || list->ipv4 == *(DWORD*)packet->local_ip)
				{
					return TRUE;
				}
			}
			else if (list->type == IOCTL_WFP_SDWS_ADD_IPV6 && packet->ip_version == 6)
			{
				if (memcmp(list->ipv6, packet->remote_ip, 16) == 0 || memcmp(list->ipv6, packet->local_ip, 16) == 0)
				{
					return TRUE;
				}
			}
			else if (list->type == IOCTL_WFP_SDWS_ADD_PORT)
			{
				if (list->port == packet->local_port || list->port == packet->remote_port)
				{
					return TRUE;
				}
			}
			else {
				break;
			}

			list = list->next;
		}
		else {
			break;
		}
	} while (list != g_ipportAttackList);

	return FALSE;
}
