/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2005

  Crawler Transporter vessel

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
  *	Revision 1.3  2005/06/29 11:01:17  tschachim
  *	new dynamics, added attachment management
  *	
  *	Revision 1.2  2005/06/14 16:14:41  tschachim
  *	File header inserted.
  *	
  **************************************************************************/

#define ORBITER_MODULE

#include "Crawler.h"

HINSTANCE g_hDLL;
char trace_file[] = "ProjectApollo-Crawler-trace.txt";


DLLCLBK void InitModule(HINSTANCE hModule) {
	g_hDLL = hModule;
}


DLLCLBK VESSEL *ovcInit(OBJHANDLE hvessel, int flightmodel) {

	return new Crawler(hvessel, flightmodel);
}


DLLCLBK void ovcExit(VESSEL *vessel) {

	if (vessel) delete (Crawler*)vessel;
}


Crawler::Crawler(OBJHANDLE hObj, int fmodel) : VESSEL2 (hObj, fmodel) {

	velocity = 0;
	targetHeading = 0;
	touchdownPointHeight = -0.0001;
	firstTimestepDone = false;
	useForce = false;
	standalone = false;
	reverseDirection = false;

	keyAccelerate = false;
	keyBrake= false;
	keyLeft= false;
	keyRight= false;
	keyUp = false;
	keyDown = false;

	soundlib.InitSoundLib(hObj, SOUND_DIRECTORY);
	soundlib.LoadSound(soundEngine, "CrawlerEngine.wav", BOTHVIEW_FADED_MEDIUM);
}


Crawler::~Crawler() {
}


void Crawler::clbkSetClassCaps(FILEHANDLE cfg) {

	SetEmptyMass(2721);
	SetSize(40);
	SetPMI(_V(133, 189, 89));

	SetSurfaceFrictionCoeff(0.005, 0.5);
	SetRotDrag (_V(0, 0, 0));
	SetCW(0, 0, 0, 0);
	SetPitchMomentScale(0);
	SetBankMomentScale(0);
	SetLiftCoeffFunc(0); 

    ClearMeshes();
    ClearExhaustRefs();
    ClearAttExhaustRefs();

	VECTOR3 meshoffset = _V(0,0,0);
    int icr = AddMesh(oapiLoadMeshGlobal("ProjectApollo\\Crawler"), &meshoffset);
	SetMeshVisibilityMode(icr, MESHVIS_ALWAYS);

	SetTouchdownPoint();
	SetCameraOffset(_V(16.5, 5.2, 18.25));

	CreateAttachment(false, _V(6.0, 10.0, 2.0), _V(0, 1, 0), _V(1, 0, 0), "ML", false);
}


