// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>

#include "hsm_client_data.h"

typedef struct CUSTOM_HSM_INFO_TAG
{
    int info;
} CUSTOM_HSM_INFO;

HSM_CLIENT_HANDLE custom_hsm_create()
{
    CUSTOM_HSM_INFO* result;
    result = malloc(sizeof(CUSTOM_HSM_INFO));
    if (result == NULL)
    {
        //TODO: TI should log something here (void)printf("Failure: malloc CUSTOM_HSM_INFO.");
        result = 0;
    }
    else
    {
        memset(result, 0, sizeof(CUSTOM_HSM_INFO));
    }
    return (HSM_CLIENT_HANDLE)result;
}

void custom_hsm_destroy(HSM_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        CUSTOM_HSM_INFO* hsm_impl = (CUSTOM_HSM_INFO*)handle;
        free(hsm_impl);
    }
}

int hsm_client_x509_init()
{
    // Add any code needed to initialize the x509 module
    return 0;
}

void hsm_client_x509_deinit()
{
}

int hsm_client_tpm_init()
{
    // Add any code needed to initialize the TPM module
    return 0;
}

void hsm_client_tpm_deinit()
{
}

// Return the X509 certificate in PEM format
char* custom_hsm_get_certificate(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        //TODO: Ideally TI would load the this from secure storage
        const char* cert =
            "-----BEGIN CERTIFICATE-----\r\n"
            "MIICqjCCAZICCQDFPrKIOKANOjANBgkqhkiG9w0BAQsFADAXMRUwEwYDVQQDDAxr\r\n"
            "cmlzaG5hLXRlc3QwHhcNMTgwMzIzMTgyMTUyWhcNMTkwMzIzMTgyMTUyWjAXMRUw\r\n"
            "EwYDVQQDDAxrcmlzaG5hLXRlc3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\r\n"
            "AoIBAQC0/yPW9Eyg2UbZJsskhucIW3viU797dTI3Bf/2+/MGli/5QpzcyEeWzglR\r\n"
            "PiGMrt+3vW08JHsUjzSNU2udbzLpgpyOohTN2V8WBlVZpJu2ypbkYr/E4/rHOEDd\r\n"
            "eYApSov6YHLnruyIyOFd17YxOwfMO19y3JQwFuvEbrK2Nmkkhn+1QrbBUy8env7u\r\n"
            "p7eWhDEoty52Ppong47yKxDgLWiKsaTNThCyxr34Tk2nUXI8V+D39cDbvWxwtbwO\r\n"
            "cERsnitvl3d27EdO0914V3eyLSEHa98G8hgCMOU5QF6Nn46blymwJ1HAfl1RjN0/\r\n"
            "lB9jb88S4+9XQE9yT62+Wc7lVq53AgMBAAEwDQYJKoZIhvcNAQELBQADggEBALAo\r\n"
            "V/UM/NLRc2/W4MUioPqfJNWNT2QG8fX2yKgCpy2z8/TMQyGR/poYMzO4rP1ZbzQy\r\n"
            "76d2TLt4TKr2ydZUzaQiWZol8qoCKI29qkn5UC7RN5N9ioKx/tD88x7c1IlfmNVk\r\n"
            "GhkYI3xKMxVR77DFCR/Si3P2VBn/yF7kUwJVlPzjSRy6KmvlwlJiKOExWXq4mCC9\r\n"
            "DaUfl4olZS43uN5i/qA3s/cyMytpAX0hIq8fYslJg7o6kneprh6LaARBMqRtKx+x\r\n"
            "p3MHINj04U9qQP5hmvidnY3w42MTZ+oV7htDRsce3W2fXcpJMwVxjFMGq2rRzsiQ\r\n"
            "y1iT/i/n015ZfU8gKOY=\r\n"
            "-----END CERTIFICATE-----\r\n";

        if (cert == NULL)
        {
            //TODO: TI should log something here (void)printf("Failure retrieving cert");
            result = NULL;
        }
        else
        {
            size_t length = strlen(cert);
            result = malloc(length + 1);
            if (result == NULL)
            {
                //TODO: TI should log something here (void)printf("Failure allocating certifiicate");
            }
            else
            {
                strcpy(result, cert);
            }
        }
    }
    return result;
}

