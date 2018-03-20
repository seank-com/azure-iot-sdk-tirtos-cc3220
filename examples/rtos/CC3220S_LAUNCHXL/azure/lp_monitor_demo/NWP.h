/*
 * Copyright (c) 2017 Texas Instruments Incorporated - http://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 *   Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== NWP.h ========
 */
#ifndef NWP_H_
#define NWP_H_

#include <ti/drivers/net/wifi/simplelink.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== NWP_initialize ========
 */
extern int NWP_initialize(void);

/*
 *  ======== NWP_macAddr ========
 */
extern _u8 *NWP_macAddr(void);

/*
 *  ======== NWP_shutdown ========
 */
extern int NWP_shutdown(void);

/*
 *  ======== NWP_startup ========
 */
extern int NWP_startup(void);

/*
 *  ======== NWP_wlanConnect ========
 */
extern void NWP_wlanConnect(void);

/*
 *  ======== NWP_wlanDisconnect ========
 */
extern void NWP_wlanDisconnect(void);


#ifdef __cplusplus
}
#endif

#endif /* NWP_H_ */
