/* Copyright 2017-2018, Eric Pernia
 * All rights reserved.
 *
 * This file is part of sAPI Library.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*==================[inclusiones]============================================*/

// Includes de FreeRTOS
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "driver_uart.h"
#include "fsm_bufferRx.h"

// sAPI header
#include "sapi.h"

// QM Pool
#include "qmpool.h"

#include <string.h>

#define STX 				0x55
#define ETX 				0x40	// 0xAA
#define LARGO_QUEUES 	10		// Tamaño cola
#define N_BYTES_HEADER	4		// Num. bytes header
#define CTE_CONV_Mm		32		// Cte. de conversion Mayus-minus

QueueHandle_t queMinusculizar;
QueueHandle_t queMinusculizado;
QueueHandle_t queMayusculizar;
QueueHandle_t queMayusculizado;
extern QueueHandle_t queIsrTx;
extern QueueHandle_t queIsrRx;

Drivers_t pDriver_Buffer_RX;

enum{
	MAY = '0', MIN, STACK_DISP, HIP_DISP, APP_MEN
};

/*==================[definiciones y macros]==================================*/
QMPool mem_pool_1; /* Estructura de control del Pool */
QMPool mem_pool_2; /* Estructura de control del Pool */
QMPool mem_pool_3; /* Estructura de control del Pool */
QMPool mem_pool_4; /* Estructura de control del Pool */

static uint8_t memoria_para_pool_1[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_pool_2[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_pool_3[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_pool_4[1024]; /* Espacio de almacenamiento para el Pool */

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

// variables necesarias para manejar la maquina de estados que llena el buffer
// de recepcion de la trama recibida

DEBUG_PRINT_ENABLE;

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// Prototipo de funcion de la tarea
static void taskMayusculas( void* taskParmPtr );
static void taskMinusculas( void* taskParmPtr );
static void taskValidarRx( void* taskParmPtr );
static void taskValidarTx( void* taskParmPtr );

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main(void)
{
	// ---------- CONFIGURACIONES ------------------------------
	// Inicializar y configurar la plataforma
	boardConfig();

	// UART for debug messages

	driver_uart_init(UART_USB, 115200);

	//debugPrintlnString( "Memory Pool con freeRTOS y sAPI." );

	queMinusculizar = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t *));
	queMinusculizado = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t *));
	queMayusculizar = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t *));
	queMayusculizado = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t *));
	//queIsrTx = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t));
	//queIsrRx = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t));

	/* Creacion de tareas */
	xTaskCreate(taskMayusculas, (const char *) "taskMayusculas",
	configMINIMAL_STACK_SIZE * 2, 0,
	tskIDLE_PRIORITY + 2,			// Prioridad = + 2
	0);

	/* Creacion de tareas */
	xTaskCreate(taskMinusculas, (const char *) "taskMinusculas",
	configMINIMAL_STACK_SIZE * 2, 0,
	tskIDLE_PRIORITY + 2,			// Prioridad = + 2
	0);

	/* Creacion de tareas */
	xTaskCreate(taskValidarRx, (const char *) "taskValidarRx",
	configMINIMAL_STACK_SIZE * 2, 0,
	tskIDLE_PRIORITY + 2,			// Prioridad = + 2
	0);

	/* Creacion de tareas */
	xTaskCreate(taskValidarTx, (const char *) "taskValidarTx",
	configMINIMAL_STACK_SIZE * 2, 0,
	tskIDLE_PRIORITY + 2,			// Prioridad = + 2
	0);

	// Led para dar señal de vida
	gpioWrite( LED2, ON );

	/* Inicialización del Pools */
	/* Bloques de 10 bytes cada uno */
	QMPool_init(&mem_pool_1,
			memoria_para_pool_1,
			sizeof(memoria_para_pool_1),
			10U);

	/* Bloques de 50 bytes cada uno */
	QMPool_init(&mem_pool_2,
			memoria_para_pool_2,
			sizeof(memoria_para_pool_2),
			50U);

	/* Bloques de 100 bytes cada uno */
	QMPool_init(&mem_pool_3,
			memoria_para_pool_3,
			sizeof(memoria_para_pool_3),
			100U);

	/* Bloques de 256 bytes cada uno */
	QMPool_init(&mem_pool_4,
			memoria_para_pool_4,
			sizeof(memoria_para_pool_4),
			256U);

	// Iniciar scheduler
	vTaskStartScheduler();

	// ---------- REPETIR POR SIEMPRE --------------------------
	while( TRUE ) {
		// Si cae en este while 1 significa que no pudo iniciar el scheduler
	}

	return 0;
}
/*========================[definiciones de tareas]===========================*/

