#ifndef PULSADORES_H_
#define PULSADORES_H_

#include "FrameworkEventos.h"
#include "sapi.h"

typedef enum {
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_RISING,
	BUTTON_FALLING
} ButtonState_t;

typedef struct {
	ButtonState_t estado;
	delay_t delay;
	gpioMap_t teclaPin;
	uint8_t teclaN;
} tecla_antirrebote_t;

void DriverPulsadores (Evento_t *evn);
void DriverPulsadoresInitMEF (tecla_antirrebote_t * tecla, gpioMap_t gpio, uint8_t teclaN );
void AntirreboteMEF (tecla_antirrebote_t * tecla);

#endif /* PULSADORES_H_ */
