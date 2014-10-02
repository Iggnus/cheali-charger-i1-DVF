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
#include "StartInfoStrategy.h"
#include "ProgramData.h"
#include "Screen.h"
#include "memory.h"
#include "Settings.h"

//for LAST_PROGRAMTYPE const ???? (charge without balancer)
#include "Program.h"

namespace StartInfoStrategy {
    uint8_t ok_;
    bool balancePort_;
    void setBalancePort(bool p) {balancePort_ = p;}

    const Strategy::VTable vtable PROGMEM = {
        powerOn,
        powerOff,
        doStrategy
    };

}


void StartInfoStrategy::powerOn()
{
    Screen::blink.startBlinkOn(7);
    Buzzer::begin();
    ok_ = 3;
}

void StartInfoStrategy::powerOff()
{
    Screen::blink.stopBlink();
    Buzzer::soundOff();
}

Strategy::statusType StartInfoStrategy::doStrategy()
{
    bool cell_nr, v_balance, v_out = false, balance;		//ign  v_out = false

    cell_nr = v_balance = false;
//    v_out = ! AnalogInputs::isConnected(AnalogInputs::Vout);		//ign  I need to charge 0-voltage batt's

    if(balancePort_) {
        uint8_t is_cells, should_be_cells;
        is_cells = AnalogInputs::getConnectedBalancePorts();
        should_be_cells = ProgramData::currentProgramData.battery.cells;
        cell_nr = (should_be_cells != is_cells);
        v_balance = (is_cells == 0);

        if(should_be_cells == 1 && is_cells == 0)  {
            //one cell
            cell_nr =   false;
            v_balance = false;
        }
     }

/*
#ifdef CHARGE_LIXX_WITHOUT_BALANCER
    if( (AnalogInputs::getConnectedBalancePorts() == 0) && (v_balance == true) &&
          (absDiff(AnalogInputs::getRealValue(AnalogInputs::Vout),
             AnalogInputs::getRealValue(AnalogInputs::Vbalancer)) > ANALOG_VOLT(0.5)  ))
             {
                 if(ProgramData::currentProgramData.isLiXX())
                 { 
                   if ( Program::programType_ == Program::ChargeLiXX ||
                        Program::programType_ == Program::DischargeLiXX ||
                        Program::programType_ == Program::FastChargeLiXX ||
                        Program::programType_ == Program::StorageLiXX) 
                   {                      
                      //Screen::displayWarningScreen();
                      Buzzer::soundInfo();
                      //without balancer
                      cell_nr =   false;
                      v_balance = false;
                   }  
                  }
              }
#endif    
*/    
    
    if(AnalogInputs::isConnected(AnalogInputs::Vbalancer) &&
            absDiff(AnalogInputs::getRealValue(AnalogInputs::Vout),
               AnalogInputs::getRealValue(AnalogInputs::Vbalancer)) > ANALOG_VOLT(0.5)) v_out = true;

    Screen::blink.blinkIndex_ = 7;
    if(cell_nr)     Screen::blink.blinkIndex_ -= 4;
    if(v_balance)   Screen::blink.blinkIndex_ -= 2;
    if(v_out)       Screen::blink.blinkIndex_ -= 1;

    if(cell_nr || v_balance || v_out) {
        Buzzer::soundInfo();
    } else {
        Buzzer::soundOff();
    }

    balance = (v_balance || cell_nr) && settings.forceBalancePort_;

    if(Keyboard::getPressed() == BUTTON_NONE)
        ok_ = 0;
    if(!balance && !v_out && Keyboard::getPressed() == BUTTON_START) {
        ok_++;
    }
    if(ok_ == 2) {
        return Strategy::COMPLETE;
    }
    return Strategy::RUNNING;
}
