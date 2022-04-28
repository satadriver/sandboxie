#ifndef INSPECT_H
#define INSPECT_H



#include "packet_pool.h"

BOOL FillPacket(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
    _Inout_opt_ void* layerData,
    _Out_ packet_t* packet);

NTSTATUS StreamNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                      _In_ const GUID* filterKey,
                      _Inout_ const FWPS_FILTER* filter);

void StreamClassify(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
                    _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
                    _Inout_opt_ void* layerData,
#if (NTDDI_VERSION >= NTDDI_WIN7)
                    _In_opt_ const void* classifyContext,
#endif
                    _In_ const FWPS_FILTER* filter,
                    _In_ UINT64 flowContext,
                    _Inout_ FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS DatagramNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                        _In_ const GUID* filterKey,
                        _Inout_ const FWPS_FILTER* filter);

void DatagramClassify(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
                      _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
                      _Inout_opt_ void* layerData,
#if (NTDDI_VERSION >= NTDDI_WIN7)
                      _In_opt_ const void* classifyContext,
#endif
                      _In_ const FWPS_FILTER* filter,
                      _In_ UINT64 flowContext,
                      _Inout_ FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS AleClosureNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                          _In_ const GUID* filterKey,
                          _Inout_ const FWPS_FILTER* filter);

void AleClosureClassify(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
                        _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
                        _Inout_opt_ void* layerData,
#if (NTDDI_VERSION >= NTDDI_WIN7)
                        _In_opt_ const void* classifyContext,
#endif
                        _In_ const FWPS_FILTER* filter,
                        _In_ UINT64 flowContext,
                        _Inout_ FWPS_CLASSIFY_OUT* classifyOut);

//#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - offsetof(packet_t, payload))

#define MAX_PAYLOAD_SIZE 0x10000

#endif /* INSPECT_H */
