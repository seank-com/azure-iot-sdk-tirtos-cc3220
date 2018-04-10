// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <string.h>

#include "hsm_client_data.h"

#include <ti/drivers/net/wifi/fs.h>
#include <ti/net/certconv.h>

#define CERTIFICATE_PATH "/cert/cert.pem"
#define PRIVATE_KEY_PATH "/cert/key.pem"

//
// For details on TLV (Type, Length, Value) in the ASN1 DER format see
// https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
//
// For details about x509 certificates see https://tools.ietf.org/html/rfc5280
//

#define CLASS_UNIVERSAL        0
#define CLASS_CONTEXT_SPECIFIC 2

#define ENCODING_PRIMITIVE    0
#define ENCODING_CONSTRUCTED  1

#define TYPE_BOOLEAN           0
#define TYPE_INTEGER           2
#define TYPE_BIT_STRING        3
#define TYPE_OBJECT_ID         6
#define TYPE_SEQUENCE         16
#define TYPE_SET              17
#define TYPE_UTF8_STRING      12
#define TYPE_PRINTABLE_STRING 19

// parseTLV takes a pointer to a TLV structure and returns the class, encoding,
// type, length, and value. It does not support types and lengths greater than
// 32 bits and also does not support indefinite forms. Normal x509 should not
// need any of that.
//
// p   - pointer to the beginning of a TLV structure
// pc  - receives the class
// pe  - receives the encoding
// pt  - receives the type
// pl  - receives the length
// ppv - receives a pointer to the value
//
// Returns a pointer to the next sibling TLV structure on success,
// otherwise NULL
//
static uint8_t* parseTLV(uint8_t* p, uint8_t* pc, uint8_t* pe, uint32_t* pt, uint32_t* pl, uint8_t** ppv) {
  int i = 0, c = 0;
  *pc = ((*p) & 0xC0) >> 6; *pe = ((*p) & 0x20) >> 5;
  *pt = 0; *pl = 0; *ppv = NULL;

  if (((*p) & 0x1F) == 0x1F) {
    // long form type
    do { c++; p++; *pt = ((*pt) << 7) | ((*p) & 0x7F);
    } while (((*p) & 0x80) == 0x80);
    if (c > 4) return NULL;
  } else {
    // short form type
    *pt = ((*p) & 0x1F);
  }
  p++; // advance to length

  if (((*p) & 0x80) == 0x80) {
    // long or indefinite form length
    c = ((*p) & 0x7F);
    if (c > 4 || c < 1) return NULL;
    for (i = 0; i < c; i++) {
      p++; *pl = ((*pl) << 8) | (*p);
    }
  } else {
    // short form length
    *pl = ((*p) & 0x7F);
  }
  // advance to value, set value, advance to next
  p++; *ppv = p; p += *pl;
  return p;
}

#define TLV(s, p, pn, expect) \
  pn = parseTLV(p, &c, &e, &t, &l, &pv); \
  if (pn==NULL || !(expect)) \
    return -1; \

#define EXPECT(class, encoding, type) (c==class && e==encoding && t==type)

