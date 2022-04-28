#ifndef UTILS_H
#define UTILS_H

__inline ADDRESS_FAMILY GetAddressFamilyForLayer(_In_ UINT16 layerId)
{
    switch (layerId)
    {
    case FWPS_LAYER_STREAM_V4:
    case FWPS_LAYER_DATAGRAM_DATA_V4:
    case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
        return AF_INET;
    case FWPS_LAYER_STREAM_V6:
    case FWPS_LAYER_DATAGRAM_DATA_V6:
    case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
        return AF_INET6;
    default:
        return AF_UNSPEC;
    }
}

__inline BOOL GetNetwork4TupleIndexesForLayer(_In_ UINT16 layerId, _Out_ UINT* localAddressIndex,  _Out_ UINT* remoteAddressIndex,
                                              _Out_ UINT* localPortIndex, _Out_ UINT* remotePortIndex, UINT* protocolIdx, UINT * directionIndex)
{
    switch (layerId)
    {
    case FWPS_LAYER_STREAM_V4:
        *localAddressIndex = FWPS_FIELD_STREAM_V4_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_STREAM_V4_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_STREAM_V4_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_STREAM_V4_IP_REMOTE_PORT;
        //*protocolIdx = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL;
        *protocolIdx = IPPROTO_TCP;
        *directionIndex = FWPS_FIELD_STREAM_V4_DIRECTION;

        return TRUE;
    case FWPS_LAYER_STREAM_V6:
        *localAddressIndex = FWPS_FIELD_STREAM_V6_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_STREAM_V6_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_STREAM_V6_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_STREAM_V6_IP_REMOTE_PORT;
        //*protocolIdx = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL;
        *protocolIdx = IPPROTO_TCP;
        *directionIndex = FWPS_FIELD_STREAM_V6_DIRECTION;
        return TRUE;
    case FWPS_LAYER_DATAGRAM_DATA_V4:
        *localAddressIndex = FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_PORT;
        //*protocolIdx = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL;
        *protocolIdx = IPPROTO_UDP;
        *directionIndex = FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION;
        return TRUE;
    case FWPS_LAYER_DATAGRAM_DATA_V6:
        *localAddressIndex = FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_PORT;
        //*protocolIdx = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL;
        *protocolIdx = IPPROTO_UDP;
        *directionIndex = FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION;
        return TRUE;
    case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4:
        *localAddressIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V4_IP_REMOTE_PORT;
        //*protocolIdx = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL;
        *protocolIdx = IPPROTO_TCP;
        *directionIndex = 0;
        return TRUE;
    case FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6:
        *localAddressIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V6_IP_LOCAL_ADDRESS;
        *remoteAddressIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V6_IP_REMOTE_ADDRESS;
        *localPortIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V6_IP_LOCAL_PORT;
        *remotePortIndex = FWPS_FIELD_ALE_ENDPOINT_CLOSURE_V6_IP_REMOTE_PORT;
        //*protocolIdx = FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL;
        *protocolIdx = IPPROTO_TCP;
        *directionIndex = 0;
        return TRUE;
    default:
        return FALSE;
    }
}

#endif /* UTILS_H */
