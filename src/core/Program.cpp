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
#include "Program.h"
#include "Hardware.h"
#include "ProgramData.h"
#include "LcdPrint.h"
#include "Utils.h"
#include "Screen.h"
#include "SimpleChargeStrategy.h"
#include "TheveninChargeStrategy.h"
#include "TheveninDischargeStrategy.h"
#include "DeltaChargeStrategy.h"
#include "StorageStrategy.h"
#include "Monitor.h"
#include "memory.h"
#include "StartInfoStrategy.h"
#include "Buzzer.h"
#include "StaticMenu.h"
#include "Settings.h"
#include "SerialLog.h"
#include "DelayStrategy.h"


Program::ProgramType Program::programType_;
Program::ProgramState Program::programState_ = Program::Done;
const char * Program::stopReason_;

namespace {
    const Screen::ScreenType deltaChargeScreens[] PROGMEM = {
      Screen::ScreenFirst,
      Screen::ScreenEnergy,
      Screen::ScreenDeltaFirst,
      Screen::ScreenDeltaVout,
      Screen::ScreenDeltaTextern,
      Screen::ScreenR,
      Screen::ScreenVout,
      Screen::ScreenVinput,
      Screen::ScreenTime,
      Screen::ScreenTemperature,
      Screen::ScreenCycles,
      Screen::ScreenCIVlimits,
      Screen::ScreenEnd
    };

    const Screen::ScreenType NiXXDischargeScreens[] PROGMEM = {
      Screen::ScreenFirst,
      Screen::ScreenEnergy,
      Screen::ScreenDeltaTextern,
      Screen::ScreenR,
      Screen::ScreenVout,
      Screen::ScreenVinput,
      Screen::ScreenTime,
      Screen::ScreenTemperature,
      Screen::ScreenCycles,
      Screen::ScreenCIVlimits,
      Screen::ScreenEnd
    };

    const Screen::ScreenType theveninScreens[] PROGMEM = {
      Screen::ScreenFirst,

      Screen::ScreenEnergy,
      Screen::ScreenBalancer1_3,            Screen::ScreenBalancer4_6,      BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9,)
      Screen::ScreenBalancer1_3Rth,         Screen::ScreenBalancer4_6Rth,   BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9Rth,)
      Screen::ScreenR,
      Screen::ScreenVout,
      Screen::ScreenVinput,
      Screen::ScreenTime,
      Screen::ScreenTemperature,
      Screen::ScreenCycles,
      Screen::ScreenCIVlimits,
      Screen::ScreenEnd

    };
    const Screen::ScreenType balanceScreens[] PROGMEM = {
      Screen::ScreenBalancer1_3,            Screen::ScreenBalancer4_6,      BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9,)
      Screen::ScreenTime,
      Screen::ScreenVinput,
      Screen::ScreenTemperature,
      Screen::ScreenEnd
    };
    const Screen::ScreenType dischargeScreens[] PROGMEM = {
      Screen::ScreenFirst,
      Screen::ScreenEnergy,
      Screen::ScreenBalancer1_3,            Screen::ScreenBalancer4_6,      BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9,)
      Screen::ScreenBalancer1_3Rth,         Screen::ScreenBalancer4_6Rth,   BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9Rth,)
      Screen::ScreenR,
      Screen::ScreenVout,
      Screen::ScreenVinput,
      Screen::ScreenTime,
      Screen::ScreenTemperature,
      Screen::ScreenCycles,
      Screen::ScreenCIVlimits,
      Screen::ScreenEnd

    };
    const Screen::ScreenType storageScreens[] PROGMEM = {
      Screen::ScreenFirst,
      Screen::ScreenEnergy,
      Screen::ScreenBalancer1_3,            Screen::ScreenBalancer4_6,      BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9,)
      Screen::ScreenBalancer1_3Rth,         Screen::ScreenBalancer4_6Rth,   BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9Rth,)
      Screen::ScreenR,
      Screen::ScreenVout,
      Screen::ScreenVinput,
      Screen::ScreenTime,
      Screen::ScreenTemperature,
      Screen::ScreenEnd

    };

    const Screen::ScreenType startInfoBalanceScreens[] PROGMEM = {
      Screen::ScreenStartInfo,
      Screen::ScreenBalancer1_3,            Screen::ScreenBalancer4_6,      BALANCER_PORTS_GT_6(Screen::ScreenBalancer7_9,)
      Screen::ScreenVinput,
      Screen::ScreenTemperature,
      Screen::ScreenEnd
    };

    const Screen::ScreenType startInfoScreens[] PROGMEM = {
      Screen::ScreenStartInfo,
      Screen::ScreenVinput,
      Screen::ScreenTemperature,   
      Screen::ScreenEnd
    };

    Strategy::statusType doStrategy(const Screen::ScreenType chargeScreens[], bool exitImmediately = false){

        Strategy::statusType status = Strategy::doStrategy(chargeScreens, exitImmediately);
        return status;
    }

    uint8_t tempCDcycles_ = 0;
    char cycleMode='-';
    
} //namespace {

