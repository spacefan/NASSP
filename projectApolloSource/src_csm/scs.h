/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2005

  Stability & Control Systems definitions.
  The implementation is in satsystems.cpp

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
  *	Revision 1.28  2009/01/18 17:48:18  tschachim
  *	Bugfix for docked vessels, GetForceVector isn't working in this case.
  *	
  *	Revision 1.27  2008/04/11 12:19:19  tschachim
  *	New SM and CM RCS.
  *	Improved abort handling.
  *	Fixed BasicExcel for VC6, reduced VS2005 warnings, bugfixes.
  *	
  *	Revision 1.26  2008/01/18 02:59:22  jasonims
  *	EMS Implementation Step 5b - Initialization bugfix!
  *	
  *	Revision 1.25  2007/12/21 09:38:43  jasonims
  *	EMS Implementation Step 5 - jasonims :   SCROLL: complete:  possible inaccuracy in velocity integration.   RSI: complete:  accurate readings, but unsure of authenticity of method, (lack of documentation found).    THRESHOLD CIRCUITRY:  complete: seems quite accurate.   CORRIDOR VERIFICATION: complete: needs testing, but appears to be correct.    RANGE:  working but inaccurate:  probable inaccuracy in range integration....unknown cause.   EMS State is saved as well.    Testing must insue.
  *	
  *	Revision 1.24  2007/12/05 19:23:30  jasonims
  *	EMS Implementation Step 4 - jasonims :   RSI is set up to rotate, but no actual controlling of it is done.
  *	
  *	Revision 1.23  2007/12/05 07:13:13  jasonims
  *	EMS Implementation Step 3b - jasonims :   EMS Scroll disappearance bug fixed.  No further implementation.
  *	
  *	Revision 1.22  2007/11/27 02:56:42  jasonims
  *	EMS Implementation Step 3 - jasonims :   EMS Scroll is functional and plots correctly, however .05G circuitry does not work yet and is commented out.  Manual  operation does work though.  Verification needed.
  *	
  *	Revision 1.21  2007/11/25 09:07:25  jasonims
  *	EMS Implementation Step 2 - jasonims :   EMS Scroll can slew, and some functionality set up for EMS.
  *	
  *	Revision 1.20  2007/11/24 21:28:46  jasonims
  *	EMS Implementation Step 1 - jasonims :   EMSdVSet Switch now works, preliminary EMS Scroll work being done.
  *	
  *	Revision 1.19  2007/10/18 00:23:24  movieman523
  *	Primarily doxygen changes; minimal functional change.
  *	
  *	Revision 1.18  2007/04/25 18:48:11  tschachim
  *	EMS dV functions.
  *	
  *	Revision 1.17  2007/01/22 15:48:18  tschachim
  *	SPS Thrust Vector Control, RHC power supply, THC clockwise switch, bugfixes.
  *	
  *	Revision 1.16  2007/01/20 02:09:51  dseagrav
  *	Tweaked RCS positions
  *	
  *	Revision 1.15  2006/12/19 15:56:11  tschachim
  *	ECS test stuff, bugfixes.
  *	
  *	Revision 1.14  2006/11/13 14:47:34  tschachim
  *	New SPS engine.
  *	New ProjectApolloConfigurator.
  *	Fixed and changed camera and FOV handling.
  *	
  *	Revision 1.13  2006/10/05 16:09:02  tschachim
  *	Fixed SCS attitude hold mode.
  *	
  *	Revision 1.12  2006/06/17 18:18:00  tschachim
  *	Bugfixes SCS automatic modes,
  *	Changed quickstart separation key to J.
  *	
  *	Revision 1.11  2006/06/07 09:53:20  tschachim
  *	Improved ASCP and GDC align button, added cabin closeout sound, bugfixes.
  *	
  *	Revision 1.10  2006/05/30 14:40:21  tschachim
  *	Fixed fuel cell - dc bus connectivity, added battery charger
  *	
  *	Revision 1.9  2006/05/19 13:48:28  tschachim
  *	Fixed a lot of devices and power consumptions.
  *	DirectO2 valve added.
  *	
  **************************************************************************/

class Saturn;

class AttitudeReference {

public:
	AttitudeReference();
	virtual void Init(VESSEL *v);
	virtual void Timestep(double simdt);
	virtual void SaveState(FILEHANDLE scn); 
	virtual void LoadState(char *line);
	VECTOR3 GetAttitude() { return Attitude; };
	VECTOR3 GetLastAttitude() { return LastAttitude; };
	void SetAttitude(VECTOR3 a);

