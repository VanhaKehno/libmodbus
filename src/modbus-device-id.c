#include "modbus-device-id.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "modbus.h"

static device_id_object_t* create_empty_device_id_object(void);
static void delete_device_id_object(device_id_object_t *obj);
static uint8_t device_id_objects_in_category(modbus_read_device_id_code code, uint8_t start_id, const device_id_object_t *root);
static int device_id_in_category(modbus_read_device_id_code code, const device_id_object_t *obj);

uint8_t validate_object_id(const device_id_object_t *root, uint8_t id)
{
	const device_id_object_t *obj = modbus_device_id_get_object(root, id);
	return obj != NULL;
}

uint8_t validate_read_device_id_code(uint8_t code)
{
	return code <= MODBUS_READ_DEVID_OBJECT ? 1 : 0;
}

device_id_object_t* create_empty_device_id_object(void)
{
	device_id_object_t *obj = malloc(sizeof(device_id_object_t));
	if(obj)
	{
		obj->id = 0xFF;
		obj->length = 0;
		obj->data = NULL;
	}
	return obj;
}

void delete_device_id_object(device_id_object_t *obj)
{
	if(obj)
	{
		//printf("Deleting %.2x\n", obj->id);
		if(obj->data)
		{
			free(obj->data);
		}
		obj->data = NULL;
		obj->length = 0;
		obj->id = 0;
		obj->next = NULL;
		free(obj);
	}
}

device_id_object_t* modbus_device_id_create_object(device_id_object_t **root, uint8_t id, const uint8_t *data, uint8_t length)
{
	device_id_object_t *obj = create_empty_device_id_object();
	if(obj == NULL)
	{
		return NULL;
	}

	obj->data = (uint8_t*)malloc(length);
	if(obj->data == NULL)
	{
		delete_device_id_object(obj);
		return NULL;
	}
	memcpy(obj->data, data, length);

	obj->id = id;
	obj->length = length;
	obj->next = NULL;

	device_id_object_t *current = *root;
	if(*root == NULL) /* first item to be added */
	{
		*root = obj;
		return obj;
	}

	if(current->id > obj->id) /* root has to be replaced */
	{

		obj->next = *root;
		current = obj;
		return obj;
	}

	device_id_object_t *previous = NULL;
	while(current != NULL)
	{

		if(obj->id > current->id)
		{
			if(current->next == NULL) /* End of the list */
			{
				current->next = obj;
				break;
			}
			previous = current;
			current = current->next;

		}
		else if(obj->id < current->id) /* put between objecst */
		{
			previous->next = obj;
			obj->next = current;
			break;
		}
		else /* Same id wont be updated */
		{
			return NULL;
		}
	}
	#if defined(MODBUS_DEBUG_DEVICE_ID_OBJS)
	printf("|-->");
	for (current = *root; current != NULL; current = current->next)
	{
		printf("[%.2X]-->", current->id);
	}
	printf("|\n");
	#endif

	return obj;
}

int device_id_list_count(const device_id_object_t *root)
{
	int i = 0;
	if(root == NULL)
	{
		return 0;
	}

	const device_id_object_t *current = root;

	for(i = 0; current != NULL; current = current->next)
	{
		i++;
	}
	return i;
}

int device_id_list_count_from_id(const device_id_object_t *root, uint8_t start_id, uint8_t stop_id)
{
	int i = 0;
	if(root == NULL)
	{
		return 0;
	}

	const device_id_object_t *current = root;

	for(i = 0; current != NULL; current = current->next)
	{
		if(current->id >= start_id && current->id <= stop_id)
		{
			i++;
		}
	}
	return i;
}

void modbus_device_id_delete_objects(device_id_object_t **root)
{
	device_id_object_t *current = *root;
	device_id_object_t *next;

	while(current != NULL)
	{
		next = current->next;
		delete_device_id_object(current);
		//printf("Deleted: 0x%.8x\nDeleted Next: 0x%.8x\nNext: 0x%.8x\n", current, current->next, next);
		current = next;
	}
}

const device_id_object_t* modbus_device_id_get_object(const device_id_object_t *root, uint8_t id)
{
	const device_id_object_t *current = root;

	while(current != NULL)
	{
		if (current->id == id)
		{
			return current;
		}
		current = current->next;
	}

	return NULL;
}