void Crawler::clbkPreStep(double simt, double simdt, double mjd) {

	double maxVelocity = 0.894; //0.894; //30; //5.0e-6; // 1.0e-6;

	if (!firstTimestepDone) DoFirstTimestep(); 

	if (IsMLAttached()) maxVelocity = maxVelocity / 2.0;

	bool uf = useForce;
	if (uf) {
		if (simdt > 1) uf = false; 
		if (simdt < 0.3) uf = true; 
	}

	double head;
	oapiGetHeading(GetHandle(), &head);

	if (keyAccelerate) {
		velocity += 0.5 * simdt;
		if (velocity > maxVelocity) velocity = maxVelocity;
		keyAccelerate = false;
	}
	if (keyBrake) {
		velocity -= 0.5 * simdt;
		if (velocity < 0) velocity = 0;
		keyBrake = false;
	}

	if (!uf) {
		VESSELSTATUS vs;
		GetStatus(vs);

		double r = 80.0 + velocity * 10.0;
		double dheading = velocity * simdt / r / 2.0;

		if (keyLeft) {
			vs.vdata[0].z -= dheading;
			if(vs.vdata[0].z < 0) vs.vdata[0].z += 2.0 * PI;
			keyLeft = false;
		}
		if (keyRight) {
			vs.vdata[0].z += dheading;
			if(vs.vdata[0].z >= 2.0 * PI) vs.vdata[0].z -= -2.0 * PI;
			keyRight = false;
		}
		if (!reverseDirection) {		
			vs.vdata[0].x += sin(head) * velocity * simdt / (1.0 * 6371010.0);
			vs.vdata[0].y += cos(head) * velocity * simdt / (1.0 * 6371010.0);
		} else {
			vs.vdata[0].x -= sin(head) * velocity * simdt / (1.0 * 6371010.0);
			vs.vdata[0].y -= cos(head) * velocity * simdt / (1.0 * 6371010.0);
		}
		DefSetState(&vs);
		targetHeading = head;
	}

	if (keyUp) {
		touchdownPointHeight -= 0.5 * simdt;
		SetTouchdownPoint();
		keyUp = false;

	} else if (keyDown) {
		touchdownPointHeight += 0.5 * simdt;
		if (touchdownPointHeight > -0.0001) touchdownPointHeight = -0.0001;
		SetTouchdownPoint();
		keyDown = false;
	}

	VECTOR3 vv;
	GetShipAirspeedVector(vv);
	double v = length(vv);

	if (uf) {
		if (keyLeft || keyRight) {
			double r = 40.0 + v * 10.0;
			double dheading = sqrt(vv.x * vv.x + vv.z * vv.z) * simdt / r;
			double Fr = GetMass() * vv.z * vv.z / r;

			if (keyLeft) {
				targetHeading -= dheading;
				if (targetHeading < 0) targetHeading += 2.0 * PI;				
				keyLeft = false;

			} else if (keyRight) {
				targetHeading += dheading;
				if (targetHeading >= 2.0 * PI) targetHeading -= 2.0 * PI;
				keyRight = false;
			}			

			double diff = targetHeading - head;
			if (diff > PI) diff = diff - (2.0 * PI);
			if (diff < -PI) diff = (2.0 * PI) + diff;
			double Fd = GetMass() * diff * __min(5000.0, 1.0 / (simdt * simdt));
			
			AddForce(_V( Fd, 0, 0), _V(0, 0,  1));
			AddForce(_V(-Fd, 0, 0), _V(0, 0, -1));

		} else {
			targetHeading = head;
		}

		// lateral force
		double F = -GetMass() * vv.x * __min(10.0, 1.0 / simdt);
		AddForce(_V(F, 0, 0), _V(0, 0, 0));

		// vertical force
		//F = -GetMass() * vv.y * __min(10.0, 1.0 / simdt);
		//AddForce(_V(0, F, 0), _V(0, 0, 0));

		// longitudinal force
		if (!reverseDirection) {
			double Fa = GetMass() * (velocity - vv.z) * 1.0 / simdt;
			AddForce(_V(0, 0, Fa), _V(0, 0, 0));
		} else {
			double Fa = -GetMass() * (velocity + vv.z) * 1.0 / simdt;
			AddForce(_V(0, 0, Fa), _V(0, 0, 0));
		}
	}

	if (velocity != 0) 
		soundEngine.play(LOOP, (int)(127.5 + 127.5 * velocity / maxVelocity));
	else
		soundEngine.stop();

	//sprintf(oapiDebugString(), "Force %i simdt %f vv.z %f", uf, simdt, vv.z);

	/*
	OBJHANDLE hML = oapiGetVesselByName("AS-506-ML");
	VECTOR3 pos;
	GetRelativePos(hML, pos);
	sprintf(oapiDebugString(), "Dist %f", length(pos));
	*/
}


void Crawler::SetTouchdownPoint() {

	SetTouchdownPoints(_V(  0, touchdownPointHeight,  20), 
					   _V(-10, touchdownPointHeight, -20), 
					   _V( 10, touchdownPointHeight, -20));
}


