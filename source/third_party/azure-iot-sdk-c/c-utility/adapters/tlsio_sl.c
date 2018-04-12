// Copyright (c) Texas Instruments. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <errno.h>
#include "ti/net/ssock.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/tlsio.h"
#include "azure_c_shared_utility/tlsio_sl.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/threadapi.h"

/* USER STEP: Flash the CA root certificate to this location */
#define SL_SSL_CA_CERT "/cert/ms.der"
#define SL_SSL_CERT "/cert/cert.der"
#define SL_SSL_KEY "/cert/key.der"

typedef enum TLSIO_STATE_ENUM_TAG
{
    TLSIO_STATE_NOT_OPEN,
    TLSIO_STATE_OPENING,
    TLSIO_STATE_OPEN,
    TLSIO_STATE_CLOSING,
    TLSIO_STATE_ERROR
} TLSIO_STATE_ENUM;

typedef struct TLS_IO_INSTANCE_TAG
{
    ON_BYTES_RECEIVED on_bytes_received;
    ON_IO_OPEN_COMPLETE on_io_open_complete;
    ON_IO_CLOSE_COMPLETE on_io_close_complete;
    ON_IO_ERROR on_io_error;
    void* on_bytes_received_context;
    void* on_io_open_complete_context;
    void* on_io_close_complete_context;
    void* on_io_error_context;
    TLSIO_STATE_ENUM tlsio_state;
    ON_SEND_COMPLETE on_send_complete;
    void* on_send_complete_callback_context;
    TLS_Handle tls_handle;
    Ssock_Handle ssock_handle;
    char* hostname;
    int port;
    int sock;
} TLS_IO_INSTANCE;

static const IO_INTERFACE_DESCRIPTION tlsio_sl_interface_description =
{
    tlsio_sl_retrieveoptions,
    tlsio_sl_create,
    tlsio_sl_destroy,
    tlsio_sl_open,
    tlsio_sl_close,
    tlsio_sl_send,
    tlsio_sl_dowork,
    tlsio_sl_setoption
};

static int getErrno(int ret)
{
    if (ret == -1)
    {
        return (errno);
    }
    else
    {
        return (ret);
    }
}

static int init_sockaddr(struct sockaddr *addr, int port, const char *hostname)
{
    struct hostent *dnsEntry;
    struct sockaddr_in taddr = {0};
    struct in_addr **addr_list;
    int ip;

    dnsEntry = gethostbyname(hostname);
    if (dnsEntry == NULL) {
        return (-1);
    }

    /* use the first IP address returned from DNS */
    addr_list = (struct in_addr **)dnsEntry->h_addr_list;
    ip = (*addr_list[0]).s_addr;

    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(port);
    taddr.sin_addr.s_addr = htonl(ip);
    *addr = *((struct sockaddr *)&taddr);

    return (0);
}

OPTIONHANDLER_HANDLE tlsio_sl_retrieveoptions(CONCRETE_IO_HANDLE handle)
{
    (void)handle;
    return NULL;
}

CONCRETE_IO_HANDLE tlsio_sl_create(void* io_create_parameters)
{
    TLSIO_CONFIG* tls_io_config = io_create_parameters;
    TLS_IO_INSTANCE* result;

    if (tls_io_config == NULL)
    {
        LogError("NULL tls_io_config");
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(TLS_IO_INSTANCE));
        if (result == NULL)
        {
            LogError("NULL TLS_IO_INSTANCE");
        }
        else
        {
            int ret;

            memset(result, 0, sizeof(TLS_IO_INSTANCE));
            mallocAndStrcpy_s(&result->hostname, tls_io_config->hostname);

            result->tls_handle = TLS_create(TLS_METHOD_CLIENT_TLSV1_2);
            if (!result->tls_handle)
            {
                LogError("NULL result->tls_handle");
                if (result->hostname != NULL)
                {
                    free(result->hostname);
                }
                free(result);
                return NULL;
            }

            ret = TLS_setCertFile(result->tls_handle, TLS_CERT_TYPE_CA,
                                  TLS_CERT_FORMAT_DER, SL_SSL_CA_CERT);
            if (ret < 0)
            {
                LogError("Error TLS setCertFile %d\n", ret);
                TLS_delete(result->tls_handle);
                if (result->hostname != NULL)
                {
                    free(result->hostname);
                }
                free(result);
                return NULL;
            }

            result->port = tls_io_config->port;

            result->on_bytes_received = NULL;
            result->on_bytes_received_context = NULL;

            result->on_io_open_complete = NULL;
            result->on_io_open_complete_context = NULL;

            result->on_io_close_complete = NULL;
            result->on_io_close_complete_context = NULL;

            result->on_io_error = NULL;
            result->on_io_error_context = NULL;

            result->on_send_complete = NULL;
            result->on_send_complete_callback_context = NULL;

            result->tlsio_state = TLSIO_STATE_NOT_OPEN;
        }
    }

    return result;
}