	static MATRIX3 GetRotationMatrixX(double angle);
	static MATRIX3 GetRotationMatrixY(double angle);
	static MATRIX3 GetRotationMatrixZ(double angle);

protected:
	bool AttitudeInitialized;
	VESSEL *Vessel;
	VECTOR3 Attitude;
	VECTOR3 LastAttitude;

	struct {					// Orbiter's state
		VECTOR3 Attitude;
		MATRIX3 AttitudeReference;
	} OrbiterAttitude;

	void SetOrbiterAttitudeReference();
	MATRIX3 GetNavigationBaseToOrbiterLocalTransformation();
	MATRIX3 GetOrbiterLocalToNavigationBaseTransformation();
	VECTOR3 GetRotationAnglesXZY(MATRIX3 m);
};

// Body-Mounted Attitude Gyro

#define BMAG1_START_STRING	"BMAG1_BEGIN"
#define BMAG2_START_STRING	"BMAG2_BEGIN"
#define BMAG_END_STRING	    "BMAG_END"

class BMAG: public AttitudeReference {
	
public: 
	BMAG();                                                                  // Cons
	void Init(Saturn *v, e_object *dcbus, e_object *acbus, Boiler *h);		 // Initialization
	void Timestep(double simdt);                                             // Update function
	void SystemTimestep(double simdt);
	void Cage(int axis);
	void Uncage(int axis);
	VECTOR3 GetAttitudeError();												 // Attitude error when uncaged
	VECTOR3 GetRates() { return rates; };
	VECTOR3 IsUncaged() { return uncaged; };
	Boiler *GetHeater() { return heater; };
	bool IsPowered() { return powered; };
	void SetPower(bool dc, bool ac);
	void SaveState(FILEHANDLE scn); 
	void LoadState(FILEHANDLE scn); 

protected:
	e_object *dc_source;                                                     // DC source for gyro heater
	e_object *ac_source;													 // 3-Phase AC source for gyro
	e_object *dc_bus;					  	          					     // DC source to use when powered
	e_object *ac_bus;														 // 3-Phase AC source to use when powered
	Boiler *heater;															 // Heat coldplates when powered
	int temperature;                                                         // Temperature
	VECTOR3 rates;                                                           // Detected rotation acceleration
	VECTOR3 uncaged;														 // 0 = caged, 1 = not caged (each axis)
	VECTOR3 targetAttitude;													 // Attitude when uncaged
	Saturn *sat;                                                             // Pointer to ship we're attached to
	bool powered;                                                            // Data valid flag.
};


// Gyro Display Coupler

#define GDC_START_STRING	"GDC_BEGIN"
#define GDC_END_STRING		"GDC_END"

class GDC: public AttitudeReference {
	
public: // We use these inside a timestep, so everything is public to make data access as fast as possible.
	GDC();                          // Cons
	void Init(Saturn *v);		    // Initialization
	void Timestep(double simt);     // TimeStep
	void SystemTimestep(double simdt);
	bool AlignGDC();                // Alignment Switch Pressed
	void SaveState(FILEHANDLE scn); // SaveState callback
	void LoadState(FILEHANDLE scn); // LoadState callback

	VECTOR3 rates;					// Integrated Euler rotation rates
	Saturn *sat;
	// FDAI error needle data from CMC
	int fdai_err_ena;
	int fdai_err_x;
	int fdai_err_y;
	int fdai_err_z;
	friend class CSMcomputer; // Needs to write FDAI error indications, which are really not on the GDC, but meh.
};

#define ASCP_START_STRING	"ASCP_BEGIN"
#define ASCP_END_STRING		"ASCP_END"

