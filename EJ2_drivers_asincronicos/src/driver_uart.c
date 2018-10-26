/*==================[inclusions]=============================================*/

#include "driver_uart.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "sapi.h"
#include "sapi_uart.h"
#include "sapi_datatypes.h"

#define UART_N_QUEUES	1
#define STX 				0x55
#define ETX 				0x40	// 0xAA

/*==================[macros]=================================================*/

// Buffers TX y RX (ambos consumidor y productor)
Drivers_t Driver_Buffer_RX, Driver_Buffer_TX;
uartMap_t uart_name;

QueueHandle_t queIsrTx;
QueueHandle_t queIsrRx;

SemaphoreHandle_t driver_uart_semaforo_rx;

/*==================[typedef]================================================*/

typedef struct {
   LPC_USART_T*      uartAddr;
   lpc4337ScuPin_t   txPin;
   lpc4337ScuPin_t   rxPin;
   IRQn_Type         uartIrqAddr;
} uartLpcInit_t;

/*==================[internal data declaration]==============================*/

uartLpcInit_t driver_uarts[] = {
   // UART_USB
   { LPC_USART2, { 7, 1, FUNC6 }, { 7, 2, FUNC6 }, USART2_IRQn }, // 3
};

LPC_USART_T *uart_addr;

uint8_t buffer_test[260]= "123456789";

/*==================[internal functions declaration]=========================*/

/*==================[internal functions definition]==========================*/

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/

// inicializar driver uart
void driver_uart_init(uartMap_t uart, uint32_t baudRate) {
	uint32_t intMask;
	uart_name = uart;
	uart_addr = driver_uarts[0].uartAddr;

	uartConfig(uart_name, baudRate);

	// Enable THRE irq (RX)
	intMask = UART_IER_RBRINT | UART_IER_RLSINT;
	Chip_UART_IntEnable(uart_addr, intMask);
    // Enable THRE irq (TX)
    intMask = UART_IER_THREINT;
    Chip_UART_IntEnable(uart_addr, intMask);

	uartInterrupt(uart_name, TRUE);


	queIsrTx = xQueueCreate(UART_N_QUEUES, sizeof(Drivers_t));
	queIsrRx = xQueueCreate(UART_N_QUEUES, sizeof(uint8_t)); // solo vamos a transmitir un char por cada vez

	driver_uart_semaforo_rx = xSemaphoreCreateBinary();

	Driver_Buffer_RX.CantidadDatos = NDATOS;
	Driver_Buffer_RX.Ind_escritura = 0;
	Driver_Buffer_RX.Ind_lectura 	 = 0;

	Driver_Buffer_TX.CantidadDatos = NDATOS;
	Driver_Buffer_TX.Ind_escritura = 0;
	Driver_Buffer_TX.Ind_lectura 	 = 0;

	// linea de test
	driver_uart_extern_write_string_to_buffer_tx(&buffer_test, 9);   //  Probamos la transmision de datos
	driver_uart_extern_write_string_to_buffer_tx(&buffer_test, 9);   //  Probamos la transmision de datos
}

// funcion de interrupcion
void UART2_IRQHandler(void)
{
	uint32_t status;
	uint8_t DatoRX, DatoTX;
	static BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	uint32_t intMask;

	status = Chip_UART_ReadLineStatus( uart_addr );

	// Rx Interrupt
	if(status & UART_LSR_RDR) { // uartRxReady
		// Execute
		DatoRX = uartRxRead(uart_name);
		xQueueSendFromISR(queIsrRx, &DatoRX, &pxHigherPriorityTaskWoken);// se envia por cola el char recibido por uart
	}

	// Tx Interrupt
	if( ( status & UART_LSR_THRE ) && // uartTxReady
			( Chip_UART_GetIntsEnabled( uart_addr ) & UART_IER_THREINT ) ) {

		// Leo dato del buffer, si el buffer esta vacio no entre a enviar dato
		if(driver_uart_get_char_from_buffer(&DatoTX,&Driver_Buffer_TX)){
			uartTxWrite( uart_name, DatoTX );// Pongo dato en elregistro TX ; Enciendo interrupcion TX
		}
		else {
			intMask = UART_IER_THREINT;
			Chip_UART_IntDisable(uart_addr, intMask);
		}
	}

	portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}

