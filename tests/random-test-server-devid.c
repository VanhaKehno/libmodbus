/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>

#include <modbus.h>
#include <string.h>

typedef struct
{
    const char* tag;
    uint8_t id;
    const char* data;
} device_id_init_t;

const device_id_init_t devid_obj[] =
{
    { "VendorName",        0,    "VendorName:A" },
    { "ProductCode",       1,    "ProductCode:B" },
    { "VendorUrl",         3,    "VendorUrl:D" },
    { "MajorMinorVersion", 2,    "MajorMinorVersion:C" },
    { "ProductName",       4,    "ProductName:E" },
    { "ModelName",         5,    "ModelName:F" },
    { "Short String",      0x80, "L0:GoofBallers" },
    { "Long String",       0x81, "L1:ABCDEFGHIJKLMNOPQRTUVWXYZabcdefghijklmnopqrstuvwyxz1234567890abcdefghlijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
    { "Long String2",      0x82, "L2:ABCDEFGHIJKLMNOPQRTUVWXYZabcdefghijklmnopqrstuvwyxz1234567890abcdefghlijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" }
};

static const int n_objects = sizeof(devid_obj)/sizeof(*devid_obj);

static const char* findTag(uint8_t id, const device_id_init_t* obj_list)
{
    for(uint8_t i = 0; i < n_objects; i++)
    {
        if(obj_list[i].id == id)
        {
            return obj_list[i].tag;
        }
    }
    return NULL;
}

int main(void)
{
    printf("Starting...\n");
    int s = -1;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;

    ctx = modbus_new_tcp("127.0.0.1", 1502);
    /* modbus_set_debug(ctx, TRUE); */

    mb_mapping = modbus_mapping_new(500, 500, 500, 500);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    modbus_device_id_t* obj;
    for(uint8_t i = 0; i < n_objects; i++)
    {
        obj = modbus_device_id_set_object(mb_mapping, devid_obj[i].id, (uint8_t*)devid_obj[i].data, strlen(devid_obj[i].data) + 1);

        if(obj == NULL)
        {
            fprintf(stderr, "Device Id object creation failed");
            modbus_mapping_free(mb_mapping);
            modbus_free(ctx);
            return EXIT_FAILURE;
        }
    }

    printf("Device ID information:\n");
    modbus_device_id_t* root = mb_mapping->tab_device_identification;
    modbus_device_id_t* current = root;
    for(; current != NULL; current = modbus_device_id_get_next(current))
    {
        printf("%-15s  : %s\n", modbus_device_id_get_data(current), findTag(modbus_device_id_get_id(current), devid_obj));
    }
    printf("no more objects\n");

    s = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &s);

    for (;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        int rc;

        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            /* rc is the query size */
            modbus_reply(ctx, query, rc, mb_mapping);
        } else if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    if (s != -1) {
        close(s);
    }

    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