// Return Private Key of the Certification
char* custom_hsm_get_alias_key(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        //TODO: Ideally TI would load the this from secure storage
        const char* private_key =
          "-----BEGIN PRIVATE KEY-----\r\n"
          "MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC0/yPW9Eyg2UbZ\r\n"
          "JsskhucIW3viU797dTI3Bf/2+/MGli/5QpzcyEeWzglRPiGMrt+3vW08JHsUjzSN\r\n"
          "U2udbzLpgpyOohTN2V8WBlVZpJu2ypbkYr/E4/rHOEDdeYApSov6YHLnruyIyOFd\r\n"
          "17YxOwfMO19y3JQwFuvEbrK2Nmkkhn+1QrbBUy8env7up7eWhDEoty52Ppong47y\r\n"
          "KxDgLWiKsaTNThCyxr34Tk2nUXI8V+D39cDbvWxwtbwOcERsnitvl3d27EdO0914\r\n"
          "V3eyLSEHa98G8hgCMOU5QF6Nn46blymwJ1HAfl1RjN0/lB9jb88S4+9XQE9yT62+\r\n"
          "Wc7lVq53AgMBAAECggEASPIvUpnBLG6FRE2DP+RyxoaAZlYUbUBtjYmIgNVIPBZZ\r\n"
          "nV8Ac2bwm1HMpYah5N4x4g6hMMUPKdkReAfv7lJ7tWrjiATA17nMvcatrWRPMZty\r\n"
          "tvcpjMPJNXNxiRFH2txlj/JBPSjdwb8cPUML9clxuhkPve4ydzM1sERhGFjThVlV\r\n"
          "pP8Fl84QmS6rz5BvrbblgM6SQhnukbTXEo7X/ovoBsI18iyk6oZ0OS0CdhAWndr4\r\n"
          "VwbTedC+MmJY+oI7hlcj11kby9iLZ7YuoDrRHE+luMkAkGM/X8gY3+V7vUSvMpB6\r\n"
          "zsVR0oAyFjDAUbCJejxj7MJFzqHDJ+dzdN83uycAUQKBgQDonR2tyVaJqGUi1MO+\r\n"
          "8naurebAW1NgVK00JhEUwsXBxRvYLlyHOiVEISOeAMxen4Wpl3TEmfaougMmyaUr\r\n"
          "7/EAdK/pTacmvWqAIy5TwSHkE/K48O70BkNlOMXPuncLoXoxVEaPlckm+/CglhUC\r\n"
          "FkYzIEOszT+PgeM9z+SWkBjCbwKBgQDHMYghzUHfsK1SrtGD1Doj4Ih3A/TJG1my\r\n"
          "Hr9n6F7CFnHiBHuha0L6vZiyMuVzAPSLT7mcrQwrkvKOyTm42TsAci1bbEnL4oKc\r\n"
          "BwmM274n3cjnCNKxC0aOJJGq6kySYCBtOZht9nd8lGhZhWxvnBjZjMdLawVp7JRg\r\n"
          "EmjO5c24eQKBgFvGiYwkkMkVMHnymhx/S0YWBKHGJnouTnvxvPGE+0M9QoQjnowX\r\n"
          "69YagRP42qlGpRTJVd+vozrk0RN/oXRZYau9Xh5dbeKB/z/5IXEYFQgIus4u+Qg8\r\n"
          "ZGDOanVP62IiXrSRvJkwDsIbys+BB17gbOgFBc5q2HYFWCPuHxEsXyhvAoGAeyXH\r\n"
          "OMaSND4hWZ3U0AC0FRwqohHjEzYChRl3UkEZ3DpOG+KToF8U4Lm4nmrS6f+sMDiQ\r\n"
          "0yk0/fdyWA5Vzk8WqBburbfMA+28u8OqBtiPvkvieds9jtEexKAdIqKJxnEBeyWB\r\n"
          "dHJMustxm+7d9D54Kn9bcufuR+dIcADRpR/zyFkCgYBY8MgL5HUaZQ+Zm1oC2Tlt\r\n"
          "sgXjYe14Ny1YgfuuLMJcHuhXx+c+OQNc3oiyu+CSPrKxlRgyB8dcM2/fS0okvW+x\r\n"
          "3JYuh/alhXrWNDDO1eU0Q50Sg7ltsdvpbJ1mysQhmtq+aOq7oGbPgqPFtwICLKR5\r\n"
          "O/KghGqNjneM97yglTk8ZQ==\r\n"
          "-----END PRIVATE KEY-----\r\n";

        if (private_key == NULL)
        {
            //TODO: TI should log something here (void)printf("Failure retrieving private key");
            result = NULL;
        }
        else
        {
            size_t length = strlen(private_key);
            result = malloc(length + 1);
            if (result == NULL)
            {
                //TODO: TI should log something here (void)printf("Failure allocating private key");
            }
            else
            {
                strcpy(result, private_key);
            }
        }
    }
    return result;
}

