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
#include "TheveninChargeStrategy.h"
#include "SMPS.h"
#include "Hardware.h"
#include "ProgramData.h"
#include "Screen.h"
#include "TheveninMethod.h"
#include "memory.h"

//#include "SerialLog.h"  //ign


namespace TheveninChargeStrategy {
    const Strategy::VTable vtable PROGMEM = {
        powerOn,
        powerOff,
        doStrategy
    };
}

void TheveninChargeStrategy::powerOff()
{
    SMPS::powerOff();
    Balancer::powerOff();
}


void TheveninChargeStrategy::powerOn()
{
    Balancer::powerOn();
    SMPS::powerOn();
    TheveninMethod::initialize(AnalogInputs::IsmpsValue);
}

void TheveninChargeStrategy::setVIB(AnalogInputs::ValueType v, AnalogInputs::ValueType i, bool balance)
{
//SerialLog::printString("TCS::setVIB "); SerialLog::printUInt(v); SerialLog::printD(); SerialLog::printUInt(i);  //ign
//SerialLog::printNL();  //ign
       TheveninMethod::setVIB(v, i, balance);
}
void TheveninChargeStrategy::setMinI(AnalogInputs::ValueType i)
{
       TheveninMethod::setMinI(i);
}

Strategy::statusType TheveninChargeStrategy::doStrategy()
{
    bool update;
    bool isendVout = isEndVout();
    uint16_t oldValue = AnalogInputs::getRealValue(AnalogInputs::Ismps);    //current
//    uint16_t oldValue = AnalogInputs::getAvrADCValue(AnalogInputs::Ismps);    //ign
	
    //test if charge complete
    if(TheveninMethod::isComlete(isendVout, oldValue)) {
        SMPS::powerOff(SMPS::CHARGING_COMPLETE);
        return Strategy::COMPLETE;
    }

//    update = AnalogInputs::isOutStable() || isendVout || TheveninMethod::isBelowMin(oldValue);

//    if(update && !Balancer::isWorking()) {
//    if(!Balancer::isWorking()) {

  uint16_t voltage = TheveninMethod::calculateNewValue(isendVout, oldValue);
  if(SMPS::getValue() != ProgramData::currentProgramData.battery.Ic)
  SMPS::setRealValue(ProgramData::currentProgramData.battery.Ic, voltage);
//}
    return Strategy::RUNNING;
}


bool TheveninChargeStrategy::isEndVout()
{
    AnalogInputs::ValueType Vc = TheveninMethod::Vend_;
    AnalogInputs::ValueType Vc_per_cell = Balancer::calculatePerCell(Vc);

    return Vc <= AnalogInputs::getVout()+50 || Balancer::isMaxVout(Vc_per_cell);
}