void Crawler::DoFirstTimestep() {

	oapiGetHeading(GetHandle(), &targetHeading);
	
	// Turn off pretty much everything that Orbitersound does by default.
	soundlib.SoundOptionOnOff(PLAYCOUNTDOWNWHENTAKEOFF, FALSE);
	soundlib.SoundOptionOnOff(PLAYCABINAIRCONDITIONING, FALSE);
	soundlib.SoundOptionOnOff(PLAYCABINRANDOMAMBIANCE, FALSE);
	soundlib.SoundOptionOnOff(PLAYLANDINGANDGROUNDSOUND, FALSE);
	soundlib.SoundOptionOnOff(PLAYRADIOATC, FALSE);
	soundlib.SoundOptionOnOff(PLAYRADARBIP, FALSE);
	soundlib.SoundOptionOnOff(DISPLAYTIMER, FALSE);

	if (!standalone) {
		// find the Mobile Launcher and the Launch Vehicle
		char nameML[256], nameLV[256], buffer[256];
		strncpy(nameML, GetName(), strlen(GetName()) - 3);
		nameML[strlen(GetName()) - 3] = '\0';
		strcpy(nameLV, nameML);
		strcat(nameML, "-ML");

		double vcount = oapiGetVesselCount();
		for (int i = 0; i < vcount; i++)	{
			OBJHANDLE h = oapiGetVesselByIndex(i);
			oapiGetObjectName(h, buffer, 256);
			if (!strcmp(nameML, buffer)){
				hML = h;
			} else if (!strcmp(nameLV, buffer)){
				hLV = h;
			}
		}

		// re-attach the ML to setup the meshes
		if (IsMLAttached()) AttachML(); else DetachML();
		//AttachLV();
	}
	firstTimestepDone = true;
}

void Crawler::clbkLoadStateEx(FILEHANDLE scn, void *status) {
	
	char *line;
	
	while (oapiReadScenario_nextline (scn, line)) {
		if (!strnicmp (line, "USEFORCE", 8)) {
			sscanf (line + 8, "%i", &useForce);
		} else if (!strnicmp (line, "STANDALONE", 10)) {
			sscanf (line + 10, "%i", &standalone);
		} else {
			ParseScenarioLineEx (line, status);
		}
	}
}

void Crawler::clbkSaveState(FILEHANDLE scn) {
	
	VESSEL2::clbkSaveState (scn);
	oapiWriteScenario_int (scn, "USEFORCE", useForce);
	oapiWriteScenario_int (scn, "STANDALONE", standalone);
}

int Crawler::clbkConsumeDirectKey(char *kstate) {

	if (!firstTimestepDone) return 0;

	if (KEYMOD_SHIFT(kstate) || KEYMOD_CONTROL(kstate)) {
		return 0; 
	}

	if (KEYDOWN(kstate, OAPI_KEY_NUMPAD8)) {
		keyAccelerate = true;				
		RESETKEY(kstate, OAPI_KEY_NUMPAD8);
	}

	if (KEYDOWN(kstate, OAPI_KEY_NUMPAD5)) {
		keyBrake = true;				
		RESETKEY(kstate, OAPI_KEY_NUMPAD5);
	}

	if (KEYDOWN(kstate, OAPI_KEY_NUMPAD4)) {
		keyLeft = true;				
		RESETKEY(kstate, OAPI_KEY_NUMPAD4);
	}

	if (KEYDOWN(kstate, OAPI_KEY_NUMPAD6)) {
		keyRight = true;				
		RESETKEY(kstate, OAPI_KEY_NUMPAD6);
	}

	if (KEYDOWN(kstate, OAPI_KEY_NUMPAD9)) {
		keyUp = true;				
		RESETKEY(kstate, OAPI_KEY_NUMPAD9);
	}

	if (KEYDOWN(kstate, OAPI_KEY_NUMPAD3)) {
		keyDown = true;				
		RESETKEY(kstate, OAPI_KEY_NUMPAD3);
	}
	return 0;
}

int Crawler::clbkConsumeBufferedKey(DWORD key, bool down, char *kstate) {

	if (!firstTimestepDone) return 0;

	if (KEYMOD_SHIFT(kstate) || KEYMOD_CONTROL(kstate)) {
		return 0; 
	}

	if (key == OAPI_KEY_NUMPAD1 && down == true) {
		if (IsMLAttached())
			DetachML();
		else
			AttachML();
		return 1;
	}

	if (key == OAPI_KEY_NUMPAD2 && down == true) {
		ToggleDirection();
		return 1;
	}
	return 0;
}

