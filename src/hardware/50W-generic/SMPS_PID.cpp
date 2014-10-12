#include "Hardware.h"
#include "imaxB6-pins.h"
#include "SMPS_PID.h"
#include "IO.h"
#include "AnalogInputs.h"
#include "atomic.h"

#include "Utils.h"			//ign
#include "Balancer.h"		//ign
#include "TheveninMethod.h"		//ign

//#include "SerialLog.h"		//ign

namespace {
    volatile uint16_t i_PID_setpoint;
    volatile uint16_t i_PID_CutOff;
    volatile uint16_t voltage;
    volatile long i_PID_MV;
    volatile bool i_PID_enable;
}

#define A 4
//long error;
uint16_t cnt = 0;			//ign
int16_t softstart;				//ign
uint16_t Vcell_err = 0;		//ign
uint16_t Vcell_err_OFF = 0;		//ign
uint8_t balance_ = 0;		//ign

AnalogInputs::ValueType Vend_per_cell;		//ign
AnalogInputs::ValueType Vcell_;		//ign
uint8_t cells_;				//ign

AnalogInputs::ValueType Vend_Vb[8]; // Vcell[8];		//ign


uint16_t hardware::getPIDValue()
{
//    return PID_setpoint;
    uint16_t v;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        v = i_PID_MV>>PID_MV_PRECISION;
    }
    return v;
}


void SMPS_PID::update()
{
    if(!i_PID_enable && IO::digitalRead(DISCHARGE_DISABLE_PIN)) return;
    //if Vout is too high disable PID
    if(AnalogInputs::getADCValue(AnalogInputs::Vout_plus_pin) > i_PID_CutOff) {
        hardware::setChargerOutput(false);
        i_PID_enable = false;
        return;
    }

long error;

#ifndef FAST_FEEDBACK  
cnt++;
if(cnt > 349) {
//if(Vcell_ != AnalogInputs::getVout() || cnt > 700) {
cnt = 0;
Vcell_ = 0;

if (Balancer::balance_ > balance_) {balance_ = Balancer::balance_; Vcell_err_OFF = Vcell_err; softstart = 1;}
if (Balancer::balance_ < balance_) {balance_ = Balancer::balance_; Vcell_err = Vcell_err_OFF; softstart = i_PID_setpoint / 16;}

// SerialLog::printString("SMPS_PID::update "); //SerialLog::printUInt(); SerialLog::printD(); SerialLog::printUInt();		//ign

   	for(uint8_t i = 0; i < cells_; i++) {
		int16_t cell_error = AnalogInputs::getRealValue(AnalogInputs::Name(AnalogInputs::Vb1+i));
		if(IO::digitalRead(DISCHARGE_DISABLE_PIN)) {
			cell_error -= Vend_per_cell;
		}
		else {
			cell_error = Vend_per_cell - cell_error;
		}
		if (cell_error > 0) Vcell_ += cell_error;
	//	if (cell_error > 0) {Vcell_ += cell_error; SerialLog::printUInt(cell_error); SerialLog::printD();}
	//	else {SerialLog::printUInt(0); SerialLog::printD();}
	}
	if(Vcell_) Vcell_err += Vcell_;		
	else if(Vcell_err) Vcell_err--;		

// SerialLog::printUInt(Vcell_err);
// SerialLog::printNL();	//ign
// SerialLog::printUInt(Balancer::balance_);
// SerialLog::printNL();	//ign
 
//Vcell_ = AnalogInputs::getVout();
}
#endif

#ifdef FAST_FEEDBACK
   	Vcell_err = 0;
   	for(uint8_t i = 0; i < cells_; i++) {
		error = AnalogInputs::getADCValue(AnalogInputs::Name(AnalogInputs::Vb1_pin+i));
		if (i == 0) error -= AnalogInputs::getADCValue(AnalogInputs::Vb0_pin);
		if (i == 1) {error -= AnalogInputs::getADCValue(AnalogInputs::Vb1_pin)/2; error += AnalogInputs::getADCValue(AnalogInputs::Vb0_pin)/2;}
		if(IO::digitalRead(DISCHARGE_DISABLE_PIN)) {
			error -= Vend_Vb[i];
		}
		else {
			error = Vend_Vb[i] - error;
		}
		error += i_PID_setpoint;
		if (error > Vcell_err) Vcell_err = error;	
	}
#endif

	uint16_t PV;
    if(IO::digitalRead(DISCHARGE_DISABLE_PIN)) PV = AnalogInputs::getADCValue(AnalogInputs::Ismps);
    else PV = AnalogInputs::getADCValue(AnalogInputs::Idischarge);
	
	error = AnalogInputs::getADCValue(AnalogInputs::Vout_plus_pin);		//ign
	error -= AnalogInputs::getADCValue(AnalogInputs::Vout_minus_pin);

#ifndef FAST_FEEDBACK	
	if(IO::digitalRead(DISCHARGE_DISABLE_PIN)) {error -= voltage; error += Vcell_err;}
	else {error = voltage - error; error -= Vcell_err;}
	error += i_PID_setpoint;
#endif
	
#ifdef FAST_FEEDBACK	
	if(IO::digitalRead(DISCHARGE_DISABLE_PIN)) error -= voltage;
	else error = voltage - error;
	error += i_PID_setpoint;
	if (Vcell_err > error) error = Vcell_err;
#endif
	

if(error > PV) PV = error;

    error = i_PID_setpoint;
    error -= PV;
	
    if(error > softstart) error = softstart;	//ign	SOFT START
/*     if(balance_) {
		if(error > 1) error = 1;
	}
	else if(error > softstart) error = softstart; */
	
	if(i_PID_enable) {
		i_PID_MV += error*A;
		if(i_PID_MV < 0) i_PID_MV = 0;
		if(i_PID_MV > MAX_PID_MV_PRECISION) i_PID_MV = MAX_PID_MV_PRECISION;
		SMPS_PID::setPID_MV(i_PID_MV>>PID_MV_PRECISION);
	}
	else if(!IO::digitalRead(DISCHARGE_DISABLE_PIN)) {
		i_PID_MV += error/8;
		if(i_PID_MV < 0) i_PID_MV = 0;
		if(i_PID_MV > i_PID_setpoint) i_PID_MV = i_PID_setpoint;		//ign   hm...
		hardware::setDischarge(i_PID_MV);
	}
}

