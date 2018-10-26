/**
 * RTOS 2: Practica N3
 *
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
#include "FrameworkEventos.h"
// sAPI header
#include "sapi.h"
// QM Pool
#include "qmpool.h"
#include <string.h>

/*==================[definiciones y macros]==================================*/

#define STX 					0x55
#define ETX 					0x40	// 0xAA
#define LARGO_QUEUES 		10		// Tamano cola
#define N_BYTES_HEADER		4		// Num. bytes header
#define CTE_CONV_Mm			32		// Cte. de conversion Mayus-minus
#define TIME_LENGTH_VECTOR 32

enum {
	MAY = '0', MIN, STACK_DISP, HIP_DISP, APP_MEN, MED_PER, BOT_EVENT
};

typedef struct {
	uint32_t id_de_paquete;
	uint8_t *payload;
	TickType_t tiempo_de_llegada;
	TickType_t tiempo_de_recepcion;
	TickType_t tiempo_de_inicio;
	TickType_t tiempo_de_fin;
	TickType_t tiempo_de_salida;
	TickType_t tiempo_de_transmision;
	uint16_t largo_del_paquete;
	uint16_t memoria_alojada;
	uint8_t op;
} token_t;

typedef union{
	TickType_t time;
	uint8_t timePtr[4];
} unionByte_t;

typedef union{
	uint16_t var;
	uint8_t varPtr[2];
} unionByte16_t;

/*==================[definiciones de datos internos]=========================*/

QueueHandle_t queMinusculizar;
QueueHandle_t queMayusculizar;
QueueHandle_t queMedirPerformance;
QueueHandle_t queTransmision;

Drivers_t pDriver_Buffer_RX;

QMPool mem_pool_1; 		/* Estructura de control del Pool */
QMPool mem_pool_2; 		/* Estructura de control del Pool */
QMPool mem_pool_3; 		/* Estructura de control del Pool */
QMPool mem_pool_4; 		/* Estructura de control del Pool */
QMPool mem_token_pool; 	/* Estructura de control del Pool */

