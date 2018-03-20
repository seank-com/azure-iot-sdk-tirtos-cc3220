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
 *  ======== NetMgr.h ========
 */
#ifndef NETMGR_H_
#define NETMGR_H_

#include <stdbool.h>
#include <stdint.h>

#include "SensorData.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== NetMgr_done ========
 *  Check if the network manager is done with all tasks
 */
extern bool NetMgr_done(void);

/*
 *  ======== NetMgr_sendData ========
 *  Provide a data packet to the network manager for transmission
 */
extern void NetMgr_sendData(DataPacket *dp);

/*
 *  ======== NetMgr_error ========
 *  Check if network manager is in error state
 */
extern bool NetMgr_error(void);

/*
 *  ======== NetMgr_initialize ========
 *  Start the network manager
 */
extern int NetMgr_initialize(void);

/*
 *  ======== NetMgr_ready ========
 *  Network manager is ready to accept data
 */
extern bool NetMgr_ready(void);

/*
 *  ======== NetMgr_recallBuf ========
 *  Recall a full data buffer
 */
extern void NetMgr_recallBuf(DataPacket **dpp);

/*
 *  ======== NetMgr_reclaimBuf ========
 *  Reclaim an empty data buffer
 *
 *  Returns true if an empty buffer was returned
 */
extern bool NetMgr_reclaimBuf(DataPacket **dpp);

#ifdef __cplusplus
}
#endif

#endif /* NETMGR_H_ */