void SMPS_PID::init(uint16_t Vin, uint16_t Vout)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        i_PID_setpoint = 0;
        if(Vout>Vin) {
            i_PID_MV = TIMER1_PRECISION_PERIOD;
        } else {
            i_PID_MV = 0;
        }
        i_PID_MV <<= PID_MV_PRECISION;
        i_PID_enable = true;
    }
}

namespace {
    void enableChargerBuck() {
        Timer1::disablePWM(SMPS_VALUE_BUCK_PIN);
        IO::digitalWrite(SMPS_VALUE_BUCK_PIN, 1);
    }
    void disableChargerBuck() {
        Timer1::disablePWM(SMPS_VALUE_BUCK_PIN);
        IO::digitalWrite(SMPS_VALUE_BUCK_PIN, 0);
    }
    void disableChargerBoost() {
        Timer1::disablePWM(SMPS_VALUE_BOOST_PIN);
        IO::digitalWrite(SMPS_VALUE_BOOST_PIN, 0);
    }
}

void SMPS_PID::setPID_MV(uint16_t value) {
    if(value > MAX_PID_MV)
        value = MAX_PID_MV;

    if(value <= TIMER1_PRECISION_PERIOD) {
        disableChargerBoost();
        Timer1::setPWM(SMPS_VALUE_BUCK_PIN, value);
    } else {
        enableChargerBuck();
        uint16_t v2 = value - TIMER1_PRECISION_PERIOD;
        Timer1::setPWM(SMPS_VALUE_BOOST_PIN, v2);
    }
}

void hardware::setChargerValue(uint16_t curr, uint16_t volt)
{
    AnalogInputs::ValueType cutoff = AnalogInputs::reverseCalibrateValue(AnalogInputs::Vout_plus_pin, PID_CUTOFF_VOLTAGE);

// SerialLog::printString("h::sCV "); SerialLog::printUInt(AnalogInputs::getADCValue(AnalogInputs::Vout_plus_pin)); SerialLog::printD(); SerialLog::printUInt(AnalogInputs::reverseCalibrateValue(AnalogInputs::Vout_plus_pin, TheveninMethod::Vend_));		//ign
// SerialLog::printNL();	//ign
	softstart = curr / 16;
	Vend_per_cell = Balancer::calculatePerCell(TheveninMethod::Vend_);
	cells_ = AnalogInputs::getRealValue(AnalogInputs::VbalanceInfo);

#ifdef FAST_FEEDBACK
  	for(uint8_t i = 0; i < cells_; i++) {
		Vend_Vb[i] = AnalogInputs::reverseCalibrateValue(AnalogInputs::Name(AnalogInputs::Vb1_pin+i), Vend_per_cell);
//Vcell[i] = AnalogInputs::getAvrADCValue(AnalogInputs::Name(AnalogInputs::Vb1_pin+i));
//SerialLog::printUInt(i); SerialLog::printD(); SerialLog::printUInt(Vcell[i]); SerialLog::printD(); SerialLog::printUInt(Vend_Vb[i]); SerialLog::printD(); SerialLog::printUInt(AnalogInputs::calibrateValue(AnalogInputs::Name(AnalogInputs::Vb1_pin+i), Vend_Vb[i])); SerialLog::printNL();
	}
#endif
 
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        i_PID_setpoint = curr;
        i_PID_CutOff = cutoff;
		voltage = volt;		//ign
	}
}

/* void hardware::setDischargerValue(uint16_t curr, uint16_t volt)
{
	setChargerValue(curr, volt);
//	i_PID_setpoint = curr;
//	voltage = volt;
	//Timer1::setPWM(DISCHARGE_VALUE_PIN, curr);
} */

void hardware::setChargerOutput(bool enable)
{
    if(enable) setDischargerOutput(false);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        disableChargerBuck();
        disableChargerBoost();
        i_PID_enable = false;
    }
    IO::digitalWrite(SMPS_DISABLE_PIN, !enable);
    if(enable) {
        SMPS_PID::init(AnalogInputs::getRealValue(AnalogInputs::Vin), AnalogInputs::getRealValue(AnalogInputs::Vout_plus_pin));
    }
Vcell_err = 0;
#ifndef FAST_FEEDBACK
Vcell_err_OFF = 0;
balance_ = 0;
#endif
}


void hardware::setDischargerOutput(bool enable)
{
    if(enable) setChargerOutput(false);
    IO::digitalWrite(DISCHARGE_DISABLE_PIN, !enable);
Vcell_err = 0;
}

void hardware::setDischarge(uint16_t curr)
{
    Timer1::setPWM(DISCHARGE_VALUE_PIN, curr);
}