class ASCP {
	// Attitude Set Control Panel
public: // We use these inside a timestep, so everything is public to make data access as fast as possible.
	ASCP(Sound &clicksound);									   // Cons
	void Init(Saturn *vessel);									   // Initialization
	void TimeStep(double simdt);						           // Timestep
	bool PaintRollDisplay(SURFHANDLE surf, SURFHANDLE digits);     // Update panel image
	bool PaintPitchDisplay(SURFHANDLE surf, SURFHANDLE digits);    // Update panel image
	bool PaintYawDisplay(SURFHANDLE surf, SURFHANDLE digits);      // Update panel image
	void PaintRoll(SURFHANDLE surf, SURFHANDLE wheel);
	void PaintPitch(SURFHANDLE surf, SURFHANDLE wheel);
	void PaintYaw(SURFHANDLE surf, SURFHANDLE wheel);
	bool RollDisplayClicked();									   // Clicked
	bool PitchDisplayClicked();									   // Clicked
	bool YawDisplayClicked();									   // Clicked
	bool RollUpClick(int Event);								   // Clicked
	bool RollDnClick(int Event);
	bool PitchUpClick(int Event);								   // Clicked
	bool PitchDnClick(int Event);
	bool YawUpClick(int Event);									   // Clicked
	bool YawDnClick(int Event);
	bool RollClick(int Event, int mx, int my);
	bool PitchClick(int Event, int mx, int my);
	bool YawClick(int Event, int mx, int my);
	void SaveState(FILEHANDLE scn);                                // SaveState callback
	void LoadState(FILEHANDLE scn);                                // LoadState callback

	int mousedowncounter;                                          // Mouse Down Counter
	int mousedownposition;
	double mousedownangle;

	VECTOR3 output;												   // Output attitude
	Saturn *sat;												   // The spacecraft
	int msgcounter;
	double rolldisplay;
	double pitchdisplay;
	double yawdisplay;
	Sound &ClickSound;
};

class EDA {
	// Electronic Display Assembly
	// This really just serves as a placeholder right now, and does some of the FDAI sourcing
	// so the FDAI redraw is less messy.
public: // Same stuff about speed and I'm lazy too.
	EDA();															// Cons
	void Init(Saturn *vessel);										// Initialization
	VECTOR3 ReturnCMCErrorNeedles();								// Return said data.
	VECTOR3 ReturnASCPError(VECTOR3 attitude);						// Return said data.
	VECTOR3 ReturnBMAG1Error();										// See the general theme here?
	VECTOR3 AdjustErrorsForRoll(VECTOR3 attitude, VECTOR3 errors);  // Adjust errors for roll so as to be FLY-TO
	VECTOR3 CalcErrors(VECTOR3 target);

	Saturn *sat;
};


#define RJEC_START_STRING	"RJEC_BEGIN"
#define RJEC_END_STRING		"RJEC_END"

class RJEC {
	// Reaction Jet Engine Control
public: // Same stuff about speed and I'm lazy too.
	RJEC();															// Cons
	void Init(Saturn *vessel);										// Initialization
	void TimeStep(double simdt);                                    // Timestep
	void SystemTimestep(double simdt);

	bool GetThruster(int thruster);
	void SetThruster(int thruster,bool Active);                     // Set Thruster Level for CMC
	
	bool GetCMTransferMotor1() { return CMTransferMotor1; };
	bool GetCMTransferMotor2() { return CMTransferMotor2; };
	void ActivateCMTransferMotor1() { CMTransferMotor1 = true; }
	void ActivateCMTransferMotor2() { CMTransferMotor2 = true; }
	
	void SetAutoRCSEnableRelayA(bool active) { AutoRCSEnableRelayA = active; }
	void SetAutoRCSEnableRelayB(bool active) { AutoRCSEnableRelayB = active; }

	bool GetSPSActive() { return SPSActive; }
	void SetSPSActive(bool active) { SPSActive = active; }
	
	bool GetDirectPitchActive() { return DirectPitchActive; }
	bool GetDirectYawActive()   { return DirectYawActive; }
	bool GetDirectRollActive()  { return DirectRollActive; }
	void SetDirectPitchActive(bool active) { DirectPitchActive = active; }
	void SetDirectYawActive(bool active)   { DirectYawActive = active; }
	void SetDirectRollActive(bool active)  { DirectRollActive = active; }
	
	void SetAGCActiveTimer(double timer) { AGCActiveTimer = timer; }

	void SaveState(FILEHANDLE scn);                                // SaveState callback
	void LoadState(FILEHANDLE scn);                                // LoadState callback

protected:
	bool ThrusterDemand[20];                                        // Set when this thruster is requested to fire
	bool AutoRCSEnableRelayA, AutoRCSEnableRelayB;					// Enable relays
	bool CMTransferMotor1, CMTransferMotor2;						// CM/SM transfer motor switches 
	bool SPSActive;                                                 // SPS Active notification
	bool DirectPitchActive, DirectYawActive, DirectRollActive;      // Direct axis fire notification
	double AGCActiveTimer;											/// \todo Dirty Hack for the AGC++ attitude control

