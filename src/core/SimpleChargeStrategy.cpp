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
#include "SimpleChargeStrategy.h"
#include "ProgramData.h"
#include "Screen.h"
#include "Hardware.h"
#include "TheveninMethod.h"


void SimpleChargeStrategy::powerOn()
{
    SMPS::powerOn();
    AnalogInputs::ValueType I = ProgramData::currentProgramData.battery.Ic;
    AnalogInputs::ValueType Vc = ProgramData::currentProgramData.getVoltage(ProgramData::VCharge);
    I/=5; //start charging with 0.2CmAh
//    uint16_t value = AnalogInputs::reverseCalibrateValue(AnalogInputs::IsmpsValue, I);
    TheveninMethod::setVIB(Vc, I, false);
    TheveninMethod::initialize(AnalogInputs::IsmpsValue);
    SMPS::setRealValue(I, Vc);    //ign
}

void SimpleChargeStrategy::powerOff()
{
    SMPS::powerOff();
}

void SimpleChargeStrategy::calculateThevenin()
{
    if(AnalogInputs::isOutStable()) TheveninMethod::calculateRthVth(AnalogInputs::getRealValue(AnalogInputs::Ismps));
}




