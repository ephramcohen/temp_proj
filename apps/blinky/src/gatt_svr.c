/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_temp_sens.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "nrf_temp.h"


/* temperature history buffer size.  (power of 2 please) */
#define TEMP_HIST_SIZE (16)

#define TEMP_SAMPLING_RATE (10)  /* in Hz */


/* 5c3a659e-897e-45e1-b016-007107c96df6 */
static const ble_uuid128_t gatt_svr_svc_temp_uuid =
        BLE_UUID128_INIT(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5d);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid128_t gatt_svr_chr_temp_uuid =
        BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5d);

static int
gatt_svr_chr_access_temp(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Temperature Sensor */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_temp_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: Temperature measurement */
            .uuid = &gatt_svr_chr_temp_uuid.u,
            .access_cb = gatt_svr_chr_access_temp,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            0, /* No more characteristics in this service */
        }, }
    },

        {
            0, /* No more services */
        },
};

/* Temperature sampling timer & data*/
static struct os_callout temp_read_timer;

volatile static int temp_queue_head;
volatile static int16_t temp_history[TEMP_HIST_SIZE];

/* Returns the internal temperature of the nRF52 in degC (2 decimal places, scaled) */
static int16_t
get_temp_measurement(void)
{
  int16_t temp;

  /* make sure that he last temperature read is complete */
  while(NRF_TEMP->EVENTS_DATARDY != TEMP_INTENSET_DATARDY_Set) {};

  /* Temp reading is in units of 0.25degC so divide by 4 to get in units of degC (scale by 100 to avoid representing as decimal) */
  temp = (nrf_temp_read() * 100) / 4.0;

  /* Start the next temperature measurement. */
  NRF_TEMP->TASKS_START = 1;

  return temp;
}

/* Reset temperature timer for 100 msec*/
static void
queue_temp_read(void)
{
    int rc;

    /* queue the next timer read */
    rc = os_callout_reset(&temp_read_timer, OS_TICKS_PER_SEC/TEMP_SAMPLING_RATE);
    assert(rc == 0);
}

/* This functions reads the temperature into a circular buffer */
static void
temp_read(struct os_event *ev)
{
    /* It is best to start of the next sample's timer as early as possible.  
     * due to the multitasking nature of the OS.  This reduces jitter in the temperature
     * reading time sampling rate.
     */
    queue_temp_read();

    /* save the current tempature reading */
    temp_history[temp_queue_head] = get_temp_measurement();

    /* inceremnt the head pointer, wrapping to the beginning of the buffer if necessary */
    temp_queue_head = (temp_queue_head+1) & (TEMP_HIST_SIZE-1);

}


/* init the temerature reading timer and start it ticking */
void
temp_reader_init(void)
{

   temp_queue_head = 0;

   /* Now setup a 100 msec timer to sample the temperature */ 
    os_callout_init(&temp_read_timer, os_eventq_dflt_get(),
                    temp_read, NULL);

    /* start the timer ticking */
    queue_temp_read();

    /* Start the temperature measurement. */
    NRF_TEMP->TASKS_START = 1;

}

static int
gatt_svr_chr_access_temp(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  int rc;
  int16_t temp;
  int i;
  int index = temp_queue_head;  /* record the head pointer so it does not change mid report */

  LOG(INFO,"Temperature read =");

  for (i=0; i<10; i++)
  {
    index = (index-1) & (TEMP_HIST_SIZE-1);
    temp = temp_history[index];

    LOG(INFO," %i",temp);

    rc = os_mbuf_append(ctxt->om, &temp, sizeof(temp));
    assert (rc == 0);
  }

  LOG(INFO,"\n");

  return rc;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        LOG(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        LOG(DEBUG, "registering characteristic %s with "
                           "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        LOG(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
gatt_svr_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

