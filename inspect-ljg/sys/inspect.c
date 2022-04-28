#include <stddef.h>
#include <ndis.h>

#pragma warning(push)
#pragma warning(disable:4201) /* Unnamed struct/union. */

#include <fwpsk.h>

#pragma warning(pop)

#include "inspect.h"
#include "worker_thread.h"
#include "packet_pool.h"
#include "utils.h"

#include "mypacket.h"




/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** FillPacket.                                                               **
 **                                                                           **
 *******************************************************************************
 *******************************************************************************/

BOOL FillPacket(_In_ const FWPS_INCOMING_VALUES* inFixedValues, _Inout_opt_ void* layerData, _Out_ packet_t* packet)
{
	const FWPS_INCOMING_VALUE* values;
	UINT localAddrIndex;
	UINT remoteAddrIndex;
	UINT localPortIndex;
	UINT remotePortIndex;
	UINT directionIndex;
    UINT protocolIdx;
	UINT32 addr;
	NET_BUFFER* nb;
	UINT8* payload;

    if (packet == 0)
    {
        return FALSE;
    }

    if (!GetNetwork4TupleIndexesForLayer(inFixedValues->layerId,&localAddrIndex,&remoteAddrIndex,&localPortIndex,&remotePortIndex,&protocolIdx, &directionIndex)) {
		return FALSE;
	}
    
	values = inFixedValues->incomingValue;
    //FWP_DIRECTION_INBOUND == 1 or FWP_DIRECTION_OUTBOUND == 0
	packet->direction = values[directionIndex].value.int8;
	packet->local_port = values[localPortIndex].value.uint16;
	packet->remote_port = values[remotePortIndex].value.uint16;
    //packet->protocol = values[protocolIdx].value.uint8;
    packet->protocol = (UCHAR)protocolIdx;

    if (GetAddressFamilyForLayer(inFixedValues->layerId) == AF_INET) {
        packet->ip_version = 4;

        addr = RtlUlongByteSwap(values[localAddrIndex].value.uint32);
        memcpy(packet->local_ip, &addr, 4);

        addr = RtlUlongByteSwap(values[remoteAddrIndex].value.uint32);
        memcpy(packet->remote_ip, &addr, 4);
    } else {
        packet->ip_version = 6;

        memcpy(packet->local_ip, values[localAddrIndex].value.byteArray16->byteArray16,16);

        memcpy(packet->remote_ip,values[remoteAddrIndex].value.byteArray16->byteArray16, 16);
    }

    if ( layerData) {
        KeQuerySystemTime(&packet->timestamp);

        FWPS_STREAM_CALLOUT_IO_PACKET0* fwps_scip = (FWPS_STREAM_CALLOUT_IO_PACKET0 * )layerData;

        switch(packet->protocol)
        //switch (packet->remote_port) 
        {
          case IPPROTO_TCP:
          //case 80: /* HTTP. */
          {
              if (packet->direction == FWP_DIRECTION_OUTBOUND)
              {
                  if (fwps_scip->streamData && fwps_scip->streamData->netBufferListChain)
                  {
                      nb = NET_BUFFER_LIST_FIRST_NB(fwps_scip->streamData->netBufferListChain);
                      if (nb)
                      {
                          /* Get pointer to payload. */
                          payload = NdisGetDataBuffer(nb, nb->DataLength, NULL, 2, 0);
                          if (payload == NULL) {
                              //DbgPrint("payload null");
                              return FALSE;
                          }

                          packet->payloadlen = nb->DataLength;
                          if (packet->payloadlen > MAX_PAYLOAD_SIZE)
                          {
                              DbgPrint("nb->DataLength size:%u error", nb->DataLength);
                              //return FALSE;
                          }

                          packet->lppayload = payload;

                          //memcpy(packet->payload, payload, packet->payloadlen);
                          //__debugbreak();

                          return TRUE;
                      }
                      else {
                          DbgPrint("fwps_scip->streamData->netBufferListChain null");
                          return FALSE;
                      }
                  }
                  else {
                      DbgPrint("fwps_scip->streamData:%p", fwps_scip->streamData);
                      return FALSE;
                  }
              }
              break;
          }
//           case 443: /* HTTPS. */
//           {
// 			  if (fwps_scip->streamData && fwps_scip->streamData->netBufferListChain)
// 			  {
//                   nb = NET_BUFFER_LIST_FIRST_NB(fwps_scip->streamData->netBufferListChain);
//                   if (nb)
//                   {
// 					  /* Get pointer to payload. */
//                       payload = NdisGetDataBuffer(nb, nb->DataLength, NULL, 2, 0);
// 					  if (payload == NULL) {
//                           DbgPrint("payload null");
// 						  return FALSE;
// 					  }
// 
// 					  /* Payload won't be processed. */
// 					  packet->payloadlen = nb->DataLength;
// 					  if (packet->payloadlen > MAX_PAYLOAD_SIZE)
// 					  {
//                           DbgPrint("nb->DataLength size:%u error", nb->DataLength);
// 						  return FALSE;
// 					  }
// 
// 					  packet->lppayload = payload;
// 
// 					  //memcpy(packet->payload, payload, packet->payloadlen);
//                       return TRUE;
// 				  }
// 				  else {
//                       DbgPrint("fwps_scip->streamData->netBufferListChain null");
// 					  return FALSE;
// 				  }
// 			  }
// 			  else {
//                   DbgPrint("fwps_scip->streamData:%p", fwps_scip->streamData);
// 				  return FALSE;
// 			  }
// 
//               break;
//           }

          case IPPROTO_UDP:
          //case 53: /* DNS. */
          {
              if (packet->direction == FWP_DIRECTION_INBOUND && packet->remote_port == 53)
              {
                  nb = NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)layerData);
                  if (nb)
                  {
                      /* Get pointer to payload. */
                      payload = NdisGetDataBuffer(nb, nb->DataLength, NULL, 2, 0);
                      if (payload == NULL) {
                          //DbgPrint("payload null");
                          return FALSE;
                      }

                      packet->payloadlen = nb->DataLength;
                      if (packet->payloadlen > MAX_PAYLOAD_SIZE)
                      {
                          DbgPrint("nb->DataLength size:%u error", nb->DataLength);
                          //return FALSE;
                      }

                      packet->lppayload = payload;

                      //__debugbreak();

                      //memcpy(packet->payload, payload, packet->payloadlen);
                      return TRUE;
                  }
                  else {
                      //DbgPrint("nb null");
                      return FALSE;
                  }
              }
              break;
          }
          default: 
          {
              break;
          }
       }
    } else {
        packet->payloadlen = 0;
        //DbgPrint("layerdata null");
        return FALSE;
    }

    return FALSE;
}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Stream.                                                                   **
 **                                                                           **
 *******************************************************************************
 *******************************************************************************/