bool Program::startInfo()
{
    bool balancer = false;
    const Screen::ScreenType * screen = startInfoScreens;
    Strategy::strategy_ = &StartInfoStrategy::vtable;
    if(ProgramData::currentProgramData.isLiXX()) {
        //usues balance port
        balancer = true;
        screen = startInfoBalanceScreens;
    }
    StartInfoStrategy::setBalancePort(balancer);
    return doStrategy(startInfoBalanceScreens, true) == Strategy::COMPLETE;
}

Strategy::statusType Program::runStorage(bool balance, bool immediately = false)
{
    StorageStrategy::setDoBalance(balance);
    StorageStrategy::setVII(ProgramData::currentProgramData.getVoltage(ProgramData::VStorage),
            ProgramData::currentProgramData.battery.Ic, ProgramData::currentProgramData.battery.Id);
    Strategy::strategy_ = &StorageStrategy::vtable;
    return doStrategy(storageScreens);
}
Strategy::statusType Program::runTheveninCharge(int minChargeC,  bool immediately = false)
{
    TheveninChargeStrategy::setVIB(ProgramData::currentProgramData.getVoltage(ProgramData::VCharge), ProgramData::currentProgramData.battery.Ic, false);
    //TheveninChargeStrategy::setMinI(ProgramData::currentProgramData.battery.Ic/minChargeC);		//ign
    Strategy::strategy_ = &TheveninChargeStrategy::vtable;
    return doStrategy(theveninScreens, immediately);
}

Strategy::statusType Program::runTheveninChargeBalance( bool immediately = false)
{
    TheveninChargeStrategy::setVIB(ProgramData::currentProgramData.getVoltage(ProgramData::VCharge),
            ProgramData::currentProgramData.battery.Ic, true);
    Strategy::strategy_ = &TheveninChargeStrategy::vtable;
    return doStrategy(theveninScreens, immediately);
}


Strategy::statusType Program::runDeltaCharge(bool immediately = false)
{
    Strategy::strategy_ = &DeltaChargeStrategy::vtable;
    return doStrategy(deltaChargeScreens, immediately);
}

Strategy::statusType Program::runDischarge(bool immediately = false)
{
    AnalogInputs::ValueType Voff = ProgramData::currentProgramData.getVoltage(ProgramData::VDischarge);
    Voff += settings.dischargeOffset_LiXX_ * ProgramData::currentProgramData.battery.cells;
    TheveninDischargeStrategy::setVI(Voff, ProgramData::currentProgramData.battery.Id);
    Strategy::strategy_ = &TheveninDischargeStrategy::vtable;
    return doStrategy(dischargeScreens, immediately);
}

Strategy::statusType Program::runNiXXDischarge(bool immediately = false)
{
    TheveninDischargeStrategy::setVI(ProgramData::currentProgramData.getVoltage(ProgramData::VDischarge), ProgramData::currentProgramData.battery.Id);
    Strategy::strategy_ = &TheveninDischargeStrategy::vtable;
    return doStrategy(NiXXDischargeScreens, immediately);
}

Strategy::statusType Program::runBalance()
{
    Strategy::strategy_ = &Balancer::vtable;
    return doStrategy(balanceScreens);
}

Strategy::statusType Program::runWasteTime()
{    
    DelayStrategy::setDelay(settings.WasteTime_);
    Strategy::strategy_ = &DelayStrategy::vtable;
    return doStrategy(dischargeScreens, true);	
}

Program::ProgramState getProgramState(Program::ProgramType prog)
{
    Program::ProgramState retu;
    switch(prog) {
    case Program::ChargeLiXX:
    case Program::ChargePb:
    case Program::FastChargePb:
    case Program::FastChargeLiXX:
    case Program::ChargeNiXX:
        retu = Program::Charging;
        break;
    case Program::Balance:
        retu = Program::Balancing;
        break;
    case Program::DischargeLiXX:
    case Program::DischargePb:
    case Program::DischargeNiXX:
        retu = Program::Discharging;
        break;
    case Program::DCcycleLiXX:
    case Program::DCcyclePb:
         retu = Program::DischargingCharging;
        break;
    case Program::DCcycleNiXX:
         retu = Program::DischargingCharging;
        break; 
    case Program::StorageLiXX:
    case Program::StorageLiXX_Balance:
        retu = Program::Storage;
        break;
    case Program::ChargeLiXX_Balance:
        retu = Program::ChargingBalancing;
        break;
    default:
        retu = Program::None;
        break;
    }
    return retu;
}

