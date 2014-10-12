/*
    cheali-charger - open sourc`e firmware for a variety of LiPo chargers
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
#include "Utils.h"
#include "Settings.h"

#include "IO.h"

//#include "SerialLog.h"		//ign


namespace TheveninMethod {
    uint16_t minValue_;
    uint16_t minBalanceValue_;
    uint16_t maxValue_;
    AnalogInputs::ValueType Vend_;
    AnalogInputs::ValueType valueTh_;
    uint16_t lastBallancingEnded_;
	
	uint16_t Vcell_err;		//ign

    Thevenin tVout_;
    Thevenin tBal_[MAX_BANANCE_CELLS];
    FallingState Ifalling_;
    uint8_t fullCount_;
    uint8_t cells_;
    AnalogInputs::Name iName_;
    bool balance_;
    Strategy::statusType bstatus_;
    AnalogInputs::ValueType idebug_;


 void setMinI(AnalogInputs::ValueType i)
    {
     if (i < 50)
        {
            minValue_ = 50;
        }
        else
        {
            minValue_ = i;
        }
    }

    uint16_t getMinValueB() {
        if(bstatus_ != Strategy::COMPLETE)
            return 0;
        else return minValue_;
    }

    AnalogInputs::ValueType getImax()
    {
        return AnalogInputs::calibrateValue(iName_, maxValue_);
    }

    bool isBelowMin(AnalogInputs::ValueType value)
    {
//		if(Ifalling_ == LastRthMesurment)
//			return false;
        return value < minValue_;
    }

}

AnalogInputs::ValueType TheveninMethod::getReadableRthCell(uint8_t cell)
{
    return tBal_[cell].Rth_.getReadableRth_calibrateI(iName_);
//      return tBal_[cell].ILastDiff_;
}
AnalogInputs::ValueType TheveninMethod::getReadableBattRth()
{
    return tVout_.Rth_.getReadableRth_calibrateI(iName_);
}

AnalogInputs::ValueType TheveninMethod::getReadableWiresRth()
{
    Resistance R;
    R.iV_ =  AnalogInputs::getRealValue(AnalogInputs::Vout);
    R.iV_ -= AnalogInputs::getRealValue(AnalogInputs::Vbalancer);
    R.uI_ = AnalogInputs::getRealValue(AnalogInputs::Iout);
    return R.getReadableRth();

}



void TheveninMethod::setVIB(AnalogInputs::ValueType Vend, AnalogInputs::ValueType i, bool balance)
{
//SerialLog::printString("TM::setVIB "); SerialLog::printUInt(Vend); SerialLog::printD(); SerialLog::printUInt(i);  //ign
//SerialLog::printNL();  //ign

    if (Vend != 1) {     //ign
    Vend_ = Vend;
    balance_ = balance;
    }                    //ign
    maxValue_ = i;
    minValue_ = i;
    minValue_ /= settings.Lixx_Imin_;    //default=10
        //low current
    if (maxValue_ < 50) { maxValue_ = 50; }
    if (minValue_ < 50) { minValue_ = 50; }
}

void TheveninMethod::initialize(AnalogInputs::Name iName)
{
    bstatus_ = Strategy::COMPLETE;
    bool charge = (iName == AnalogInputs::IsmpsValue);

    iName_ = iName;
//    minBalanceValue_ = AnalogInputs::reverseCalibrateValue(iName_, IBALANCE);		//ign
    AnalogInputs::ValueType Vout = AnalogInputs::getVout();
    tVout_.init(Vout, Vend_, minValue_, charge);

    cells_ = Balancer::getCells();
    AnalogInputs::ValueType Vend_per_cell = Balancer::calculatePerCell(Vend_);

    for(uint8_t c = 0; c < cells_; c++) {
        AnalogInputs::ValueType v = Balancer::getPresumedV(c);
        tBal_[c].init(v, Vend_per_cell, minValue_, charge);
    }

//    Ifalling_ = NotFalling;
    fullCount_ = 0;
//	Vcell_err = 0;
}

//TODO: the TheveninMethod  is too complex, should be refactored, maybe when switching to mAmps


bool TheveninMethod::isComlete(bool isEndVout, AnalogInputs::ValueType value)
{
// SerialLog::printString("TM::isComlete "); SerialLog::printUInt(isEndVout); SerialLog::printD(); SerialLog::printUInt(value); SerialLog::printD(); SerialLog::printUInt(minValue_); SerialLog::printD(); SerialLog::printUInt(getMinValueB());
// SerialLog::printNL();

    if(balance_) {
        //if(value > max(minBalanceValue_, minValue_))
        if(value > max(IBALANCE, minValue_))
            Balancer::done_ = false;
//        if(Ifalling_ != LastRthMesurment)				//ign
            bstatus_ = Balancer::doStrategy();
	}

    if(bstatus_ != Strategy::COMPLETE)
        return false;

//    isEndVout |= (Ifalling_ == Falling)  && value < minValue_;
//	if (value < minValue_) isEndVout = true;					//ign

    if(value <= getMinValueB() && isEndVout) {
        if(fullCount_++ >= 10) {
            return true;
        }
    }
	else {
        fullCount_ = 0;
    }
    return false;
}


AnalogInputs::ValueType TheveninMethod::calculateNewValue(bool isEndVout, AnalogInputs::ValueType oldValue)
{

    calculateRthVth(oldValue);
    storeOldValue(oldValue);
	
/*     AnalogInputs::ValueType Verr = 0;
	AnalogInputs::ValueType Vend_per_cell = Balancer::calculatePerCell(Vend_);
  	for(uint8_t i = 0; i < cells_; i++) {
		int16_t error = Balancer::getV(i);
		if(!IO::digitalRead(SMPS_DISABLE_PIN)) {
			error -= Vend_per_cell;
		}
		else {
			error = Vend_per_cell - error;
		}
		if (error > 0) Verr += error;	
	}
	if(Verr > 1) Vcell_err+= Verr;
	else if(Vcell_err)  Vcell_err -= 2;

	if(!IO::digitalRead(SMPS_DISABLE_PIN)) {
	//	if(isEndVout && balance_) {
	//		Balancer::endBalancing();
	//		Balancer::done_ = false;
	//	}

    return Vend_ - Vcell_err;
	}
	else return Vend_ + Vcell_err;
 */
    return Vend_;
}


void TheveninMethod::calculateRthVth(AnalogInputs::ValueType oldValue)
{
    tVout_.calculateRthVth(AnalogInputs::getVout(),oldValue);

    for(uint8_t c = 0; c < cells_; c++) {
        tBal_[c].calculateRthVth(Balancer::getPresumedV(c),oldValue);
    }
}


void TheveninMethod::storeOldValue(AnalogInputs::ValueType oldValue)
{
    tVout_.storeLast(AnalogInputs::getVout(), oldValue);

    for(uint8_t i = 0; i < cells_; i++) {
        AnalogInputs::ValueType vi = Balancer::getPresumedV(i);
        tBal_[i].storeLast(vi, oldValue);
    }
}