// Return allocated common name on the x509 certificate
char* custom_hsm_get_common_name(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        //TODO: Ideally TI would load the this from secure storage
        const char* common_name = "krishna-test";
        if (common_name == NULL)
        {
            //TODO: TI should log something here (void)printf("Failure retrieving common name");
            result = NULL;
        }
        else
        {
            size_t length = strlen(common_name);
            result = malloc(length + 1);
            if (result == NULL)
            {
                //TODO: TI should log something here (void)printf("Failure allocating common name");
            }
            else
            {
                strcpy(result, common_name);
            }
        }
    }
    return result;
}

// TPM Custom Information handling
// Allocates the endorsement key using as key and the length as key_len
int custom_hsm_get_endorsement_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len)
{
    int result;
    if (handle == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified");
        result = __LINE__;
    }
    else
    {
      result = 12345678; //TI doesn't have a TPM
    }
    return result;
}

// Allocates the Storage Root key using as key and the length as key_len
int custom_hsm_get_storage_root_key(HSM_CLIENT_HANDLE handle, unsigned char** key, size_t* key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified");
        result = __LINE__;
    }
    else
    {
      result = 12345678; //TI doesn't have a TPM
    }
    return result;
}

// Decrypt and Stores the encrypted key
int custom_hsm_activate_id_key(HSM_CLIENT_HANDLE handle, const unsigned char* key, size_t key_len)
{
    int result;
    if (handle == NULL || key == NULL || key_len == 0)
    {
        //TODO: TI should log something here (void)printf("Invalid argument specified handle: %p, key: %p, key_len: %d", handle, key, key_len);
        result = __LINE__;
    }
    else
    {
      result = 12345678; //TI doesn't have a TPM
    }
    return result;
}

// Hashes value specified in data with the key stored in slot 1 and returns the result in signed_value
int custom_hsm_sign_with_identity(HSM_CLIENT_HANDLE handle, const unsigned char* data, size_t data_len, unsigned char** signed_value, size_t* signed_len)
{
    int result;
    if (handle == NULL || data == NULL || data_len == 0 || signed_value == NULL || signed_len == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified handle: %p, data: %p", handle, data);
        result = __LINE__;
    }
    else
    {
        result = 12345678; //TI doesn't have a TPM
    }
    return result;
}

static const HSM_CLIENT_X509_INTERFACE x509_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_get_certificate,
    custom_hsm_get_alias_key,
    custom_hsm_get_common_name
};

static const HSM_CLIENT_TPM_INTERFACE tpm_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_activate_id_key,
    custom_hsm_get_endorsement_key,
    custom_hsm_get_storage_root_key,
    custom_hsm_sign_with_identity
};

const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface()
{
    return &tpm_interface;
}

const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface()
{
    return &x509_interface;
}