	Saturn *sat;
	ThreePosSwitch *PoweredSwitch[20];                              // Set when power is drawn from this switch

	void SetRCSState(int thruster, bool cm, int smquad, int smthruster, int cmthruster, ThreePosSwitch *s, bool lockout);
	bool IsThrusterPowered(ThreePosSwitch *s);
};

class ECA {
	// Electronic Control Assembly
public:
	ECA();
	void Init(Saturn *vessel);										// Initialization
	void TimeStep(double simdt);                                    // Timestep
	void SystemTimestep(double simdt);
	bool IsPowered();

	long rhc_x,rhc_y,rhc_z;											// RHC position
	long rhc_ac_x,rhc_ac_y,rhc_ac_z;								// RHC AC powered position
	long thc_x,thc_y,thc_z;											// THC position
	int accel_roll_trigger;                                         // Joystick triggered roll thrust in ACCEL CMD mode
	int mnimp_roll_trigger;                                         // Joystick triggered roll thrust in MIN IMP mode
	int accel_pitch_trigger;                                        // Joystick triggered pitch thrust in ACCEL CMD mode
	int mnimp_pitch_trigger;                                        // Joystick triggered pitch thrust in MIN IMP mode
	int accel_yaw_trigger;                                          // Joystick triggered yaw thrust in ACCEL CMD mode
	int mnimp_yaw_trigger;                                          // Joystick triggered yaw thrust in MIN IMP mode
	int trans_x_trigger;                                            // Translation triggers
	int trans_y_trigger;
	int trans_z_trigger;
	Saturn *sat;
	VECTOR3 pseudorate;
};

#define EMS_START_STRING	"EMS_BEGIN"
#define EMS_END_STRING		"EMS_END"
#define EMS_SCROLL_LENGTH_PX    2500   //Pixel length of bitmap
#define EMS_RSI_CENTER_X        42     //Pixel center on bitmap
#define EMS_RSI_CENTER_Y        41     //Pixel center on bitmap


class EMS {

public:
	EMS(PanelSDK &p);
	void Init(Saturn *vessel);										// Initialization
	void TimeStep(double MissionTime, double simdt);
	void SystemTimestep(double simdt);
	double GetdVRangeCounter() { return dVRangeCounter; };
	POINT ScribePntArray[EMS_SCROLL_LENGTH_PX*3]; //Thrice the number of pixels in the scrolling direction.
	POINT RSITriangle[3];
	void SetRSIRotation(double angle);
	int ScribePntCnt;
	int GetScrollOffset() { return ScribePntArray[ScribePntCnt-1].x-40; };
	void SwitchChanged();
	bool SPSThrustLight();
	bool pt05GLight();
	int LiftVectLight();
	bool IsOff();
	bool IsdVMode();
	void SaveState(FILEHANDLE scn);                                // SaveState callback
	void LoadState(FILEHANDLE scn);                                // LoadState callback
	
protected:
	bool IsPowered();
	
	void AccelerometerTimeStep(double simdt);
	double xacc, xaccG, constG;

	int status;
	int SlewScribe; //pixels
	int GScribe; //pixels
	double ScrollPosition; //fractional pixels
	double MaxScrollPosition;
	bool dVInitialized;
	VECTOR3 lastWeight;
	VECTOR3 lastGlobalVel;
	double dVRangeCounter;
	double dVTestTime;

	bool switchchangereset;

	void RotateRSI(double simdt);
	double RSIRotation; // Current angle in radians.
	double RSITarget;   // Target angle in radians.

	bool pt05GLightOn;
	bool pt05GFailed;

	//Comparator Circuits
	bool pt05GComparator(double simdt);
	bool pt02GComparator(double simdt);
	bool InitialTrip;
	double OneSecTimer;
	short int VerifyCorridor();

	short int LiftVectLightOn; //1 is up light, -1 is down light

	//Threshold Circuits
	double TenSecTimer;
	bool ThresholdBreeched; // .05G Comparator has been tripped.
	double ThresholdBreechTime; // MissionTime that .05 Comparator is tripped
	bool CorridorEvaluated;

	double ScrollBitmapLength;
	double ScrollScaling;

	PowerMerge DCPower;
	Saturn *sat;
};