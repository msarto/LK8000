/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgWaypointEdit.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/

#include "externs.h"
#include "InputEvents.h"
#include "Waypointparser.h"
#include "Dialogs.h"
#include "dlgTools.h"
#include "WindowControls.h"
#include <iterator>
#include <functional>
#include "resource.h"
#include "LKStyle.h"

using namespace std::placeholders;

extern void LatLonToUtmWGS84 (int& utmXZone, char& utmYZone, double& easting, double& northing, double lat, double lon);
extern void UtmToLatLonWGS84 (int utmXZone, char utmYZone, double easting, double northing, double& lat, double& lon);

static WAYPOINT *global_wpt=NULL;

static void UpdateButtons(WndForm* pForm) {
  TCHAR text[MAX_PATH];

  WndButton* buttonName = ((WndButton *)pForm->FindByName(TEXT("cmdName")));
  if (buttonName) {
	if (_tcslen(global_wpt->Name)<=0) {
		// LKTOKEN  _@M451_ = "Name"
		_stprintf(text,TEXT("%s: %s"), MsgToken(451),
		// LKTOKEN  _@M7_ = "(blank)"
		MsgToken(7));
	} else {
		// LKTOKEN  _@M451_ = "Name"
		_stprintf(text,TEXT("%s: %s"), MsgToken(451),
		global_wpt->Name);
	}
	buttonName->SetCaption(text);
  }


  WndButton* buttonComment = ((WndButton *)pForm->FindByName(TEXT("cmdComment")));
  if (buttonComment) {
	if ((global_wpt->Comment==NULL) || (_tcslen(global_wpt->Comment)<=0) ) {
		// LKTOKEN  _@M190_ = "Comment"
		_stprintf(text,TEXT("%s: %s"), MsgToken(190),
		// LKTOKEN  _@M7_ = "(blank)"
		MsgToken(7));
	} else {
		// LKTOKEN  _@M190_ = "Comment"
		_stprintf(text,TEXT("%s: %s"), MsgToken(190),
		global_wpt->Comment);
	}
	buttonComment->SetCaption(text);
  }
}

static void OnNameClicked(WndButton* pWnd) {
  dlgTextEntryShowModal(global_wpt->Name, NAME_SIZE);
  if(pWnd) {
    UpdateButtons(pWnd->GetParentWndForm());
  }
}

static void OnCommentClicked(WndButton* pWnd) {
	//@ 101219
	TCHAR comment[COMMENT_SIZE*2];
	if (global_wpt->Comment != NULL) {
		LK_tcsncpy(comment,global_wpt->Comment, COMMENT_SIZE);
	} else {
		_tcscpy(comment,_T(""));
	}
	dlgTextEntryShowModal(comment, COMMENT_SIZE);

	// in any case free the space
	if (global_wpt->Comment != NULL) {
		free(global_wpt->Comment);
		global_wpt->Comment=NULL;
	}
	if (_tcslen(comment)>0) {
		// do we have a new comment?
		global_wpt->Comment = (TCHAR*)malloc((_tcslen(comment)+2)*sizeof(TCHAR));
		if (global_wpt->Comment == NULL) {
			OutOfMemory(_T(__FILE__),__LINE__);
		} else {
			_tcscpy(global_wpt->Comment,comment);
		}
	}

  if(pWnd) {
    UpdateButtons(pWnd->GetParentWndForm());
  }
}


static void SetUnits(WndForm* wf) {
  WndProperty* wp;
  switch (Units::CoordinateFormat) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
    if (wp) {
      wp->SetVisible(false);
    }
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
    // hide this field for DD.dddd format
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
    if (wp) {
      wp->SetVisible(false);
    }
    break;
  case 4: // UTM (" 32T 123456 1234567 ")
    break;
  }
}
static const TCHAR* cYZone[] = {
		TEXT("C"),TEXT("D"),TEXT("E"),TEXT("F"),
		TEXT("G"),TEXT("H"),TEXT("J"),TEXT("K"),
		TEXT("L"),TEXT("M"),TEXT("N"),TEXT("P"),
		TEXT("Q"),TEXT("R"),TEXT("S"),TEXT("T"),
		TEXT("U"),TEXT("V"),TEXT("W"),TEXT("X")
};

char enumToYZone(int i){
	static const char cArray[] = "CDEFGHJKLMNPQRSTUVWX";
	return cArray[i];
}

int YZoneToenum(char c){
	static const char cArray[] = "CDEFGHJKLMNPQRSTUVWX";

	return std::distance(std::begin(cArray), std::find(std::begin(cArray), std::end(cArray), c));
}