static uint8_t memoria_para_pool_1[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_pool_2[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_pool_3[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_pool_4[1024]; /* Espacio de almacenamiento para el Pool */
static uint8_t memoria_para_token[1024]; 	/* Espacio de almacenamiento para el Pool */

Modulo_t * ModuloDriverLeds;
Modulo_t * ModuloDriverPulsadores;

/*==================[definiciones de datos externos]=========================*/

extern QueueHandle_t queIsrRx;

DEBUG_PRINT_ENABLE;

/*==================[declaraciones de funciones internas]====================*/

// Prototipos de funciones de tareas
static void taskMayusculas(void* taskParmPtr);
static void taskMinusculas(void* taskParmPtr);
static void taskValidarRx(void* taskParmPtr);
static void taskValidarTx(void* taskParmPtr);
static void taskMedirPerformance(void* taskParmPtr);

/*==================[declaraciones de funciones externas]====================*/

/*==================[funcion principal]======================================*/

int main(void) {

	boardConfig();

	driver_uart_init(UART_USB, 115200);

	queMinusculizar = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t *));
	queMayusculizar = xQueueCreate(LARGO_QUEUES, sizeof(uint8_t *));
	queMedirPerformance = xQueueCreate(LARGO_QUEUES, sizeof(token_t *));
	queTransmision = xQueueCreate(LARGO_QUEUES, sizeof(token_t *));

	colaEventosPrioridadBaja = xQueueCreate(15, sizeof(Evento_t));

	// Creo la tarea de baja prioridad
	xTaskCreate(TareaDespachadoraEventos, "Control",
	5 * configMINIMAL_STACK_SIZE,
	(void*) colaEventosPrioridadBaja,
	PRIORIDAD_BAJA,
	NULL );

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

	/* Creacion de tareas */
	xTaskCreate(taskMedirPerformance, (const char *) "taskMedirPerformance",
	configMINIMAL_STACK_SIZE * 2, 0,
	tskIDLE_PRIORITY + 2,			// Prioridad = + 2
	0);

	/* InicializaciÃ³n del Pools */
	/* Bloques de 10 bytes cada uno */
	QMPool_init(&mem_pool_1, memoria_para_pool_1, sizeof(memoria_para_pool_1),
			10U);

	/* Bloques de 50 bytes cada uno */
	QMPool_init(&mem_pool_2, memoria_para_pool_2, sizeof(memoria_para_pool_2),
			50U);

	/* Bloques de 100 bytes cada uno */
	QMPool_init(&mem_pool_3, memoria_para_pool_3, sizeof(memoria_para_pool_3),
			100U);

	/* Bloques de 256 bytes cada uno */
	QMPool_init(&mem_pool_4, memoria_para_pool_4, sizeof(memoria_para_pool_4),
			256U);
	/* Bloques de 256 bytes cada uno */
	QMPool_init(&mem_token_pool, memoria_para_token, sizeof(memoria_para_token),
			sizeof(token_t));

	/* Registro e inicio módulos del framework basado en eventos */
	ModuloDriverLeds			= RegistrarModulo(DriverLeds, PRIORIDAD_BAJA);
	ModuloDriverPulsadores	= RegistrarModulo(DriverPulsadores, PRIORIDAD_BAJA);
	IniciarTodosLosModulos();

	// Iniciar scheduler
	vTaskStartScheduler();

	while ( TRUE) {
		// Si cae en este while 1 significa que no pudo iniciar el scheduler
	}

	return 0;
}
/*========================[definiciones de tareas]===========================*/
static void taskMedirPerformance(void* taskParmPtr) {
	token_t * token;
	uint32_t i;

	while (TRUE) {

		i = 0;

		if (xQueueReceive(queMedirPerformance, &token, portMAX_DELAY) == pdTRUE){
			token->tiempo_de_inicio = xTaskGetTickCount();
			while (i < token->payload[0]) {	// T bytes
				i++;

				if (token->payload[i] >= 'a' && token->payload[i] <= 'z') {
					token->payload[i] -= CTE_CONV_Mm;
				}
			}
			token->tiempo_de_fin = xTaskGetTickCount();
			if (xQueueSend(queTransmision, &token, 0) == pdFALSE) {

			}
		}
	}
}

// Implementacion de funcion de la tarea
static void taskValidarRx(void* taskParmPtr) {
	bufferStruct_t bufferStruct;
	fsmStruct_t fsmStruct;
	uint8_t dataPointer[260], dataSize, OP;
	uint8_t * block1;
	token_t * blockToken;
	uint8_t i;
	uint8_t datoRx = 0;
	uint32_t id = 0;

	// inicializo la maquina de estados de recepcion de la trama
	fsmBufferRxInit(&bufferStruct, &fsmStruct);

	while (TRUE) {

		if (xQueueReceive(queIsrRx, &datoRx, portMAX_DELAY) == pdTRUE) {

			fsmBufferRxAct(datoRx, &bufferStruct, &fsmStruct);

			if (receptionStatus(&bufferStruct) == COMPLETE) {
				/* Realizar funcion */
				dataSize = bufferStruct.bufferRx[2];
				OP = bufferStruct.bufferRx[1];

				blockToken = QMPool_get(&mem_token_pool, 0U);

				blockToken->op = OP;

				if (dataSize <= 10) {
					blockToken->memoria_alojada = 10;
					block1 = QMPool_get(&mem_pool_1, 0U);
				} else if (dataSize <= 50) {
					blockToken->memoria_alojada = 50;
					block1 = QMPool_get(&mem_pool_2, 0U);
				} else if (dataSize <= 100) {
					blockToken->memoria_alojada = 100;
					block1 = QMPool_get(&mem_pool_3, 0U);
				} else {
					blockToken->memoria_alojada = 250;
					block1 = QMPool_get(&mem_pool_4, 0U);
				}

				for (i = 0; i <= dataSize; i++) {
					block1[i] = bufferStruct.bufferRx[2 + i];
				}

				blockToken->payload = block1;

				switch (OP) {
				case MAY:
					if (xQueueSend(queMayusculizar, &blockToken, 0) == pdFALSE) {
						/* Condicion de error */
					}
					break;
				case MIN:
					if (xQueueSend(queMinusculizar, &blockToken, 0) == pdFALSE) {
						/* Condicion de error */
					}
					break;
				case MED_PER:
					//Se inicializa aca porque antes de inicializarlo se debe alocar
					//la memoria de la estructura
					blockToken->id_de_paquete = id;
					id++;
					blockToken->largo_del_paquete = dataSize + N_BYTES_HEADER;
					blockToken->tiempo_de_llegada = bufferStruct.tiempoDeLlegada;
					blockToken->tiempo_de_recepcion = bufferStruct.tiempoDeRecepcion;
					if (xQueueSend(queMedirPerformance, &blockToken, 0) == pdFALSE){
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

// Tarea de transmision
static void taskValidarTx(void* taskParmPtr) {
	TickType_t tiempoInicioCiclo;

	unionByte_t unionByte;
	unionByte16_t unionByte16;

	uint8_t dataPointer[260], dataSize, OP,j = 0;
	uint16_t i;
	token_t *token;
	uint8_t *timePtr;

	tiempoInicioCiclo = xTaskGetTickCount();

	while (TRUE) {

		vTaskDelayUntil(&tiempoInicioCiclo, 100 / portTICK_RATE_MS);

		if (xQueueReceive(queTransmision, &token, 0) == pdTRUE) {
			//Se rearma la trama de salida
			dataPointer[0] = STX;
			dataPointer[1] = token->op;
			for (i = 0; i <= token->payload[0]; i++) {
				dataPointer[2 + i] = token->payload[i];
			}
			dataPointer[3 + token->payload[0]] = ETX;

			dataSize = dataPointer[2];

			//Cuando se envia la trama de salida a la interfaz de la
			//uart tx se toma el tiempo de salida
			token->tiempo_de_salida = xTaskGetTickCount();

			driver_uart_extern_write_string_to_buffer_tx(&dataPointer,
					dataPointer[2] + N_BYTES_HEADER);

			memset(dataPointer, '\0', 260);

			//cuando se finaliza se toma el tiempo de transmision
			token->tiempo_de_transmision = xTaskGetTickCount();

			//Se parsean los tiempos y se cargan en un vector
			//para enviarlos por la uart
			unionByte.time = token->tiempo_de_llegada;
			for(j=0;j<4;j++){
				dataPointer[j] = unionByte.timePtr[3-j];
			}

			unionByte.time = token->tiempo_de_recepcion;
			for(j=0;j<4;j++){
				dataPointer[4 + j] = unionByte.timePtr[3-j];
			}

			unionByte.time = token->tiempo_de_inicio;
			for(j=0;j<4;j++){
				dataPointer[8 + j] = unionByte.timePtr[3-j];
			}
			unionByte.time = token->tiempo_de_fin;
			for(j=0;j<4;j++){
				dataPointer[12 + j] = unionByte.timePtr[3-j];
			}

			unionByte.time = token->tiempo_de_salida;
			for(j=0;j<4;j++){
				dataPointer[16 + j] = unionByte.timePtr[3-j];
			}

			unionByte.time = token->tiempo_de_transmision;
			for(j=0;j<4;j++){
				dataPointer[20 + j] = unionByte.timePtr[3-j];
			}

			unionByte16.var = token->largo_del_paquete;
			for(j=0;j<2;j++){
				dataPointer[24 + j] = unionByte16.varPtr[1-j];
			}

			unionByte16.var = token->memoria_alojada;
			for(j=0;j<2;j++){
				dataPointer[26 + j] = unionByte16.varPtr[1-j];
			}

			unionByte.time = token->id_de_paquete;
			for(j=0;j<4;j++){
				dataPointer[28 + j] = unionByte.timePtr[3-j];
			}

			//Se envia el vector de tiempos por la uart
			driver_uart_extern_write_string_to_buffer_tx(&dataPointer,
					TIME_LENGTH_VECTOR);

			// Se liberan los pools de memoria alocados
			if (dataSize <= 10) {
				QMPool_put(&mem_pool_1, token->payload);
			} else if (dataSize <= 50) {
				QMPool_put(&mem_pool_2, token->payload);
			} else if (dataSize <= 100) {
				QMPool_put(&mem_pool_3, token->payload);
			} else {
				QMPool_put(&mem_pool_4, token->payload);
			}
			QMPool_put(&mem_token_pool, token);

		}
	}
}

// Tarea Mayusculizar
static void taskMayusculas(void* taskParmPtr) {
	uint32_t i;
	token_t * token;

	while (TRUE) {
		i = 0;
		if (xQueueReceive(queMayusculizar, &token, portMAX_DELAY) == pdTRUE) {

			while (i < token->payload[0]) {
				i++;
				// Convierto a mayusculas solo los caracteres en minusculas
				if (token->payload[i] >= 'a' && token->payload[i] <= 'z') {
					token->payload[i] -= CTE_CONV_Mm;
				}
			}
			if (xQueueSend(queTransmision, &token, 0) == pdFALSE) {
				// Condicion de error
			}
		}
	}
}

// Tarea Minusculizar
static void taskMinusculas(void* taskParmPtr) {
	uint32_t i;
	token_t * token;

	while (TRUE) {
		i = 0;
		if (xQueueReceive(queMinusculizar, &token, portMAX_DELAY) == pdTRUE) {

			while (i < token->payload[0]) {	// T bytes
				i++;
				// Convierto a minusculas solo los caracteres en mayusculas
				if (token->payload[i] >= 'A' && token->payload[i] <= 'Z') {
					token->payload[i] += CTE_CONV_Mm;
				}
			}
			if (xQueueSend(queTransmision, &token, 0) == pdFALSE) {
				// Condicion de error
			}
		}
	}
}

void vApplicationIdleHook ( void ) {

}

/*==================[fin del archivo]========================================*/