void Program::run(ProgramType prog)
{
    programType_ = prog;
    stopReason_ = PSTR("");

    programState_ = getProgramState(prog);  

    SerialLog::powerOn();
    AnalogInputs::powerOn();


    if(startInfo()) {
        Buzzer::soundStartProgram();

        switch(prog) {
        case Program::ChargeLiXX:
        case Program::ChargePb:
            runTheveninCharge(settings.Lixx_Imin_);   //(default end current = start current / 10)
            break;
        case Program::Balance:
            runBalance();
            break;
        case Program::DischargeLiXX:
        case Program::DischargePb:
            runDischarge();
            break;
        case Program::FastChargeLiXX:
        case Program::FastChargePb:
            runTheveninCharge(5);
            break;
        case Program::StorageLiXX:
            runStorage(false);
            break;
        case Program::StorageLiXX_Balance:
            runStorage(true);
            break;
        case Program::ChargeNiXX:
            runDeltaCharge();
            break;
        case Program::DischargeNiXX:
            runNiXXDischarge();
            break;
           
        case Program::DCcycleLiXX:
              runDCcycle(1);  //1= lixx
              break;
        case Program::DCcycleNiXX:
            runDCcycle(0);   //0 = nixx
            break;
        case Program::DCcyclePb:
            runDCcycle(2);   //2= pb
            break;   
        case Program::ChargeLiXX_Balance:
            runTheveninChargeBalance();
            break;

        default:
            //TODO:
            Screen::runNotImplemented();
            break;
        }
    }
    AnalogInputs::powerOff();
    SerialLog::powerOff();
}


uint8_t Program::currentCycle() { return tempCDcycles_;}

char Program::currentCycleMode() { return cycleMode;}

Strategy::statusType Program::runDCcycle(uint8_t prog1)
{ //TODO_NJ
    Strategy::statusType status;
    
    for(tempCDcycles_=1; tempCDcycles_ <= settings.CDcycles_; tempCDcycles_++) {

          //discharge
          AnalogInputs::resetAccumulatedMeasurements();
          cycleMode='D';
          status = runCycleDischargeCommon(prog1);
          if(status != Strategy::COMPLETE) {break;} 
 
          //waiting after discharge
          cycleMode='W';
          status = runWasteTime();
          if(status != Strategy::COMPLETE) { break; } 
          
          //charge
          Screen::resetETA();
          AnalogInputs::resetAccumulatedMeasurements();
          cycleMode='C';
              
          //lastcharge? (no need wait at end?)
          if (tempCDcycles_ != settings.CDcycles_)
           {
              status = runCycleChargeCommon(prog1, true); //independent exit
           } 
           else 
           {
              status = runCycleChargeCommon(prog1, false);
           }  
           if(status != Strategy::COMPLETE) {break;} 
          
          //waiting after charge
          cycleMode='W';
          if (tempCDcycles_ != settings.CDcycles_)
           {
            status = runWasteTime();
            if(status != Strategy::COMPLETE) { break; }
            else {status = Strategy::COMPLETE;}
          }      
    } 
    return status;
}


Strategy::statusType Program::runCycleDischargeCommon(uint8_t prog1)
{
  Strategy::statusType status;

  if(prog1 == 1)  //1 is lixx
  { 
    status = runDischarge(true);
  }

  if(prog1 == 0)  //0 nixx
  { 
    status = runNiXXDischarge(true);
  }

  if(prog1 == 2)  //2 pb
  { 
    status = runDischarge(true);
  }

  return status;
}

Strategy::statusType Program::runCycleChargeCommon(uint8_t prog1, bool mode)
{
 Strategy::statusType status;
 
 //lastcharge? (no need wait at end?)
          if (tempCDcycles_ != settings.CDcycles_)
           {
              if(prog1 == 1)  //1 is lixx  //0 is nixx
              { if (settings.forceBalancePort_) status =  runTheveninChargeBalance(mode);		//ign
                else status =  runTheveninCharge(settings.Lixx_Imin_, mode); } //independent exit		//ign
             if(prog1 == 0)     //nixx
              { status =  runDeltaCharge(mode); } //independent exit  
             if(prog1 == 2)  //pb
              { status =  runTheveninCharge(settings.Lixx_Imin_, mode); }  //independent exit  
           } 
           else 
           {
              if(prog1 == 1)
              { if (settings.forceBalancePort_) status = runTheveninChargeBalance();		//ign
                else status =  runTheveninCharge(settings.Lixx_Imin_); } //normal exit point		//ign
               if(prog1 == 0)   //nixx
              { status = runDeltaCharge();  } //normal exit point
              if(prog1 == 2)  //pb
              { status =  runTheveninCharge(settings.Lixx_Imin_); }   //normal exit point
           }

 return status;
}