NTSTATUS StreamNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                      _In_ const GUID* filterKey,
                      _Inout_ const FWPS_FILTER* filter)
{
	UNREFERENCED_PARAMETER(notifyType);
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);

#if DEBUG
	DbgPrint("StreamNotify() value:%d", notifyType);
#endif

	return STATUS_SUCCESS;
}



void StreamClassify(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
                    _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
                    _Inout_opt_ void* layerData,
#if(NTDDI_VERSION >= NTDDI_WIN7)
                    _In_opt_ const void* classifyContext,
#endif
                    _In_ const FWPS_FILTER* filter,
                    _In_ UINT64 flowContext,
                    _Inout_ FWPS_CLASSIFY_OUT* classifyOut)
{
    packet_t* packet;

    UNREFERENCED_PARAMETER(inMetaValues);

    #if(NTDDI_VERSION >= NTDDI_WIN7)
    UNREFERENCED_PARAMETER(classifyContext);
    #endif

    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    #if DEBUG
    DbgPrint("StreamClassify()");
    #endif

    if (!(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE))
    {
	    return;
    }

    classifyOut->actionType = FWP_ACTION_PERMIT;
    int result = processMyPacket(inFixedValues, layerData);
    if (result)
    {
	    classifyOut->actionType = FWP_ACTION_BLOCK;
    }

    if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
    {
	    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
    return;

    /* Get packet from the packet pool. */
    if ((packet = PopPacket()) != NULL) {
    if (FillPacket(inFixedValues, layerData, packet)) {
        if (!GivePacketToWorkerThread(packet)) {
        /* Return packet to packet pool. */
        PushPacket(packet);
        }
    } else {
        /* Return packet to packet pool. */
        PushPacket(packet);
    }
    }

    FWPS_STREAM_CALLOUT_IO_PACKET0* pkt =
                                    (FWPS_STREAM_CALLOUT_IO_PACKET0*) layerData;

    /* If we want to get all the data, use:
    * pkt-> streamAction = FWP_ACTION_CONTINUE;
    */
    pkt->streamAction = FWPS_STREAM_ACTION_ALLOW_CONNECTION;
    pkt->countBytesEnforced = 0;
    pkt->countBytesRequired = 0;

    classifyOut->actionType = FWP_ACTION_CONTINUE;

}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Datagram.                                                                 **
 **                                                                           **
 *******************************************************************************
 *******************************************************************************/

NTSTATUS DatagramNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                        _In_ const GUID* filterKey,
                        _Inout_ const FWPS_FILTER* filter)
{
	UNREFERENCED_PARAMETER(notifyType);
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);