static void SetValues(WndForm* wf) {
  WndProperty* wp;
  if(Units::CoordinateFormat==4) {
	  int utmXZone;
	  char utmYZone;
	  double easting, northing;

      LatLonToUtmWGS84(utmXZone, utmYZone, easting, northing, global_wpt->Latitude, global_wpt->Longitude );

      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMzoneX"));
      if (wp) {
	wp->GetDataField()->SetAsInteger(utmXZone);
		wp->RefreshDisplay();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMzoneY"));
      if (wp) {
	  DataField* dfe = wp->GetDataField();
	  if(dfe){
		  std::for_each(std::begin(cYZone), std::end(cYZone), std::bind(&DataField::addEnumText, dfe, _1, nullptr));
		  dfe->Set(YZoneToenum(utmYZone));
	  }
	  wp->RefreshDisplay();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMeast"));
      if (wp) {
	  wp->GetDataField()->SetAsFloat(easting);
	  wp->RefreshDisplay();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMnorth"));
      if (wp) {
	  wp->GetDataField()->SetAsFloat(northing);
	  wp->RefreshDisplay();
      }
  } else {
	  bool sign;
	  int dd,mm,ss;


	  Units::LongitudeToDMS(global_wpt->Longitude,
				&dd, &mm, &ss, &sign);

	   wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeSign"));
	  if (wp) {
		DataField* dfe = wp->GetDataField();
		dfe->addEnumText((TEXT("W")));
		dfe->addEnumText((TEXT("E")));
		dfe->Set(sign);
		wp->RefreshDisplay();
	  }
	  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeD"));
	  if (wp) {
		wp->GetDataField()->SetAsFloat(dd);
		wp->RefreshDisplay();
	  }

	  switch (Units::CoordinateFormat) {
	  case 0: // ("DDMMSS");
	  case 1: // ("DDMMSS.ss");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(mm);
		  wp->RefreshDisplay();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(ss);
		  wp->RefreshDisplay();
		}
		break;
	  case 2: // ("DDMM.mmm");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(mm);
		  wp->RefreshDisplay();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(1000.0*ss/60.0);
		  wp->RefreshDisplay();
		}
		break;
	  case 3: // ("DD.dddd");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(10000.0*(mm+ss/60.0)/60.0);
		  wp->RefreshDisplay();
		}
		break;
	  case 4:
		  break;
	  }

	  Units::LatitudeToDMS(global_wpt->Latitude,
				   &dd, &mm, &ss, &sign);

	  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeSign"));
	  if (wp) {
		DataField* dfe = wp->GetDataField();
		dfe->addEnumText((TEXT("S")));
		dfe->addEnumText((TEXT("N")));
		dfe->Set(sign);
		wp->RefreshDisplay();
	  }
	  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeD"));
	  if (wp) {
		wp->GetDataField()->SetAsFloat(dd);
		wp->RefreshDisplay();
	  }

	  switch (Units::CoordinateFormat) {
	  case 0: // ("DDMMSS");
	  case 1: // ("DDMMSS.ss");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(mm);
		  wp->RefreshDisplay();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(ss);
		  wp->RefreshDisplay();
		}
		break;
	  case 2: // ("DDMM.mmm");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(mm);
		  wp->RefreshDisplay();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(1000.0*ss/60.0);
		  wp->RefreshDisplay();
		}
		break;
	  case 3: // ("DD.dddd");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
		if (wp) {
		  wp->GetDataField()->SetAsFloat(10000.0*(mm+ss/60.0)/60.0);
		  wp->RefreshDisplay();
		}
		break;
	  case 4:
		break;
	  }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(
				   iround(global_wpt->Altitude
					  *ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlags"));
  if (wp) {
    DataField* dfe = wp->GetDataField();
	// LKTOKEN _@M1226_ "Turnpoint"
    dfe->addEnumText(MsgToken(1226));
	// LKTOKEN _@M1224_ "Airport"
    dfe->addEnumText(MsgToken(1224));
	// LKTOKEN _@M1225_ "Landable"
    dfe->addEnumText(MsgToken(1225));
    dfe->Set(0);
    if ((global_wpt->Flags & LANDPOINT)==LANDPOINT) {
      dfe->Set(2);
    }
    if ((global_wpt->Flags & AIRPORT)==AIRPORT) {
      dfe->Set(1);
    }

    wp->RefreshDisplay();
  }
}


static void GetValues(WndForm* wf) {
  WndProperty* wp;
  double num=0, mm = 0, ss = 0; // mm,ss are numerators (division) so don't want to lose decimals

  if(Units::CoordinateFormat==4) {
	  int utmXZone=0;
	  char utmYZone='\0';;
	  double easting=0, northing=0;

      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMzoneX"));
      if (wp) {
	utmXZone = wp->GetDataField()->GetAsInteger();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMzoneY"));
      if (wp) {
	  DataField* dfe = wp->GetDataField();
	  if(dfe){
		  utmYZone = enumToYZone(dfe->GetAsInteger());
	  }
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMeast"));
      if (wp) {
	  easting = wp->GetDataField()->GetAsFloat();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpUTMnorth"));
      if (wp) {
	  northing = wp->GetDataField()->GetAsFloat();
      }
      UtmToLatLonWGS84(utmXZone, utmYZone, easting, northing, global_wpt->Latitude, global_wpt->Longitude );

  } else {
	  bool sign = false;
	  int dd = 0;

	  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeSign"));
	  if (wp) {
		sign = (wp->GetDataField()->GetAsInteger()==1);
	  }
	  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeD"));
	  if (wp) {
		dd = wp->GetDataField()->GetAsInteger();
	  }

	  switch (Units::CoordinateFormat) {
	  case 0: // ("DDMMSS");
	  case 1: // ("DDMMSS.ss");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
		if (wp) {
		  mm = wp->GetDataField()->GetAsInteger();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
		if (wp) {
		  ss = wp->GetDataField()->GetAsInteger();
		}
		num = dd+mm/60.0+ss/3600.0;
		break;
	  case 2: // ("DDMM.mmm");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
		if (wp) {
		  mm = wp->GetDataField()->GetAsInteger();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
		if (wp) {
		  ss = wp->GetDataField()->GetAsInteger();
		}
		num = dd+(mm+ss/1000.0)/60.0;
		break;
	  case 3: // ("DD.dddd");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
		if (wp) {
		  mm = wp->GetDataField()->GetAsInteger();
		}
		num = dd+mm/10000;
		break;
	  case 4:
		break;
	  }
	  if (!sign) {
		num = -num;
	  }

	  global_wpt->Longitude = num;

	  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeSign"));
	  if (wp) {
		sign = (wp->GetDataField()->GetAsInteger()==1);
	  }
	  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeD"));
	  if (wp) {
		dd = wp->GetDataField()->GetAsInteger();
	  }

	  switch (Units::CoordinateFormat) {
	  case 0: // ("DDMMSS");
	  case 1: // ("DDMMSS.ss");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
		if (wp) {
		  mm = wp->GetDataField()->GetAsInteger();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
		if (wp) {
		  ss = wp->GetDataField()->GetAsInteger();
		}
		num = dd+mm/60.0+ss/3600.0;
		break;
	  case 2: // ("DDMM.mmm");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
		if (wp) {
		  mm = wp->GetDataField()->GetAsInteger();
		}
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
		if (wp) {
		  ss = wp->GetDataField()->GetAsInteger();
		}
		num = dd+(mm+ss/1000.0)/60.0;
		break;
	  case 3: // ("DD.dddd");
		wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
		if (wp) {
		  mm = wp->GetDataField()->GetAsInteger();
		}
		num = dd+mm/10000;
		break;
	  case 4:
		break;
	  }
	  if (!sign) {
		num = -num;
	  }

	  global_wpt->Latitude = num;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
  if (wp) {
    ss = wp->GetDataField()->GetAsFloat();
    if (ss==0) {
      WaypointAltitudeFromTerrain(global_wpt);
    } else {
      global_wpt->Altitude = ss/ALTITUDEMODIFY;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlags"));
  if (wp) {
    int myflag = wp->GetDataField()->GetAsInteger();
    switch(myflag) {
    case 0:
      global_wpt->Flags = TURNPOINT;
	#if 100825
	if ( global_wpt->Format == LKW_CUP) {
		// set normal turnpoint style
		global_wpt->Style = STYLE_NORMAL;
	}
	#endif
      break;
    case 1:
      global_wpt->Flags = AIRPORT | TURNPOINT;
	#if 100825
	if ( global_wpt->Format == LKW_CUP) {
		// set airfield style
		global_wpt->Style = STYLE_AIRFIELDSOLID;
	}
	#endif
      break;
    case 2:
      global_wpt->Flags = LANDPOINT;
	#if 100825
	if ( global_wpt->Format == LKW_CUP) {
		// set outlanding style
		global_wpt->Style = STYLE_OUTLANDING;
	}
	#endif
      break;
    default:
      global_wpt->Flags = 0;
      break;
    };
  }
}


static void OnCloseClicked(WndButton* pWnd){
  if(pWnd) {
    WndForm * pForm = pWnd->GetParentWndForm();
    if(pForm) {
      pForm->SetModalResult(mrOK);
    }
  }
}


static CallBackTableEntry_t CallBackTable[]={
  ClickNotifyCallbackEntry(OnCloseClicked),
  EndCallBackEntry()
};



void dlgWaypointEditShowModal(WAYPOINT *wpt) {
  if (!wpt) {
    return;
  }

  global_wpt = wpt;

  unsigned XmlResID = ScreenLandscape?IDR_XML_WAYPOINTEDIT_L:IDR_XML_WAYPOINTEDIT_P;

  if(Units::CoordinateFormat == cfUTM) {
	  XmlResID = ScreenLandscape?IDR_XML_WAYPOINTEDITUTM_L:IDR_XML_WAYPOINTEDITUTM_P;
  }

  WndForm* wf = dlgLoadFromXML(CallBackTable, XmlResID);
  if (wf) {

    WndButton* buttonName = ((WndButton *)wf->FindByName(TEXT("cmdName")));
    if (buttonName) {
      buttonName->SetOnClickNotify(OnNameClicked);
    }

    WndButton* buttonComment = ((WndButton *)wf->FindByName(TEXT("cmdComment")));
    if (buttonComment) {
      buttonComment->SetOnClickNotify(OnCommentClicked);
    }

    UpdateButtons(wf);

    SetUnits(wf);
    SetValues(wf);

    if (wf->ShowModal()==mrOK) {

      ////
      GetValues(wf);

    }
    delete wf;
  }
}