void tlsio_sl_destroy(CONCRETE_IO_HANDLE tls_io)
{
    TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

    if (tls_io == NULL)
    {
        LogError("NULL tls_io");
    }
    else
    {
        if ((tls_io_instance->tlsio_state == TLSIO_STATE_OPENING) ||
            (tls_io_instance->tlsio_state == TLSIO_STATE_OPEN) ||
            (tls_io_instance->tlsio_state == TLSIO_STATE_CLOSING))
        {
            LogError("TLS destroyed with a SSL connection still active.");
        }
        if (tls_io_instance->hostname != NULL)
        {
            free(tls_io_instance->hostname);
        }
        free(tls_io_instance);
    }
}

int tlsio_sl_open(CONCRETE_IO_HANDLE tls_io,
                     ON_IO_OPEN_COMPLETE on_io_open_complete,
                     void* on_io_open_complete_context,
                     ON_BYTES_RECEIVED on_bytes_received,
                     void* on_bytes_received_context,
                     ON_IO_ERROR on_io_error,
                     void* on_io_error_context)
{
    int result = 0;

    if (tls_io == NULL)
    {
        LogError("NULL tls_io");
        result = __FAILURE__;
    }
    else
    {
        int ret;
        TLS_IO_INSTANCE* instance = (TLS_IO_INSTANCE*)tls_io;

        if (instance->tlsio_state != TLSIO_STATE_NOT_OPEN)
        {
            LogError("IO should not be open: %d\n", instance->tlsio_state);
            result =  __FAILURE__;
        }
        else
        {
            instance->on_bytes_received = on_bytes_received;
            instance->on_bytes_received_context = on_bytes_received_context;

            instance->on_io_open_complete = on_io_open_complete;
            instance->on_io_open_complete_context = on_io_open_complete_context;

            instance->on_io_error = on_io_error;
            instance->on_io_error_context = on_io_error_context;

            instance->tlsio_state = TLSIO_STATE_OPENING;

            struct sockaddr sa;
            ret = init_sockaddr(&sa, instance->port, instance->hostname);
            if (ret != 0)
            {
                LogError("Cannot resolve hostname");
                instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
                result = __FAILURE__;
            }
            else
            {
                instance->sock = socket(sa.sa_family, SOCK_STREAM,
                                        SL_SEC_SOCKET);
                if (socket >= 0)
                {
                    instance->ssock_handle = Ssock_create(instance->sock);
                    if (instance->ssock_handle == NULL)
                    {
                        LogError("Cannot create ssock");
                        instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
                        result = __FAILURE__;
                        close(instance->sock);
                    }
                    else
                    {
                        ret = Ssock_startTLS(instance->ssock_handle,
                                             instance->tls_handle);
                        if (ret < 0)
                        {
                            LogError("Cannot start ssock TLS");
                            instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
                            result = __FAILURE__;
                            Ssock_delete(&instance->ssock_handle);
                            close(instance->sock);
                        }
                        else
                        {
                            ret = connect(instance->sock, &sa,
                                          sizeof(struct sockaddr_in));
                            /*
                             *  SL returns unknown root CA error code if the CA certificate
                             *  is not found in its certificate store. This is a warning
                             *  code and not an error. So this error code is being ignored
                             *  till a better alternative is found.
                             */
                            if ((ret < 0) && (getErrno(ret) != SL_ERROR_BSD_ESECUNKNOWNROOTCA))
                            {
                                LogError("Cannot connect");
                                instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
                                result = __FAILURE__;
                                Ssock_delete(&instance->ssock_handle);
                                close(instance->sock);

                            }
                            /* setup for nonblocking */
                            SlSockNonblocking_t blocking;
                            blocking.NonBlockingEnabled = 1;
                            setsockopt(instance->sock, SOL_SOCKET,
                                    SO_NONBLOCKING, &blocking,
                                    sizeof(blocking));
                        }
                    }
                }
                else
                {
                    LogError("Cannot open socket");
                    instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
                    result = __FAILURE__;
                }
            }

            IO_OPEN_RESULT oresult = result == __FAILURE__ ? IO_OPEN_ERROR :
                                                             IO_OPEN_OK;
            instance->tlsio_state = TLSIO_STATE_OPEN;
            instance->on_io_open_complete(instance->on_io_open_complete_context,
                                          oresult);
            if (oresult == IO_OPEN_ERROR)
            {
                if (on_io_error != NULL)
                {
                    (void)on_io_error(on_io_error_context);
                }
            }
        }
    }

    return result;
}