#if DEBUG
	DbgPrint("DatagramNotify() value:%d", notifyType);
#endif

	return STATUS_SUCCESS;
}



void DatagramClassify(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
                      _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
                      _Inout_opt_ void* layerData,
#if(NTDDI_VERSION >= NTDDI_WIN7)
                      _In_opt_ const void* classifyContext,
#endif
                      _In_ const FWPS_FILTER* filter,
                      _In_ UINT64 flowContext,
                      _Inout_ FWPS_CLASSIFY_OUT* classifyOut)
{
    packet_t* packet;

    #if(NTDDI_VERSION >= NTDDI_WIN7)
    UNREFERENCED_PARAMETER(classifyContext);
    #endif

    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    #if DEBUG
    DbgPrint("DatagramClassify()");
    #endif

    int result = 0;

    if (!(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE))
    {
	    return;
    }

    classifyOut->actionType = FWP_ACTION_PERMIT;

    result = processMyPacket(inFixedValues, layerData);
    if (result)
    {
	    classifyOut->actionType = FWP_ACTION_BLOCK;
    }


    if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
    {
	    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }

    return;

    /* ipHeaderSize is not applicable to the outbound path at the
    * FWPS_LAYER_DATAGRAM_DATA_V4/FWPS_LAYER_DATAGRAM_DATA_V6
    * layers.
    */
    if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_IP_HEADER_SIZE)) {
        /* Get packet from the packet pool. */
        if ((packet = PopPacket()) != NULL) {
            if (FillPacket(inFixedValues, layerData, packet)) {
                if (!GivePacketToWorkerThread(packet)) {
                    /* Return packet to packet pool. */
                    PushPacket(packet);
                }
            } else {
                /* Return packet to packet pool. */
                PushPacket(packet);
            }
        }
    }

    classifyOut->actionType = FWP_ACTION_CONTINUE;
}


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 ** Closure.                                                                  **
 **                                                                           **
 *******************************************************************************
 *******************************************************************************/

NTSTATUS AleClosureNotify(_In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
                          _In_ const GUID* filterKey,
                          _Inout_ const FWPS_FILTER* filter)
{
	UNREFERENCED_PARAMETER(notifyType);
	UNREFERENCED_PARAMETER(filterKey);
	UNREFERENCED_PARAMETER(filter);

#if DEBUG
	DbgPrint("AleClosureNotify() value:%d", notifyType);
#endif

	return STATUS_SUCCESS;
}

void AleClosureClassify(_In_ const FWPS_INCOMING_VALUES* inFixedValues,
                        _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
                        _Inout_opt_ void* layerData,
#if (NTDDI_VERSION >= NTDDI_WIN7)
                        _In_opt_ const void* classifyContext,
#endif
                        _In_ const FWPS_FILTER* filter,
                        _In_ UINT64 flowContext,
                        _Inout_ FWPS_CLASSIFY_OUT* classifyOut)
{
    return;

    packet_t* packet;

    UNREFERENCED_PARAMETER(inMetaValues);

    #if(NTDDI_VERSION >= NTDDI_WIN7)
    UNREFERENCED_PARAMETER(classifyContext);
    #endif

    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

    #if DEBUG
    DbgPrint("AleClosureClassify()");
    #endif

    if (!(classifyOut->rights & FWPS_RIGHT_ACTION_WRITE))
    {
	    return;
    }

    classifyOut->actionType = FWP_ACTION_PERMIT;

    int result = processMyPacket(inFixedValues, layerData);
    if (result)
    {
	    classifyOut->actionType = FWP_ACTION_BLOCK;
    }

    if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
    {
	    classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
    }
    return;

    /* Get packet from the packet pool. */
    if ((packet = PopPacket()) != NULL) {
        if (FillPacket(inFixedValues, layerData, packet)) {
            if (!GivePacketToWorkerThread(packet)) {
                /* Return packet to packet pool. */
                PushPacket(packet);
            }
        } else {
            /* Return packet to packet pool. */
            PushPacket(packet);
        }
    }

    classifyOut->actionType = FWP_ACTION_CONTINUE;
}
