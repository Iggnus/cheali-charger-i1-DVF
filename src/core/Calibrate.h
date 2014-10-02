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
#ifndef CALIBRATE_H_
#define CALIBRATE_H_

#include "AnalogInputs.h"

#define ACCEPT_DELAY 3

#ifdef ENABLE_SIMPLIFIED_VB0_VB2_CIRCUIT
#define ENABLE_B0_CALIBRATION
#endif

namespace Calibrate {
    void run();
    void calibrateVoltage();
    bool checkCalibrate(AnalogInputs::ValueType , AnalogInputs::Name);
    void checkCalibrateIdischarge();
    void checkCalibrateIcharge();
};

#endif /* CALIBRATE_H_ */