// findCommonName takes a pointer to the beginning of an x509 certificate in
// ASN1 DER format and walks the TLV structures to find the common name
//
// Returns a pointer to a static buffer containing the common name on success,
// otherwise NULL
//
static int findCommonName(uint8_t* p, char* pr, uint32_t rl) {
  uint8_t *pn, c, e, *pv; uint32_t t, l;
  // Certificate  ::=  SEQUENCE  {
  //      tbsCertificate       TBSCertificate,
  //      signatureAlgorithm   AlgorithmIdentifier,
  //      signatureValue       BIT STRING  }
  TLV("Certificate", p, pn, \
    EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
    // TBSCertificate  ::=  SEQUENCE  {
    //      version         [0]  EXPLICIT Version DEFAULT v1,
    //      serialNumber         CertificateSerialNumber,
    //      signature            AlgorithmIdentifier,
    //      issuer               Name,
    //      validity             Validity,
    //      subject              Name,
    TLV("TBSCertificate", pv, pn,
      EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
      TLV("version", pv, pn,
        EXPECT(CLASS_CONTEXT_SPECIFIC, ENCODING_CONSTRUCTED, TYPE_BOOLEAN));
      TLV("serialNumber", pn, pn,
        EXPECT(CLASS_UNIVERSAL, ENCODING_PRIMITIVE, TYPE_INTEGER));
      TLV("signature", pn, pn,
        EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
      TLV("issuer", pn, pn,
        EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
      TLV("validity", pn, pn,
        EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
      TLV("subject", pn, pn,
        EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
        // Name ::= CHOICE { -- only one possibility for now --
        //   rdnSequence  RDNSequence }
        //
        // RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
        //
        // RelativeDistinguishedName ::=
        //  SET SIZE (1..MAX) OF AttributeTypeAndValue

        // subject contains a sequence or ordered sets of oid value pairs

        // save the next sibling so we know where to stop and
        // update pn so the loop below can process both the first
        // inner child and all subsequent siblings
        uint8_t* pe = pn; pn = pv;
        while(pn < pe) {
          TLV("rdnSequence", pn, pn,
            EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SET));
            // AttributeTypeAndValue ::= SEQUENCE {
            //  type     AttributeType,
            //  value    AttributeValue }
            //
            // AttributeType ::= OBJECT IDENTIFIER
            //
            // AttributeValue ::= ANY -- DEFINED BY AttributeType
            //
            // DirectoryString ::= CHOICE {
            //       teletexString           TeletexString (SIZE (1..MAX)),
            //       printableString         PrintableString (SIZE (1..MAX)),
            //       universalString         UniversalString (SIZE (1..MAX)),
            //       utf8String              UTF8String (SIZE (1..MAX)),
            //       bmpString               BMPString (SIZE (1..MAX)) }
            uint8_t* pattr;
            TLV("AttributeTypeAndValue", pv, pattr,
              EXPECT(CLASS_UNIVERSAL, ENCODING_CONSTRUCTED, TYPE_SEQUENCE));
              uint8_t* ptv;
              TLV("type", pv, ptv,
                EXPECT(CLASS_UNIVERSAL, ENCODING_PRIMITIVE, TYPE_OBJECT_ID));
              // common name is oid 1.21.4.3
              int cn = (l==3 && pv[0]==85 && pv[1]==4 && pv[2]==3) ? 1 : 0;
              TLV("value", ptv, ptv,
                EXPECT(CLASS_UNIVERSAL, ENCODING_PRIMITIVE, t));
              if (t == TYPE_UTF8_STRING || t == TYPE_PRINTABLE_STRING) {
                if (cn && l < rl) {
                  strncpy((char*)pr, (char*)pv, l);
                  pr[l+1] = '\0';
                  return 0;
                }
              }
            // END AttributeTypeAndValue
          // END rdnSequence
        }
      // END subject
    // END TBSCertificate
  // END Certificate
  return -1;
}

static int loadCommonName(const char *path, char *name, uint32_t len)
{
    SlFsFileInfo_t fsFileInfo;
    int32_t ret = (name != NULL && len > 0) ? 0 : -1;

    if (ret == 0) {
      name[0] = '\0';
      ret = sl_FsGetInfo((uint8_t *)path, 0, &fsFileInfo);
    }

    int32_t f = 0;
    if (ret == 0) {
      f = sl_FsOpen((unsigned char *)path, SL_FS_READ, NULL);
      ret = (f > 0) ? 0 : -1;
    }

    uint8_t *pemPtr = NULL;
    uint32_t pemLen;
    if (ret == 0) {
        pemLen = fsFileInfo.Len;
        pemPtr = (uint8_t *)calloc(pemLen, sizeof(*pemPtr));
        ret = (pemPtr == NULL) ? -1 : 0;
    }

    if (ret == 0) {
      ret = sl_FsRead(f, 0, pemPtr, pemLen);
      ret = (ret < 0) ? ret : 0;
    }

    if (f > 0) {
      sl_FsClose(f, NULL, NULL, 0);
      f = 0;
    }

    uint8_t *derPtr = NULL;
    uint32_t derLen;
    if (ret == 0) {
      ret = CertConv_pem2der(pemPtr, pemLen, &derPtr, &derLen);
    }

    if (pemPtr != NULL) {
      free(pemPtr);
      pemPtr = NULL;
    }

    if (ret == 0) {
      ret = findCommonName(derPtr, name, len);
    }

    if (derPtr != NULL) {
      CertConv_free(&derPtr);
    }

    return (ret);
}


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
        cust_hsm->info += 1;

        SlFsFileInfo_t fsFileInfo;
        const char* cert = CERTIFICATE_PATH;
        int32_t ret = sl_FsGetInfo((uint8_t *)cert, 0, &fsFileInfo);
        if (ret != 0)
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
        cust_hsm->info += 1;

        SlFsFileInfo_t fsFileInfo;
        const char* private_key = PRIVATE_KEY_PATH;
        int32_t ret = sl_FsGetInfo((uint8_t *)private_key, 0, &fsFileInfo);
        if (ret != 0)
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
    static char common_name[129] = { 0 };

    char* result;
    if (handle == NULL)
    {
        //TODO: TI should log something here (void)printf("Invalid handle value specified");
        result = NULL;
    }
    else
    {
        CUSTOM_HSM_INFO* cust_hsm = (CUSTOM_HSM_INFO*)handle;
        cust_hsm->info += 1;

        int ret = loadCommonName(CERTIFICATE_PATH, common_name, 129);
        if (ret != 0)
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
