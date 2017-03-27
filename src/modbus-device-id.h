#ifndef _MSC_VER
# include <stdint.h>
#else
# include "stdint.h"
typedef int ssize_t;
#endif

#define MODBUS_MEI_DEVICE_IDENTIFICATION    0x0E

#define MODBUS_DEVICE_ID_CONFORMITY_LEVEL   0x83
#define MODBUS_DEVICE_ID_READ_BASIC         1
#define MODBUS_DEVICE_ID_READ_REGULAR       2
#define MODBUS_DEVICE_ID_READ_EXTENDED      3
#define MODBUS_DEVICE_ID_READ_SPECIFIC      4

/* Locations in response */
#define MODBUS_DEVICE_ID_FUNCTION_OFFSET    0
#define MODBUS_DEVICE_ID_MEI_TYPE_OFFSET    1
#define MODBUS_DEVICE_ID_READ_CODE_OFFSET   2
#define MODBUS_DEVICE_ID_CLEVEL_OFFSET      3
#define MODBUS_DEVICE_ID_MORE_OFFSET        4
#define MODBUS_DEVICE_ID_NEXT_OBJ_OFFSET    5
#define MODBUS_DEVICE_ID_N_OBJECTS_OFFSET   6
#define MODBUS_DEVICE_ID_FIRST_OBJ_OFFSET   7

typedef enum
{
    MODBUS_DEVID_VENDOR_NAME            = 0,
    MODBUS_DEVID_PRODUCT_CODE           = 0x01,
    MODBUS_DEVID_MAJOR_MINOR_REVISION   = 0x02,
    MODBUS_DEVID_VENDOR_URL             = 0x03,
    MODBUS_DEVID_PRODUCT_NAME           = 0x04,
    MODBUS_DEVID_MODEL_NAME             = 0x05,
    MODBUS_DEVID_USER_APPLICATION_NAME  = 0x06
} modbus_devid_object_id;

typedef enum
{
    MODBUS_READ_DEVID_BASIC             = 1,
    MODBUS_READ_DEVID_REGULAR           = 2,
    MODBUS_READ_DEVID_EXTENDED          = 3,
    MODBUS_READ_DEVID_OBJECT            = 4
} modbus_read_device_id_code;


typedef struct device_id_object
{
    uint8_t id;
    uint8_t *data;
    uint8_t length;
    struct device_id_object *next;
} device_id_object_t;

device_id_object_t* modbus_device_id_create_object(device_id_object_t **root, uint8_t id, 
                                                    const uint8_t *data, uint8_t length);
void                modbus_device_id_delete_objects(device_id_object_t **root);
const device_id_object_t* modbus_device_id_get_object(const device_id_object_t *root, uint8_t id);
uint8_t             modbus_device_id_create_response(const device_id_object_t *root, uint8_t read_if_code,
                                                    uint8_t object_id, uint8_t* rsp,
                                                    uint8_t* status);

uint8_t             modbus_get_mei_type(const uint8_t *pud);
uint8_t             modbus_get_read_device_id_code(const uint8_t *pdu);
uint8_t             modbus_get_object_id(const uint8_t *pdu);

uint8_t             validate_read_device_id_code(uint8_t code);
uint8_t             validate_object_id(const device_id_object_t *root, uint8_t id);

int                 device_id_list_count(const device_id_object_t *root);
int                 device_id_list_count_from_id(const device_id_object_t *root, uint8_t start_id, uint8_t stop_id);

/* Client side funcions */

/* Server side functions */
void                modbus_server_get_device_id(const uint8_t *rsp_pdu, device_id_object_t **root);