// Obtengo un dato del buffer indicado
uint8_t driver_uart_get_char_from_buffer(uint8_t * dato, Drivers_t * buffer) {

	// Hay nuevo dato en el buffer?
	if(buffer->Ind_lectura != buffer->Ind_escritura) {
		* dato = buffer->Buffer[buffer->Ind_lectura];
		buffer->Ind_lectura = (buffer->Ind_lectura+1) % buffer->CantidadDatos;
		buffer->Ultimo_char = buffer->Buffer[buffer->Ind_escritura];// leemos el utlimo char
		buffer->Buffer_Vacio = 0;
		buffer->Buffer_Error = 0;
		return 1;	// Hay nuevo dato
	}
	else {
		buffer->Buffer_Vacio = 1;
		return 0;	// No Hay
	}
}

// Escribo un dato en el buffer indicado
uint8_t driver_uart_write_char_to_buffer(uint8_t  dato, Drivers_t * buffer)
{
	if(((buffer->Ind_escritura + 1) % buffer->CantidadDatos) != buffer->Ind_lectura) {
		buffer->Buffer[buffer->Ind_escritura] = dato;
		buffer->Ultimo_char = buffer->Buffer[buffer->Ind_escritura];// leemos el utlimo char ingresado
		buffer->Ind_escritura = (buffer->Ind_escritura + 1) % buffer->CantidadDatos;
		buffer->Buffer_Vacio = 0;
		buffer->Buffer_Error = 0;
		return 1;	// Se pudo escribir correctamente
	}
	else {
		buffer->Buffer_Error = 1;	// Buffer lleno
		return 0;						// No se pudo escribir correctamente
	}
}

// Escribo un string al buffer indicado
uint8_t driver_uart_write_string_to_buffer(uint8_t * strdato, uint16_t sizestr, Drivers_t * buffer) {

	uint16_t i = 0;

	for(i = 0; i < sizestr; i++)
		if(0 == driver_uart_write_char_to_buffer(strdato[i], buffer)) return 0;
}

// Escribimos un string en el Buffer TX
void driver_uart_extern_write_string_to_buffer_tx(uint8_t * strdato, uint16_t sizestr) {

	uint8_t DatoTX=0;
	uint32_t intMask;


	if(driver_uart_write_string_to_buffer(strdato, sizestr, &Driver_Buffer_TX)) {
		if(0 == (Driver_Buffer_TX.Buffer_Vacio || Driver_Buffer_TX.Buffer_Error)) {
			// Leo dato del buffer, si el buffer esta vacio no entre a enviar dato
			if(driver_uart_get_char_from_buffer(&DatoTX, &Driver_Buffer_TX)){
				// Pongo dato en elregistro TX ; Enciendo interrupcion TX
				uartTxWrite(uart_name, DatoTX);
   				intMask = UART_IER_THREINT;
    			Chip_UART_IntEnable(uart_addr, intMask);
			}
		}
	}
}

// Leemos un char del buffer RX
uint8_t driver_uart_extern_get_char_from_buffer_rx(uint8_t * dato) {
	return driver_uart_get_char_from_buffer(dato, &Driver_Buffer_RX);
}

// Leemos el buffer rx
uint8_t driver_uart_extern_get_string_from_buffer_rx(uint8_t * strdato, uint8_t * sizestr) {

	if(xSemaphoreTake(driver_uart_semaforo_rx, portMAX_DELAY)) {
		* sizestr = 0;
		while(driver_uart_extern_get_char_from_buffer_rx((uint8_t *) &strdato[* sizestr])) {
			* sizestr++;
		}
	}
	if(* sizestr)
		return 1;
	else
		return 0;
}

// Lee el buffer y devuelve los datos en un buffer
void driver_uart_extern_get_string_from_buffer(uint8_t * strdato, Drivers_t * buffer) {

	uint16_t i = 0 ;

	while(driver_uart_get_char_from_buffer((uint8_t *) &strdato[i], buffer))
	{
		i++;
	}
}

/*==================[end of file]============================================*/
