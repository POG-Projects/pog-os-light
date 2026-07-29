#pragma once
#include <Arduino.h>

enum BtnEvent { BTN_NONE = -1, BTN_UP = 0, BTN_DOWN = 1, BTN_LEFT = 2, BTN_RIGHT = 3 };

void buttonsBegin();
int  buttonsPoll();   // renvoie un BtnEvent (front d'appui) ou BTN_NONE, a appeler souvent