// Implementacion de funcion de la tarea
static void taskValidarRx( void* taskParmPtr )
{
	bufferStruct_t bufferStruct;
	fsmStruct_t fsmStruct;
	uint8_t dataPointer[260], dataSize, OP;
	uint8_t * block1;
	uint8_t i;
	uint8_t datoRx = 0;

	// inicializo la maquina de estados de recepcion de la trama
	fsmBufferRxInit(&bufferStruct, &fsmStruct);

	while(TRUE) {

		if(xQueueReceive(queIsrRx, &datoRx, portMAX_DELAY) == pdTRUE) {

			fsmBufferRxAct(datoRx, &bufferStruct, &fsmStruct);

			if(receptionStatus(&bufferStruct) == COMPLETE){
				/* Realizar funcion */
				dataSize = bufferStruct.bufferRx[2];// - '0';
				OP = bufferStruct.bufferRx[1];

				if(dataSize <= 10){
					block1 = QMPool_get(&mem_pool_1, 0U);
				}
				else if (dataSize <= 50){
					block1 = QMPool_get(&mem_pool_2, 0U);
				}
				else if (dataSize <= 100){
					block1 = QMPool_get(&mem_pool_3, 0U);
				}
				else{
					block1 = QMPool_get(&mem_pool_4, 0U);
				}

				for(i = 0; i <= dataSize; i++) {
					block1[i] = bufferStruct.bufferRx[2+i];
				}

				switch(OP){
					case MAY:
						if(xQueueSend(queMayusculizar, &block1, 0) == pdFALSE) {
						/* Condicion de error */
						}
						break;
					case MIN:
						if(xQueueSend(queMinusculizar, &block1, 0) == pdFALSE) {
						/* Condicion de error */
						}
						break;
					default:
						break;
				}
			}
		}
	}
}

// Implementacion de funcion de la tarea
static void taskValidarTx( void* taskParmPtr )
{
	// ---------- INICIALIZACION ------------------------------
	TickType_t tiempoInicioCiclo;

	uint8_t dataPointer[260],dataSize,OP;
	uint8_t * block1;
	uint16_t i;

	tiempoInicioCiclo = xTaskGetTickCount();

	// ---------- REPETIR POR SIEMPRE --------------------------
	while(TRUE) {

		vTaskDelayUntil(&tiempoInicioCiclo, 100 / portTICK_RATE_MS);

		if(xQueueReceive(queMinusculizado, &block1, 0) == pdTRUE) {
						
			/* Reconstruir paquete segun protocolo */
			dataPointer[0] = STX;
			dataPointer[1] = MIN;
			for(i = 0; i <= block1[0]; i++)
			 {
			 	dataPointer[2+i] = block1[i];
			 } 
			dataPointer[3+block1[0]] = ETX;

			driver_uart_extern_write_string_to_buffer_tx(&dataPointer, dataPointer[2] + N_BYTES_HEADER);

			dataSize = dataPointer[2];

			if(dataSize <= 10){
				QMPool_put(&mem_pool_1, block1);
			}
			else if (dataSize <= 50){
				QMPool_put(&mem_pool_2, block1);
			}
			else if (dataSize <= 100){
				QMPool_put(&mem_pool_3, block1);
			}
			else{
				QMPool_put(&mem_pool_4, block1);	
			}

		}

		if(xQueueReceive(queMayusculizado, &block1, 0) == pdTRUE) {

			/* Reconstruir paquete segun protocolo */
			dataPointer[0] = STX;
			dataPointer[1] = MAY;
			for(i = 0; i <= block1[0]; i++) {
			 	dataPointer[2+i] = block1[i];
			}
			dataPointer[3+block1[0]] = ETX;

			driver_uart_extern_write_string_to_buffer_tx(&dataPointer, dataPointer[2] + N_BYTES_HEADER);

			dataSize = dataPointer[2];

			if(dataSize <= 10){
				QMPool_put(&mem_pool_1, block1);
			}
			else if (dataSize <= 50){
				QMPool_put(&mem_pool_2, block1);
			}
			else if (dataSize <= 100){
				QMPool_put(&mem_pool_3, block1);
			}
			else{
				QMPool_put(&mem_pool_4, block1);	
			}
		}
	}
}

// Implementacion de funcion de la tarea
static void taskMayusculas( void* taskParmPtr )
{
	// ---------- INICIALIZACION ------------------------------
	uint8_t * data;
	uint32_t i;

	// ---------- REPETIR POR SIEMPRE --------------------------
	while(TRUE) {

		i = 0;

		if(xQueueReceive(queMayusculizar, &data, portMAX_DELAY) == pdTRUE) {

			while(i < data[0]) {	// T bytes

				i++;
				/* Convierto a mayusculas solo los caracteres en minusculas */
				if(data[i] >= 'a' && data[i] <= 'z') {
					data[i] -= CTE_CONV_Mm;
				}
			}

			if(xQueueSend(queMayusculizado, &data, 0) == pdFALSE) {
				/* Condicion de error */
			}
		}
	}
}

// Implementacion de funcion de la tarea
static void taskMinusculas( void* taskParmPtr )
{
	// ---------- INICIALIZACION ------------------------------
	uint8_t * data;
	uint32_t i;

	// ---------- REPETIR POR SIEMPRE --------------------------
	while(TRUE) {

		i = 0;

		if(xQueueReceive(queMinusculizar, &data, portMAX_DELAY) == pdTRUE) {

			while(i < data[0]) {	// T bytes

				i++;
				/* Convierto a minusculas solo los caracteres en mayusculas */
				if(data[i] >= 'A' && data[i] <= 'Z') {
					data[i] += CTE_CONV_Mm;
				}
			}
			if(xQueueSend(queMinusculizado, &data, 0) == pdFALSE) {
				/* Condicion de error */
			}
		}
	}
}

/*==================[fin del archivo]========================================*/
