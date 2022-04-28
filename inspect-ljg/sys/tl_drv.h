#pragma once

#ifndef TL_DRV_H_H_H
#define TL_DRV_H_H_H

#include <ntddk.h>
#include <wdf.h>

#include <fwpsk.h>

#include <fwpmk.h>

#define INITGUID
#include <guiddef.h>

#define NUMBER_BUCKETS		127
#define MAX_DNS_ENTRIES		1024
#define WFP_DEVICE_NAME		L"\\device\\inspect"
#define WFP_DEVICE_SYMBOL	L"\\DosDevices\\inspect"


#pragma pack(1)
typedef struct {
	const GUID* layerKey;
	const GUID* calloutKey;
	FWPS_CALLOUT_NOTIFY_FN notifyFn;
	FWPS_CALLOUT_CLASSIFY_FN classifyFn;
	wchar_t* name;
	wchar_t* description;
	UINT32* calloutId;
} callout_t;
#pragma pack()

/* Callout and sublayer GUIDs. */

/* 2e207682-d95f-4525-b966-969f26587f03 */
DEFINE_GUID(
	TL_INSPECT_SUBLAYER,
	0x2e207682,
	0xd95f,
	0x4525,
	0xb9, 0x66, 0x96, 0x9f, 0x26, 0x58, 0x7f, 0x03
);

/* 76b743d4-1249-4614-a632-6f9c4d08d25a */
DEFINE_GUID(
	TL_LAYER_STREAM_V4,
	0x76b743d4,
	0x1249,
	0x4614,
	0xa6, 0x32, 0x6f, 0x9c, 0x4d, 0x08, 0xd2, 0x5a
);

/* ac80683a-5b84-43c3-8ae9-eddb5c0d23c2 */
DEFINE_GUID(
	TL_LAYER_STREAM_V6,
	0xac80683a,
	0x5b84,
	0x43c3,
	0x8a, 0xe9, 0xed, 0xdb, 0x5c, 0x0d, 0x23, 0xc2
);

/* bb6e405b-19f4-4ff3-b501-1a3dc01aae01 */
DEFINE_GUID(
	TL_LAYER_DATAGRAM_V4,
	0xbb6e405b,
	0x19f4,
	0x4ff3,
	0xb5, 0x01, 0x1a, 0x3d, 0xc0, 0x1a, 0xae, 0x01
);

/* cabf7559-7c60-46c8-9d3b-2155ad5cf83f */
DEFINE_GUID(
	TL_LAYER_DATAGRAM_V6,
	0xcabf7559,
	0x7c60,
	0x46c8,
	0x9d, 0x3b, 0x21, 0x55, 0xad, 0x5c, 0xf8, 0x3f
);

/* 07248379-248b-4e49-bf07-24d99d52f8d0 */
DEFINE_GUID(
	TL_LAYER_ALE_EP_CLOSURE_V4,
	0x07248379,
	0x248b,
	0x4e49,
	0xbf, 0x07, 0x24, 0xd9, 0x9d, 0x52, 0xf8, 0xd0
);

/* 6d126434-ed67-4285-925c-cb29282e0e06 */
DEFINE_GUID(
	TL_LAYER_ALE_EP_CLOSURE_V6,
	0x6d126434,
	0xed67,
	0x4285,
	0x92, 0x5c, 0xcb, 0x29, 0x28, 0x2e, 0x0e, 0x06
);

#endif