void Crawler::AttachML() {

	if (standalone) return;
	if (velocity != 0) return;

	ATTACHMENTHANDLE ah = GetAttachmentHandle(false, 0);
	VESSEL *ml = oapiGetVesselInterface(hML);

	if (GetAttachmentStatus(ah) == NULL) {
		// Is the LV attached?
		if (GetAttachmentStatus(ml->GetAttachmentHandle(false, 0)) == NULL) return;

		// Is the crawler close enough to the ML?
		VECTOR3 pos;
		GetRelativePos(hML, pos);
		if (length(pos) > 71) return;

		AttachChild(hML, ah, ml->GetAttachmentHandle(true, 0));
	}

	ml->ClearMeshes();
	ml->AddMesh(oapiLoadMeshGlobal("ProjectApollo\\Saturn5ML"));
}

void Crawler::DetachML() {

	if (standalone) return;
	if (velocity != 0) return;

	ATTACHMENTHANDLE ah = GetAttachmentHandle(false, 0);
	VESSEL *ml = oapiGetVesselInterface(hML);

	if (GetAttachmentStatus(ah) != NULL) {
		// Is the crawler near launch pad?
		VESSEL *ml = oapiGetVesselInterface(hML);
		VESSEL *lv = oapiGetVesselInterface(hLV);

		OBJHANDLE hEarth = oapiGetGbodyByName("Earth");
		OBJHANDLE hCanaveral = oapiGetBaseByName(hEarth, "Cape Canaveral");
		double padlng, padlat, padrad, lng, lat, rad;
		// TODO pad index config
		oapiGetBasePadEquPos(hCanaveral, 1, &padlng, &padlat, &padrad);

		GetEquPos(lng, lat, rad);
		double dlng = (padlng - lng) * 6371010.0;
		double dlat = (padlat - lat) * 6371010.0;

		if (dlng > -5 && dlng < 5 && dlat > -8 && dlat < 2) {

			ATTACHMENTHANDLE ahml = ml->GetAttachmentHandle(false, 0);
			if (GetAttachmentStatus(ahml) != NULL) {
				ml->DetachChild(ahml);
			}
			DetachChild(ah);

			ml->SetTouchdownPoints(_V(  0, -87.0,  10), 
				  			       _V(-10, -87.0, -10), 
							       _V( 10, -87.0, -10));
			VESSELSTATUS vs;
			ml->GetStatus(vs);
			vs.vdata[0].x = -80.6081642;
			vs.vdata[0].y = 28.6009997;
			vs.vdata[0].z = 89.85;
			ml->DefSetState(&vs);
		
		} else {
			DetachChild(ah);
			ml->SetTouchdownPoints(_V(  0, touchdownPointHeight - 70.0,  10), 
				  			       _V(-10, touchdownPointHeight - 70.0, -10), 
							       _V( 10, touchdownPointHeight - 70.0, -10));
		}
	}

	ml->ClearMeshes();
	ml->AddMesh(oapiLoadMeshGlobal("ProjectApollo\\Saturn5ML"));
	VECTOR3 meshoffset = _V(0, -67, -5.5);
	ml->AddMesh(oapiLoadMeshGlobal("ProjectApollo\\Saturn5MLPedestals"), &meshoffset);
}

bool Crawler::IsMLAttached() {

	if (standalone) return false;

	ATTACHMENTHANDLE ah = GetAttachmentHandle(false, 0);
	return (GetAttachmentStatus(ah) != NULL);
}

void Crawler::ToggleDirection() {

	if (velocity != 0) return;

	if (reverseDirection) {
		SetCameraOffset(_V(16.5, 5.2, 18.5));
		SetCameraDefaultDirection(_V(0, 0, 1));
		reverseDirection = false;

	} else {
		SetCameraOffset(_V(-15.0, 5.2, -15.0));
		SetCameraDefaultDirection(_V(0, 0, -1));
		reverseDirection = true;
	} 
}

void Crawler::AttachLV() {

	if (standalone) return;

	VESSEL *ml = oapiGetVesselInterface(hML);
	VESSEL *lv = oapiGetVesselInterface(hLV);
	ATTACHMENTHANDLE ah = ml->GetAttachmentHandle(false, 0);

	if (GetAttachmentStatus(ah) == NULL) {
		ml->AttachChild(hLV, ah, lv->GetAttachmentHandle(true, 0));
	}
}