// Program selection depending on the battery type

namespace {

    const char charge_str[] PROGMEM = "charge";
    const char chaBal_str[] PROGMEM = "charge+balance";
    const char balanc_str[] PROGMEM = "balance";
    const char discha_str[] PROGMEM = "discharge";
    const char fastCh_str[] PROGMEM = "fast charge";
    const char storag_str[] PROGMEM = "storage";
    const char stoBal_str[] PROGMEM = "storage+balanc";
    const char dccycl_str[] PROGMEM = "D>C format";       
    const char edBatt_str[] PROGMEM = "edit battery";

    const char * const programLiXXMenu[] PROGMEM =
    { charge_str,
      chaBal_str,
      balanc_str,
      discha_str,
      fastCh_str,
      storag_str,
      stoBal_str,
      dccycl_str,
      edBatt_str,
      NULL
    };

    const Program::ProgramType programLiXXMenuType[] PROGMEM =
    { Program::ChargeLiXX,
      Program::ChargeLiXX_Balance,
      Program::Balance,
      Program::DischargeLiXX,
      Program::FastChargeLiXX,
      Program::StorageLiXX,
      Program::StorageLiXX_Balance,
      Program::DCcycleLiXX,   
      Program::EditBattery
    };

    const char * const programNiZnMenu[] PROGMEM =
    { charge_str,
      chaBal_str,
      balanc_str,
      discha_str,
      fastCh_str,
      dccycl_str,
      edBatt_str,
      NULL
    };

    const Program::ProgramType programNiZnMenuType[] PROGMEM =
    { Program::ChargeLiXX,
      Program::ChargeLiXX_Balance,
      Program::Balance,
      Program::DischargeLiXX,
      Program::FastChargeLiXX,   //TODO: check
      Program::DCcycleLiXX,      //TODO: check
      Program::EditBattery
    };


    const char * const programNiXXMenu[] PROGMEM =
    { charge_str,
      discha_str,
      dccycl_str,
      edBatt_str,
      NULL
    };

    const Program::ProgramType programNiXXMenuType[] PROGMEM =
    { Program::ChargeNiXX,
      Program::DischargeNiXX,
      Program::DCcycleNiXX,
      Program::EditBattery
    };

    const char * const programPbMenu[] PROGMEM =
    { charge_str,
      discha_str,
      fastCh_str,
      dccycl_str,
      edBatt_str,
      NULL
    };

    const Program::ProgramType programPbMenuType[] PROGMEM =
    { Program::ChargePb,
      Program::DischargePb,
      Program::FastChargePb,   //TODO: check
      Program::DCcyclePb,      //TODO: check
      Program::EditBattery
    };


    StaticMenu selectLiXXMenu(programLiXXMenu);
    StaticMenu selectNiXXMenu(programNiXXMenu);
    StaticMenu selectNiZnMenu(programNiZnMenu);
    StaticMenu selectPbMenu(programPbMenu);

    StaticMenu * getSelectProgramMenu() {
        if(ProgramData::currentProgramData.battery.type == ProgramData::NiZn)
            return &selectNiZnMenu;
        if(ProgramData::currentProgramData.isLiXX())
            return &selectLiXXMenu;
        else if(ProgramData::currentProgramData.isNiXX())
            return &selectNiXXMenu;
        else return &selectPbMenu;
    }
    Program::ProgramType getSelectProgramType(uint8_t index) {
        const Program::ProgramType * address;
        if(ProgramData::currentProgramData.battery.type == ProgramData::NiZn)
            address = &programNiZnMenuType[index];
        else if(ProgramData::currentProgramData.isLiXX())
            address = &programLiXXMenuType[index];
        else if(ProgramData::currentProgramData.isNiXX())
            address = &programNiXXMenuType[index];
        else address = &programPbMenuType[index];
        return pgm::read(address);
    }
}

void Program::selectProgram(int index)
{
    ProgramData::loadProgramData(index);
    StaticMenu * selectPrograms = getSelectProgramMenu();
    int8_t menuIndex;
    do {
        menuIndex = selectPrograms->runSimple();
        if(menuIndex >= 0)  {
            Program::ProgramType prog = getSelectProgramType(menuIndex);
            if(prog == Program::EditBattery) {
                ProgramData::currentProgramData.edit(index);
                ProgramData::saveProgramData(index);
                selectPrograms = getSelectProgramMenu();
            } else {
                Program::run(prog);
            }
        }
    } while(menuIndex >= 0);
}