int tlsio_sl_close(CONCRETE_IO_HANDLE tls_io,
                        ON_IO_CLOSE_COMPLETE on_io_close_complete,
                        void* callback_context)
{
    int result = 0;

    if (tls_io == NULL)
    {
        result = __FAILURE__;
    }
    else
    {
        TLS_IO_INSTANCE* instance = (TLS_IO_INSTANCE*)tls_io;

        if ((instance->tlsio_state == TLSIO_STATE_NOT_OPEN) ||
            (instance->tlsio_state == TLSIO_STATE_CLOSING))
        {
            result = __FAILURE__;
        }
        else
        {
            instance->tlsio_state = TLSIO_STATE_CLOSING;
            instance->on_io_close_complete = on_io_close_complete;
            instance->on_io_close_complete_context = callback_context;

            Ssock_delete(&instance->ssock_handle);
            close(instance->sock);

            instance->tlsio_state = TLSIO_STATE_NOT_OPEN;
            instance->on_io_close_complete(
                                       instance->on_io_close_complete_context);
        }
    }

    return result;
}

int tlsio_sl_send(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size,
                     ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    int result;

    if (tls_io == NULL)
    {
        result = __FAILURE__;
    }
    else
    {
        TLS_IO_INSTANCE* instance = (TLS_IO_INSTANCE*)tls_io;

        if (instance->tlsio_state != TLSIO_STATE_OPEN)
        {
            result = __FAILURE__;
        }
        else
        {
            const char* buf = (const char*)buffer;
            instance->on_send_complete = on_send_complete;
            instance->on_send_complete_callback_context = callback_context;

            result = 0;
            while (size)
            {
                int res = send(instance->sock, buf, size, 0);
                if (res < 0)
                {
                    result = __FAILURE__;
                    break;
                }
                else if (size < res)
                {
                    /* no more space left, lets wait for more to become
                     * available and try again.
                     */
                    ThreadAPI_Sleep(10);
                }
                size -= res;
                buf += res;
            }
            IO_SEND_RESULT oresult = result == __FAILURE__ ? IO_SEND_ERROR :
                                                             IO_SEND_OK;
            instance->on_send_complete(
                          instance->on_send_complete_callback_context, oresult);
        }
    }

    return result;
}

void tlsio_sl_dowork(CONCRETE_IO_HANDLE tls_io)
{
    if (tls_io != NULL)
    {
        TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

        if ((tls_io_instance->tlsio_state != TLSIO_STATE_NOT_OPEN) &&
            (tls_io_instance->tlsio_state != TLSIO_STATE_ERROR))
        {
            unsigned char buffer[64];
            int rcv_bytes = 1;

            while (rcv_bytes > 0)
            {
                rcv_bytes = Ssock_recv(tls_io_instance->ssock_handle, buffer, sizeof(buffer), 0);
                if (rcv_bytes > 0)
                {
                    if (tls_io_instance->on_bytes_received != NULL)
                    {
                        tls_io_instance->on_bytes_received(
                                     tls_io_instance->on_bytes_received_context,
                                     buffer, rcv_bytes);
                    }
                }
            }
        }
    }
}

const IO_INTERFACE_DESCRIPTION* tlsio_sl_get_interface_description(void)
{
    return &tlsio_sl_interface_description;
}

int tlsio_sl_setoption(CONCRETE_IO_HANDLE tls_io, const char* optionName, const void* value)
{
    int result = 0;
    TLS_IO_INSTANCE* inst = (TLS_IO_INSTANCE*)tls_io;

    if (strcmp("x509EccCertificate", optionName) == 0)
    {
        result = TLS_setCertFile(inst->tls_handle, TLS_CERT_TYPE_CERT, TLS_CERT_FORMAT_DER, value);
    }
    else if (strcmp("x509EccAliasKey", optionName) == 0)
    {
        result = TLS_setCertFile(inst->tls_handle, TLS_CERT_TYPE_KEY, TLS_CERT_FORMAT_DER, value);
    }
    return result;
}
