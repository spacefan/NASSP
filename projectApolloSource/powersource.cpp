/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2005 Mark Grant

  ORBITER vessel module: Power distribution code

  Project Apollo is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Project Apollo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Project Apollo; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  See http://nassp.sourceforge.net/license/ for more details.

  **************************** Revision History ****************************
  *	$Log$
  *	Revision 1.3  2005/11/17 19:19:12  movieman523
  *	Added three-phase AC bus and battery buses.
  *	
  *	Revision 1.2  2005/11/17 01:52:29  movieman523
  *	Simplified setup for circuit breakers, and added battery buses.
  *	
  *	Revision 1.1  2005/11/16 23:14:02  movieman523
  *	Initial support for wiring in the circuit breakers.
  *	
  **************************************************************************/


#include <stdio.h>
#include "orbitersdk.h"

#include "PanelSDK/PanelSDK.h"
#include "PanelSDK/Internals/Esystems.h"

#include "powersource.h"

PowerSource::PowerSource()

{
	next_source = 0;
}

PowerSource::~PowerSource()

{
	// Nothing for now.
}

//
// Default to passing calls on.
//

double PowerSource::Voltage()

{
	if (next_source)
		return next_source->Voltage();

	return 0.0;
}

void PowerSource::DrawPower(double watts)

{
	if (next_source)
		next_source->DrawPower(watts);
}

double PowerBreaker::Voltage()

{
	if (!IsOpen() && next_source)
		return next_source->Voltage();

	return 0.0;
}

double PowerSDKObject::Voltage()

{
	if (SDKObj)
		return SDKObj->Volts;

	return 0.0;
}

void PowerSDKObject::DrawPower(double watts)

{
	//
	// Nothing for now.
	//
}

//
// Tie power together from two sources. For now we just take the
// largest voltage from both sources.
//

double PowerMerge::Voltage()

{
	double VoltsA = 0.0, VoltsB = 0.0;

	if (BusA)
		VoltsA = BusA->Voltage();
	if (BusB)
		VoltsB = BusB->Voltage();

	if (VoltsA > VoltsB)
		return VoltsA;

	return VoltsB;
}

void PowerMerge::DrawPower(double watts)

{
	double Volts = 0.0;
	double VoltsA = 0.0;
	double VoltsB = 0.0;

	if (BusA)
		VoltsA = BusA->Voltage();
	if (BusB)
		VoltsB = BusB->Voltage();

	Volts = VoltsA + VoltsB;

	if (Volts > 0.0) {
		if (BusA)
			BusA->DrawPower(watts * VoltsA / Volts);
		if (BusB)
			BusB->DrawPower(watts * VoltsB / Volts);
	}
}

//
// Tie power together from three sources. For now we just take the
// largest voltage from all sources.
//

double ThreeWayPowerMerge::Voltage()

{
	double Volts1 = 0.0, Volts2 = 0.0, Volts3 = 0.0;
	double MaxVolts;

	if (Phase1)
		Volts1 = Phase1->Voltage();
	if (Phase2)
		Volts2 = Phase2->Voltage();
	if (Phase3)
		Volts3 = Phase2->Voltage();

	MaxVolts = Volts1;

	if (Volts2 > Volts1)
		MaxVolts = Volts2;
	if (Volts3 > MaxVolts)
		MaxVolts = Volts3;

	return MaxVolts;
}

void ThreeWayPowerMerge::DrawPower(double watts)

{
	double Volts = 0.0;
	double VoltsA = 0.0;
	double VoltsB = 0.0;
	double VoltsC = 0.0;

	if (Phase1)
		VoltsA = Phase1->Voltage();
	if (Phase2)
		VoltsB = Phase2->Voltage();
	if (Phase3)
		VoltsC = Phase3->Voltage();

	Volts = VoltsA + VoltsB + VoltsC;

	if (Volts > 0.0) {
		if (Phase1)
			Phase1->DrawPower(watts * VoltsA / Volts);
		if (Phase2)
			Phase2->DrawPower(watts * VoltsB / Volts);
		if (Phase3)
			Phase3->DrawPower(watts * VoltsC / Volts);
	}
}
