/*
 * TimerService.h
 */

#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include "FrameworkEventos.h"

#define TIMER_DISABLED ((int)-1)

void 		TimerArmUnico		( Modulo_t * modulo, unsigned int time );
void 		TimerArmRepetitivo	( Modulo_t * modulo, unsigned int timeout );
void 		TimerDisarm			( Modulo_t * modulo );

#endif /* TIMEOUT_H_ */
