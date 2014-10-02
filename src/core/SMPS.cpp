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
#include "Hardware.h"
#include "SMPS.h"
#include "Program.h"
#include "EditMenu.h"
#include "LcdPrint.h"
#include "Screen.h"
#include "Settings.h"

#include "TheveninMethod.h"		//ign


namespace SMPS {
    STATE state_;
    uint16_t current_, voltage_;		//ign	


    STATE getState()    { return state_; }
    bool isPowerOn()    { return getState() == CHARGING; }
    bool isWorking()    { return current_ != 0; }


    uint16_t getValue() { return current_; }

}

void SMPS::initialize()
{
    current_ = 0;
    setValue(0, 0);
    powerOff(CHARGING_COMPLETE);
}


void SMPS::setValue(uint16_t current, uint16_t voltage)
{
    //if (settings.calibratedState_ >= 7) //disable limit if uncalibrated.
    if (Program::programState_ != Program::Calibration)
    {
      if(current > settings.SMPS_Upperbound_Value_) current = settings.SMPS_Upperbound_Value_;
    }

    current_ = current;
	hardware::setChargerValue(AnalogInputs::reverseCalibrateValue(AnalogInputs::IsmpsValue, current_), AnalogInputs::reverseCalibrateValue(AnalogInputs::Vout_plus_pin, voltage));
    AnalogInputs::resetMeasurement();
}

void SMPS::setRealValue(uint16_t I)
{
	voltage_ = AnalogInputs::reverseCalibrateValue(AnalogInputs::Vout_plus_pin, TheveninMethod::Vend_);		//ign
    uint16_t current = AnalogInputs::reverseCalibrateValue(AnalogInputs::IsmpsValue, I);
    setValue(current, voltage_);
}

void SMPS::powerOn()
{
    if(isPowerOn())
        return;
    //reset rising current
    current_ = 0;
    setValue(0, 0);
    hardware::setChargerOutput(true);
    state_ = CHARGING;
}


void SMPS::powerOff(STATE reason)
{
    if(!isPowerOn() || reason == CHARGING)
        return;

    setValue(0, 0);
    //reset rising current
    current_ = 0;
    hardware::setChargerOutput(false);
    state_ = reason;
}


