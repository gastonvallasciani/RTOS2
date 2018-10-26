/*
 * fsm_bufferRx.c
 *
 *  Created on: 6/10/2018
 *      Author: gastonvallasciani
 */
/*==================[inclusiones]============================================*/
#include "fsm_bufferRx.h"

#include "sapi.h"
/*==================[definiciones de datos internos]=========================*/
/*==================[definiciones de datos externos]=========================*/

/*==================[definiciones de funciones internas]=====================*/
/*==================[definiciones de funciones externas]=====================*/
/*
 * Funcion de inicilizacion de la maquina de estados de recepcion de trama
 */
void fsmBufferRxInit(bufferStruct_t *bufferStruct, fsmStruct_t *fsmStruct){
	bufferStruct->bufferRxIndex = 0;
	bufferStruct->receptionCompleteFlag = FALSE;
	bufferStruct->dataCounter = 0;
	fsmStruct->fsmActualState = RECEIVE_STX;
}
/*
 * Para verificar si se finalizo la recepcion de la trama se debe utilizar
 * la funcion uint8_t receptionStatus(bufferStruct_t *bufferStruct)
 */
void fsmBufferRxAct(uint8_t dataRx, bufferStruct_t *bufferStruct, fsmStruct_t *fsmStruct){

	switch(fsmStruct->fsmActualState){
	case RECEIVE_STX:
		if(dataRx == STX){
			bufferStruct->bufferRx[bufferStruct->bufferRxIndex] = dataRx;
			bufferStruct->bufferRxIndex++;
			fsmStruct->fsmActualState = RECEIVE_OP;
			bufferStruct->tiempoDeLlegada = xTaskGetTickCount();
		}
		else{
			fsmStruct->fsmActualState = RECEIVE_STX;
		}
		break;
	case RECEIVE_OP:
		if((dataRx=='0')||(dataRx=='1')||(dataRx=='2')||(dataRx=='3')||(dataRx=='4')||(dataRx=='5')){
			bufferStruct->bufferRx[bufferStruct->bufferRxIndex] = dataRx;
			bufferStruct->bufferRxIndex++;
			fsmStruct->fsmActualState = RECEIVE_T;
		}
		else{
			bufferStruct->bufferRxIndex = 0;
			fsmStruct->fsmActualState = RECEIVE_STX;
		}
		break;
	case RECEIVE_T:
			bufferStruct->bufferRx[bufferStruct->bufferRxIndex] = dataRx;
			bufferStruct->bufferRxIndex++;
			bufferStruct->dataLength = dataRx;
			fsmStruct->fsmActualState = RECEIVE_DATA;
		break;
	case RECEIVE_DATA:
		if(dataRx!=ETX){
			bufferStruct->bufferRx[bufferStruct->bufferRxIndex] = dataRx;
			bufferStruct->bufferRxIndex++;
			bufferStruct->dataCounter++;
		}
		else{
			if(bufferStruct->dataCounter == bufferStruct->dataLength){
				fsmStruct->fsmActualState = RECEIVE_STX;
				bufferStruct->bufferRx[bufferStruct->bufferRxIndex] = dataRx;
				bufferStruct->bufferLength = bufferStruct->bufferRxIndex;
				bufferStruct->bufferRxIndex = 0;
				bufferStruct->dataCounter = 0;
				bufferStruct->tiempoDeRecepcion = xTaskGetTickCount();
				bufferStruct->receptionCompleteFlag = TRUE;
			}
			else{
				fsmStruct->fsmActualState = RECEIVE_STX;
				bufferStruct->bufferRxIndex = 0;
				bufferStruct->receptionCompleteFlag = FALSE;
				bufferStruct->dataCounter = 0;
			}
		}
		break;
	default:
		bufferStruct->bufferRxIndex = 0;
		fsmStruct->fsmActualState = RECEIVE_STX;
		break;
	}
}
/*
 * Devuelve 1 si se completo la recepcion de la trama
 * Devuelve 0 si no se completo la recepcion
 */
uint8_t receptionStatus(bufferStruct_t *bufferStruct){
	if(bufferStruct->receptionCompleteFlag == TRUE){
		bufferStruct->receptionCompleteFlag = FALSE;
		return TRUE;
	}
	else{
		return FALSE;
	}
}

/*==================[fin del archivo]========================================*/




