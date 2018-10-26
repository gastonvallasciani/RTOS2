#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_

/*==================[inclusions]=============================================*/

/*==================[cplusplus]==============================================*/

#ifdef __cplusplus
extern "C" {
#endif
#include "sapi.h"

/*==================[macros]=================================================*/

#define NDATOS 260

/*==================[typedef]================================================*/

// defino tipo de estructura de la MEF de UART
typedef struct{
	uint8_t Buffer[NDATOS]; 		// Buffer de datos circular
	uint16_t Ind_lectura;			// Indice de lectura
	uint16_t Ind_escritura;
	bool_t  Buffer_Vacio;
	bool_t  Buffer_Error;
	uint16_t CantidadDatos;
	uint8_t  Ultimo_char;
} Drivers_t;

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/

void driver_uart_init(uartMap_t uart, uint32_t baudRate);
void UART2_IRQHandler(void);

uint8_t driver_uart_get_char_from_buffer(uint8_t * dato, Drivers_t * buffer);
uint8_t driver_uart_write_char_to_buffer(uint8_t dato, Drivers_t * buffer);
uint8_t driver_uart_write_string_to_buffer(uint8_t * strdato, uint16_t sizestr, Drivers_t * buffer);
void driver_uart_extern_write_string_to_buffer_tx(uint8_t * strdato, uint16_t  sizestr);
uint8_t driver_uart_extern_get_char_from_buffer_rx(uint8_t * dato);
uint8_t driver_uart_extern_get_string_from_buffer_rx(uint8_t * strdato, uint8_t * sizestr);
void driver_uart_extern_get_string_from_buffer(uint8_t * strdato, Drivers_t * buffer);

/*==================[cplusplus]==============================================*/

#ifdef __cplusplus
}
#endif

/*==================[end of file]============================================*/
#endif /* #ifndef _DRIVER_UART_H_ */
