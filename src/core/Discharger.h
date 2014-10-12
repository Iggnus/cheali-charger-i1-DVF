/*
    cheali-charger - open source firmware for a variety of LiPo chargers
    Copyright (C) 2013  Paweł Stawicki. All right reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DISCHARGER_H
#define DISCHARGER_H

#include "Hardware.h"

namespace Discharger {
    enum STATE { DISCHARGING, DISCHARGING_COMPLETE, ERROR};


    void initialize();

    STATE getState();
    bool isPowerOn();
    bool isWorking();


    uint16_t getValue();
    void setValue(uint16_t current, uint16_t voltage);
    void setRealValue(uint16_t I, uint16_t V);

    void powerOn();
    void powerOff(STATE reason = DISCHARGING_COMPLETE);

    void doIdle();
};


#endif //DISCHARGER_H
