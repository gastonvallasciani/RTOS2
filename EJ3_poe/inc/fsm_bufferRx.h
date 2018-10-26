/*
 * fsm_bufferRx.h
 *
 *  Created on: 6/10/2018
 *      Author: gastonvallasciani
 */

#ifndef _FSM_BUFFERRX_H_
#define _FSM_BUFFERRX_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "sapi.h"

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
/*==================[definiciones de datos internos]=========================*/
/*==================[definiciones de datos externos]=========================*/
#define STX 				0x55
#define ETX 				0x40	// 0xAA
#define COMPLETE			TRUE

typedef struct{
	uint16_t bufferRxIndex;
	uint8_t bufferRx[250];
	uint8_t bufferLength;
	uint8_t receptionCompleteFlag;
	TickType_t tiempoDeLlegada;
	TickType_t tiempoDeRecepcion;
	uint8_t dataLength;
	uint8_t dataCounter;
}bufferStruct_t;

typedef enum{
	RECEIVE_STX,
	RECEIVE_OP,
	RECEIVE_T,
	RECEIVE_DATA
}fsmEnum_t;

typedef struct{
	fsmEnum_t fsmActualState;
}fsmStruct_t;

/*==================[definiciones de funciones internas]=====================*/
/*==================[definiciones de funciones externas]=====================*/
void fsmBufferRxInit(bufferStruct_t *bufferStruct, fsmStruct_t *fsmStruct);
void fsmBufferRxAct(uint8_t dataRx, bufferStruct_t *bufferStruct, fsmStruct_t *fsmStruct);
uint8_t receptionStatus(bufferStruct_t *bufferStruct);
/*==================[fin del archivo]========================================*/
#endif /* _FSM_BUFFERRX_H_ */