int device_id_in_category(modbus_read_device_id_code code, const device_id_object_t *obj)
{
	switch(code)
	{
		case MODBUS_READ_DEVID_BASIC:
			return obj->id < 4 ? 0 : -1;
		case  MODBUS_READ_DEVID_REGULAR:
			return obj->id < 0x80 ? 0 : -1;
		case MODBUS_READ_DEVID_EXTENDED:
		case MODBUS_READ_DEVID_OBJECT:
			return 0;
		default:
			return -1;
	}
}

uint8_t device_id_objects_in_category(modbus_read_device_id_code code, uint8_t start_id, const device_id_object_t *root)
{
	const device_id_object_t *current = root;
	uint8_t n = 0;
	for(uint8_t i = 0; current != NULL; i++)
	{
		if(0 == device_id_in_category(code, current) && current->id >= start_id)
		{
			n++;
		}
		current = current->next;
	}

	return n;
}

uint8_t modbus_device_id_create_response(const device_id_object_t *root, uint8_t read_id_code, uint8_t object_id, uint8_t* rsp_pdu, uint8_t* status)
{
	modbus_read_device_id_code read_code = (modbus_read_device_id_code)read_id_code;
	uint16_t n_objects = device_id_objects_in_category(read_code, object_id, root);

	rsp_pdu[MODBUS_DEVICE_ID_MEI_TYPE_OFFSET - 1] = MODBUS_MEI_DEVICE_IDENTIFICATION;
	rsp_pdu[MODBUS_DEVICE_ID_READ_CODE_OFFSET - 1] = read_id_code;
	rsp_pdu[MODBUS_DEVICE_ID_CLEVEL_OFFSET - 1] = MODBUS_DEVICE_ID_CONFORMITY_LEVEL;
	rsp_pdu[MODBUS_DEVICE_ID_MORE_OFFSET - 1] = 0;
	rsp_pdu[MODBUS_DEVICE_ID_NEXT_OBJ_OFFSET - 1] = 0;

	/* Read object(s) */
	uint8_t *pData = &rsp_pdu[MODBUS_DEVICE_ID_FIRST_OBJ_OFFSET - 1];
	uint8_t n_objects_in_pdu = 0;
	uint16_t total_size = MODBUS_DEVICE_ID_FIRST_OBJ_OFFSET - 1;

	while(n_objects)
	{
		const device_id_object_t *obj = modbus_device_id_get_object(root, object_id);
		if(obj == NULL)
		{
			object_id = 0;
			obj = modbus_device_id_get_object(root, object_id);
		}

		if(obj)
		{
			/* Data wont fit and needs to be segmentized */
			if((total_size + obj->length + 2) >= (MODBUS_MAX_PDU_LENGTH - MODBUS_DEVICE_ID_FIRST_OBJ_OFFSET))
			{
				rsp_pdu[MODBUS_DEVICE_ID_NEXT_OBJ_OFFSET - 1] = obj->id;
				rsp_pdu[MODBUS_DEVICE_ID_MORE_OFFSET - 1] = 0xFF;
				break;
			}
			total_size += obj->length + 2;

			pData[0] = obj->id;
			pData[1] = obj->length;
			memcpy(&pData[2], obj->data, obj->length);
 			n_objects_in_pdu++;
 			pData += obj->length + 2;
		}
		else
		{
			break;
		}
		n_objects--;
		object_id++;
	}

	rsp_pdu[MODBUS_DEVICE_ID_N_OBJECTS_OFFSET - 1] = n_objects_in_pdu;	
	return total_size;
}

uint8_t modbus_get_mei_type(const uint8_t *pdu)
{
	return pdu[1];
}

uint8_t modbus_get_read_device_id_code(const uint8_t *pdu)
{
	return pdu[2];
}

uint8_t modbus_get_object_id(const uint8_t *pdu)
{
	return pdu[3];
}

void modbus_server_get_device_id(const uint8_t *rsp_pdu, device_id_object_t **root)
{
	uint8_t n_objects = rsp_pdu[MODBUS_DEVICE_ID_N_OBJECTS_OFFSET];

	/* Expects rsp to be in payload location */
	const uint8_t *pObj = &rsp_pdu[MODBUS_DEVICE_ID_FIRST_OBJ_OFFSET];
	uint8_t id, length = 0;
	for(uint8_t n = 0; n < n_objects; n++, length += 2)
	{
		id = pObj[0];
		length = pObj[1];
		const uint8_t *data = &pObj[2];
		if(NULL == modbus_device_id_create_object(root, id, data, length))
		{
			break;
		}
		pObj += length + 2;
	}
}
