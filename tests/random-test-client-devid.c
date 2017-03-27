/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <modbus.h>
#include "modbus-device-id.h"

/* The goal of this program is to check all read device identification functio of
   libmodbus
*/
#define LOOP             1
#define SERVER_ID       17

typedef struct
{
    const char* tag;
    uint8_t id;
} device_id_init_t;

const device_id_init_t devid_obj[] =
{
    { "VendorName",        0,    },
    { "ProductCode",       1,    },
    { "MajorMinorVersion", 2,    },
    { "VendorUrl",         3,    },
    { "ProductName",       4,    },
    { "ModelName",         5,    },
    { "Extended Info",     0x80, },
    { "Long String",       0x81, },
    { "Long String2",      0x82, },
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

static void get_id(modbus_device_id_t** root, modbus_t *ctx, uint8_t code, uint8_t start)
{
    uint8_t segmented = 0;
    uint8_t start_id = start;
    uint8_t next_id = 0;
    while(1)
    {
        modbus_device_id(ctx, root, code, start_id, &segmented, &next_id);
        if(segmented)
        {
            start_id = next_id;
        }
        else
        {
            break;
        }
    }
}

static void print_device_id(const modbus_device_id_t *root)
{
    const modbus_device_id_t *current = root;

    printf("Device Identification Object\n");
    while(current)
    {
        printf("Object id: %.2x ", modbus_device_id_get_id(current));
        printf("%-25s : %s\n", findTag(modbus_device_id_get_id(current), devid_obj), modbus_device_id_get_data(current));
        current = modbus_device_id_get_next(current);
    }
}

/* At each loop, the program works in the range ADDRESS_START to
 * ADDRESS_END then ADDRESS_START + 1 to ADDRESS_END and so on.
 */
int main(void)
{
    modbus_t *ctx;
    int nb_fail;
    int nb_loop;
    int nb;
    uint8_t *tab_rq_bits;
    uint8_t *tab_rp_bits;
    uint16_t *tab_rq_registers;
    uint16_t *tab_rw_rq_registers;
    uint16_t *tab_rp_registers;

    /* RTU */
/*
    ctx = modbus_new_rtu("/dev/ttyUSB0", 19200, 'N', 8, 1);
    modbus_set_slave(ctx, SERVER_ID);
*/

    /* TCP */
    ctx = modbus_new_tcp("127.0.0.1", 1502);
    modbus_set_debug(ctx, TRUE);

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    /* Allocate and initialize the different memory spaces */
    nb = 1;

    tab_rq_bits = (uint8_t *) malloc(nb * sizeof(uint8_t));
    memset(tab_rq_bits, 0, nb * sizeof(uint8_t));

    tab_rp_bits = (uint8_t *) malloc(nb * sizeof(uint8_t));
    memset(tab_rp_bits, 0, nb * sizeof(uint8_t));

    tab_rq_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
    memset(tab_rq_registers, 0, nb * sizeof(uint16_t));

    tab_rp_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
    memset(tab_rp_registers, 0, nb * sizeof(uint16_t));

    tab_rw_rq_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
    memset(tab_rw_rq_registers, 0, nb * sizeof(uint16_t));

    nb_loop = nb_fail = 0;
    modbus_device_id_t *root = NULL;
    while (nb_loop++ < LOOP)
    {
        printf("Getting device id(s)\n");
        printf("Get basic (mandatory) set\n");
        get_id(&root, ctx, 1, 0);
        print_device_id(root);
        modbus_device_id_delete(&root);

        printf("Get extended set\n");
        get_id(&root, ctx, 3, 0x80);
        print_device_id(root);
        modbus_device_id_delete(&root);

        printf("Get regular set\n");
        get_id(&root, ctx, 2, 0);
        print_device_id(root);
        modbus_device_id_delete(&root);

        printf("Get regular set starting from 0x05\n");
        get_id(&root, ctx, 2, 0x05);
        print_device_id(root);
        modbus_device_id_delete(&root);

        printf("Test: ");
        if (nb_fail)
            printf("%d FAILS\n", nb_fail);
        else
            printf("SUCCESS\n");
    }

    /* Free the memory */
    free(tab_rq_bits);
    free(tab_rp_bits);
    free(tab_rq_registers);
    free(tab_rp_registers);
    free(tab_rw_rq_registers);

    /* Close the connection */
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
