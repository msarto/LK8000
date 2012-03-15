/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: LKDrawWaypoints.cpp,v 1.2 2010/12/11 19:03:52 root Exp root $
*/

#include "externs.h"
#include "MapWindow.h"
#include "LKMapWindow.h"
#include "LKStyle.h"
#include "Bitmaps.h"
#include "DoInits.h"
#include "LKObjects.h"


#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

#include "utils/heapcheck.h"
#include <string.h>

MapWaypointLabel_t MapWaypointLabelList[200]; 

int MapWaypointLabelListCount=0;

#define WPCIRCLESIZE        2 // check also duplicate in TextInBox!


bool MapWindow::WaypointInRange(int i) {
  return (zoom.RealScale() <= 10);
}

void DrawRunway(const HDC hdc,const WAYPOINT* wp, const RECT rc, const double scale);

int _cdecl MapWaypointLabelListCompare(const void *elem1, const void *elem2 ){

  // Now sorts elements in task preferentially.
  if (((MapWaypointLabel_t *)elem1)->AltArivalAGL > ((MapWaypointLabel_t *)elem2)->AltArivalAGL)
    return (-1);
  if (((MapWaypointLabel_t *)elem1)->AltArivalAGL < ((MapWaypointLabel_t *)elem2)->AltArivalAGL)
    return (+1);
  return (0);
}


void MapWaypointLabelAdd(const TCHAR *Name, const int X, const int Y, 
			 const TextInBoxMode_t *Mode, 
			 const int AltArivalAGL, const bool inTask, const bool isLandable, const bool isAirport, 
			 const bool isExcluded,  const int index, const short style){
  MapWaypointLabel_t *E;

  static int xLeft,xRight,yTop,yBottom, labelListSize;

  if (DoInit[MDI_MAPWPLABELADD]) {
	xLeft=MapWindow::MapRect.left-WPCIRCLESIZE;
	xRight=MapWindow::MapRect.right+(WPCIRCLESIZE*3);
	yTop=MapWindow::MapRect.top-WPCIRCLESIZE;
	yBottom=MapWindow::MapRect.bottom+WPCIRCLESIZE;

	labelListSize=(signed int)(sizeof(MapWaypointLabelList)/sizeof(MapWaypointLabel_t))-1;

	DoInit[MDI_MAPWPLABELADD]=false;
  }

  if ((X<xLeft) || (X>xRight) || (Y<yTop) || (Y>yBottom)) return;

  if (MapWaypointLabelListCount >= labelListSize) return;

  E = &MapWaypointLabelList[MapWaypointLabelListCount];

  _tcscpy(E->Name, Name);
  E->Pos.x = X;
  E->Pos.y = Y;
  memcpy((void*)&(E->Mode), Mode, sizeof(TextInBoxMode_t));     // E->Mode = Mode;
  E->AltArivalAGL = AltArivalAGL;
  E->inTask = inTask;
  E->isLandable = isLandable;
  E->isAirport  = isAirport;
  E->isExcluded = isExcluded;
  E->index = index;
  E->style = style;

  MapWaypointLabelListCount++;

}






void MapWindow::DrawWaypointsNew(HDC hdc, const RECT rc)
{
  unsigned int i; 
  int bestwp=-1;
  TCHAR Buffer[LKSIZEBUFFER];
  TCHAR Buffer2[LKSIZEBUFFER];
  TCHAR sAltUnit[LKSIZEBUFFERUNIT];
  TextInBoxMode_t TextDisplayMode = {0};

  static int resizer[10]={ 20,20,20,3,5,8,10,12,0 };


  // if pan mode, show full names
  int pDisplayTextType = DisplayTextType;

  if (!WayPointList) return;

  _tcscpy(sAltUnit, Units::GetAltitudeName());

  MapWaypointLabelListCount = 0;

  int arrivalcutoff=0, foundairport=0;
  bool isairport;
  bool islandpoint;

  if (MapWindow::zoom.RealScale() <=20) for(i=0;i<NumberOfWayPoints;i++) {

	if (WayPointList[i].Visible != TRUE )	continue; // false may not be FALSE?

	if (WayPointCalc[i].IsAirport) {
		if (WayPointList[i].Reachable == FALSE)	{ 
			SelectObject(hDCTemp,hBmpAirportUnReachable);
		} else {
			SelectObject(hDCTemp,hBmpAirportReachable);
			if ( arrivalcutoff < (int)(WayPointList[i].AltArivalAGL)) {
				arrivalcutoff = (int)(WayPointList[i].AltArivalAGL);
				bestwp=i; foundairport++;
			}
		}
	} else {
		if ( WayPointCalc[i].IsOutlanding ) {
			// outlanding
			if (WayPointList[i].Reachable == FALSE)
				SelectObject(hDCTemp,hBmpFieldUnReachable);
			else { 
				SelectObject(hDCTemp,hBmpFieldReachable);
				// get the outlanding as bestwp only if no other choice
				if (foundairport == 0) { 
					// do not set arrivalcutoff: any next reachable airport is better than an outlanding
					if ( arrivalcutoff < (int)(WayPointList[i].AltArivalAGL)) bestwp=i;  
				}
			}
		} else continue; // do not draw icons for normal turnpoints here
	}
//	  Appearance.IndLandable=wpLandableDefault,
    if(Appearance.IndLandable == wpLandableDefault ) // WinPilot style
    {
      //double fScaleFact =MapWindow::zoom.RealScale();
      //if(fScaleFact < 0.1)  fScaleFact = 0.1; // prevent division by zero

      //double fScaleFact = zoom.DrawScale();
      //if(fScaleFact > 6000.0) fScaleFact = 6000.0; // limit to prevent huge airfiel symbols
      //if(fScaleFact < 1600)   fScaleFact = 1600; // limit to prevent tiny airfiel symbols
      //if(fScaleFact < 2500)   fScaleFact = 2500; // limit to prevent tiny airfiel symbols

  	  DrawRunway(hdc,&WayPointList[i],rc, zoom.DrawScale());
    }
    else
    {
	  DrawBitmapX(hdc, WayPointList[i].Screen.x-10, WayPointList[i].Screen.y-10, 20,20, hDCTemp,0,0,SRCPAINT,false);
  	  DrawBitmapX(hdc, WayPointList[i].Screen.x-10, WayPointList[i].Screen.y-10, 20,20, hDCTemp,20,0,SRCAND,false);
    }

  } // for all waypoints

  if (foundairport==0 && bestwp>=0)  arrivalcutoff = (int)WayPointList[bestwp].AltArivalAGL;


  for(i=0;i<NumberOfWayPoints;i++)
    {
      if(WayPointList[i].Visible )
	{

#ifdef HAVEEXCEPTIONS
	  __try{
#endif

	    bool irange = false;
	    bool intask = false;
	    bool islandable;	// isairport+islandpoint
	    bool excluded=false;
	    bool dowrite;

	    intask = WaypointInTask(i);
	    dowrite = intask; // initially set only for intask
	    memset((void*)&TextDisplayMode, 0, sizeof(TextDisplayMode));

	    // airports are also landpoints. should be better handled
	    isairport=((WayPointList[i].Flags & AIRPORT) == AIRPORT);
	    islandpoint=((WayPointList[i].Flags & LANDPOINT) == LANDPOINT);

	islandable=WayPointCalc[i].IsLandable;

 	    // always in range if MapScale <=10 
	    irange = WaypointInRange(i); 

	    if(MapWindow::zoom.RealScale() > 20) { 
	      SelectObject(hDCTemp,hInvSmall);
	      irange=false;
	      goto NiklausWirth; // with compliments
	    } 

	    if( islandable ) { 

	      if(WayPointList[i].Reachable){

		TextDisplayMode.Reachable = 1;

		if ( isairport )
		  SelectObject(hDCTemp,hBmpAirportReachable);
		else
		  SelectObject(hDCTemp,hBmpFieldReachable);

		if ((DeclutterLabels<MAPLABELS_ALLOFF)||intask) { 

		  dowrite = true;
		  // exclude outlandings worst than visible airports, only when there are visible reachable airports!
		  if ( isairport==false && islandpoint==true ) {
		    if ( (int)WayPointList[i].AltArivalAGL >=2000 ) { // more filter
		      excluded=true;
		    } else {
		      if ( (bestwp>=0) && (i==(unsigned)bestwp) && (foundairport==0) ) { // this outlanding is the best option
			isairport=true;
			islandpoint=false; // make it an airport TODO paint it as best
		      } else
			{
			  if ( foundairport >0 ) {
			    if ( (int)WayPointList[i].AltArivalAGL <= arrivalcutoff ) {
			      excluded=true;
			    } 
			  }
			}
		    }

		  }  else
		    // do not display airport arrival if close to the best so far.
		    // ex: best arrival is 1200m, include onlye below 1200/4  (prevent division by zero)
		    // This way we only display far points, and skip closer points 
		    // WE NEED MORE INFO ABOUT LANDING POINTS: THE .CUP FORMAT WILL LET US KNOW WHAT IS
		    // BEST TO SHOW AND WHAT IS NOT. Winpilot format is useless here.
		    {
		      dowrite=true;// TEST FIX not necessary probably
		      // it's an airport
		      if ( (bestwp>=0) && (i != (unsigned)bestwp) && (arrivalcutoff>600) ) {
			if ( (arrivalcutoff / ((int)WayPointList[i].AltArivalAGL+1))<4 ) {
			  excluded=true;
			}
		      }
		    } 
		}


	      } else  // landable waypoint is unreachable
		{
		  dowrite=true; 
		  if ( isairport ) {
		    SelectObject(hDCTemp,hBmpAirportUnReachable);
		  } else {
		    SelectObject(hDCTemp,hBmpFieldUnReachable);
		  }
		}
	    } else { // waypoint is an ordinary turnpoint
	      if(MapWindow::zoom.RealScale() > 4) {
		if (BlackScreen) 
			SelectObject(hDCTemp,hInvSmall);
		else
			SelectObject(hDCTemp,hSmall);
	      } else {
		if (BlackScreen) 
			SelectObject(hDCTemp,hInvTurnPoint);
		else
			SelectObject(hDCTemp,hTurnPoint);
	      }

	    } // end landable-not landable

	  NiklausWirth:

	    if (intask || (OutlinedTp==(OutlinedTp_t)otAll) ) { 
	      TextDisplayMode.WhiteBold = 1;
	      TextDisplayMode.Color=RGB_WHITE; 
	    }
	

	// No matter of how we thought to draw it, let it up to the user..
	switch(NewMapDeclutter) {
		case 0:
			excluded=false; // no decluttering: show all airports and outlandings
			break;
		case 1:
			if ( isairport ) excluded=false; //  show all airports, declutter outlandings
			break;
		default:
			break; // else work normally
	}


	    // here come both turnpoints and landables..
	    if( intask || irange || dowrite) {  // irange always set when MapScale <=10 

	      bool draw_alt = TextDisplayMode.Reachable && ((DeclutterLabels<MAPLABELS_ONLYTOPO) || intask); // 100711 reachable landing point!

	      if (excluded==true) draw_alt=false; // exclude close outlandings

	      switch(pDisplayTextType) {

	     case DISPLAYNAME:
	     case DISPLAYFIRSTTHREE:
	     case DISPLAYFIRSTFIVE:
	     case DISPLAYFIRST8:
	     case DISPLAYFIRST10:
	     case DISPLAYFIRST12:

		dowrite = (DeclutterLabels<MAPLABELS_ONLYTOPO) || intask || islandable;  // 100711
		if ( (islandable && !isairport) && MapWindow::zoom.RealScale() >=10 ) dowrite=0; // FIX then no need to go further

		// 101215 
		if (DisplayTextType == DISPLAYNAME) {
			_tcscpy(Buffer2,WayPointList[i].Name);
		} else {
			_tcsncpy(Buffer2, WayPointList[i].Name, resizer[DisplayTextType]);
			Buffer2[resizer[DisplayTextType]] = '\0';
		}
		// ConvToUpper(Buffer2); 

		if (draw_alt) {
		  if ( ArrivalValue == (ArrivalValue_t) avAltitude ) {
			if ( (MapBox == (MapBox_t)mbUnboxedNoUnit) || (MapBox == (MapBox_t)mbBoxedNoUnit) )
		  		wsprintf(Buffer, TEXT("%s:%d"), Buffer2, (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY));
			else
		  		wsprintf(Buffer, TEXT("%s:%d%s"), Buffer2, (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), sAltUnit);
		  } else 
			wsprintf(Buffer, TEXT("%s:%d"), Buffer2, (int)WayPointCalc[i].GR);


		  if ( (MapBox == (MapBox_t)mbBoxed) || (MapBox == (MapBox_t)mbBoxedNoUnit)) {
			  TextDisplayMode.Border = 1;
			  TextDisplayMode.WhiteBold = 0;
		  } else
		  	TextDisplayMode.WhiteBold = 1; // outlined 
		  	TextDisplayMode.Color=RGB_WHITE; 
		} else {
			//wsprintf(Buffer, TEXT("%s"),Buffer2);
			_tcscpy(Buffer,Buffer2);
			if (islandable && isairport) {
				TextDisplayMode.WhiteBold = 1; // outlined 
				TextDisplayMode.Color=RGB_WHITE; 
			}
		}
				  
		break;
	      case DISPLAYNUMBER:
		dowrite = (DeclutterLabels<MAPLABELS_ONLYTOPO) || intask || islandable;  // 100620
		if ( (islandable && !isairport) && MapWindow::zoom.RealScale() >=10 ) dowrite=0; // FIX then no need to go further

		if (draw_alt) {
		  if ( ArrivalValue == (ArrivalValue_t) avAltitude ) {
			if ( (MapBox == (MapBox_t)mbUnboxedNoUnit) || (MapBox == (MapBox_t)mbBoxedNoUnit) )
		  		wsprintf(Buffer, TEXT("%d:%d"), WayPointList[i].Number, (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY));
			else
		  		wsprintf(Buffer, TEXT("%d:%d%s"), WayPointList[i].Number, (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), sAltUnit);
		  } else
		  	wsprintf(Buffer, TEXT("%d:%d"), WayPointList[i].Number, (int)(WayPointCalc[i].GR));
		  if ( (MapBox == (MapBox_t)mbBoxed) || (MapBox == (MapBox_t)mbBoxedNoUnit)) {
			  TextDisplayMode.Border = 1;
			  TextDisplayMode.WhiteBold = 0;
		  } else
		  	TextDisplayMode.WhiteBold = 1; // outlined 
	      		TextDisplayMode.Color=RGB_WHITE; 
		} else {
		  wsprintf(Buffer, TEXT("%d"),WayPointList[i].Number);
		  if (islandable && isairport) {
		    TextDisplayMode.WhiteBold = 1; // outlined 
	      		TextDisplayMode.Color=RGB_WHITE; 
		  }
		}
		break;
				  
				  

	      case DISPLAYNAMEIFINTASK: 
		dowrite = intask;
		if (intask) {
		  if (draw_alt) {
		  if ( ArrivalValue == (ArrivalValue_t) avAltitude ) {
			if ( (MapBox == (MapBox_t)mbUnboxedNoUnit) || (MapBox == (MapBox_t)mbBoxedNoUnit) )
			    wsprintf(Buffer, TEXT("%s:%d"),
				     WayPointList[i].Name, 
				     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY));
			else
			    wsprintf(Buffer, TEXT("%s:%d%s"),
				     WayPointList[i].Name, 
				     (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
				     sAltUnit);


		  } else
			    wsprintf(Buffer, TEXT("%s:%d"),
				     WayPointList[i].Name, 
				     (int)(WayPointCalc[i].GR)); 

		  if ( (MapBox == (MapBox_t)mbBoxed) || (MapBox == (MapBox_t)mbBoxedNoUnit)) {
			  TextDisplayMode.Border = 1;
			  TextDisplayMode.WhiteBold = 0;
		  } else
		  	TextDisplayMode.WhiteBold = 1; // outlined 
	      		TextDisplayMode.Color=RGB_WHITE; 

		  }
		  else {
		    wsprintf(Buffer, TEXT("%s"),WayPointList[i].Name);
		    // TODO CHECK THIS, UNTESTED..
                    if (islandable && isairport) {
		       TextDisplayMode.WhiteBold = 1; // outlined 
	      		TextDisplayMode.Color=RGB_WHITE; 
		    }
		  }
		}
			else {
		  		wsprintf(Buffer, TEXT(" "));
				dowrite=true;
			}
		break;
	      case DISPLAYNONE:
		dowrite = (DeclutterLabels<MAPLABELS_ONLYTOPO) || intask || islandable;  // 100711
		if (draw_alt) {
		  if ( ArrivalValue == (ArrivalValue_t) avAltitude ) {
			if ( (MapBox == (MapBox_t)mbUnboxedNoUnit) || (MapBox == (MapBox_t)mbBoxedNoUnit) )
			  wsprintf(Buffer, TEXT("%d"), 
				   (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY));
			else
			  wsprintf(Buffer, TEXT("%d%s"), 
				   (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY), 
				   sAltUnit);
		  } else
			  wsprintf(Buffer, TEXT("%d"), 
				   (int)(WayPointCalc[i].GR) );

		  if ( (MapBox == (MapBox_t)mbBoxed) || (MapBox == (MapBox_t)mbBoxedNoUnit)) {
			  TextDisplayMode.Border = 1;
			  TextDisplayMode.WhiteBold = 0;
		  } else
		  	TextDisplayMode.WhiteBold = 1; // outlined 
	      	    TextDisplayMode.Color=RGB_WHITE; 
		}
		else {
		  	wsprintf(Buffer, TEXT(" "));
		}
		break;

	      default:
#if (WINDOWSPC<1)
		//ASSERT(0);
#endif
		break;

	      } // end intask/irange/dowrite

    if (MapWindow::zoom.RealScale()<20 && islandable && dowrite) {
      TextInBox(hdc, Buffer, WayPointList[i].Screen.x+5, WayPointList[i].Screen.y, 0, &TextDisplayMode, true); 
      dowrite=false; // do not pass it along
    }

		// Do not show takeoff for gliders, check TakeOffWayPoint 
		if (i==RESWP_TAKEOFF) {
			if (TakeOffWayPoint) {
				intask=false; // 091031 let TAKEOFF be decluttered
				WayPointList[i].Visible=TRUE;
			} else {
				WayPointList[i].Visible=FALSE;
				dowrite=false;
			}
		}

		if(i==RESWP_OPTIMIZED) {
			dowrite = DoOptimizeRoute();
		}


	      if (dowrite) { 
          MapWaypointLabelAdd(
                  Buffer,
                  WayPointList[i].Screen.x+5,
                  WayPointList[i].Screen.y,
                  &TextDisplayMode,
                  (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                  intask,islandable,isairport,excluded,i,WayPointList[i].Style);
	      }
	    } // end if intask
      
#ifdef HAVEEXCEPTIONS
	  }__finally
#endif
	     { ; }
	} // if visible
    } // for all waypoints
 
  qsort(&MapWaypointLabelList, MapWaypointLabelListCount, sizeof(MapWaypointLabel_t), MapWaypointLabelListCompare);

  int j;

  // now draw task/landable waypoints in order of range (closest last)
  // writing unconditionally

  for (j=MapWaypointLabelListCount-1; j>=0; j--){

    MapWaypointLabel_t *E = &MapWaypointLabelList[j];

    // draws if they are in task unconditionally,
    // otherwise, does comparison
    if ( E->inTask || (E->isLandable && !E->isExcluded) ) { 

	TextInBox(hdc, E->Name, E->Pos.x, E->Pos.y, 0, &(E->Mode), false); 

	// At low zoom, dont print the bitmap because drawn task would make it look offsetted
	if(MapWindow::zoom.RealScale() > 2) continue;

    // If we are at low zoom, use a dot for icons, so we dont clutter the screen
    if(MapWindow::zoom.RealScale() > 1) {
	if (BlackScreen) 
 		 SelectObject(hDCTemp,hInvSmall);
	else
 		 SelectObject(hDCTemp,hSmall);
    } else {
	if (BlackScreen)
		SelectObject(hDCTemp,hInvTurnPoint);
	else
		SelectObject(hDCTemp,hTurnPoint);
    }
    DrawBitmapX(hdc,
	    E->Pos.x-10,
	    E->Pos.y-10,
	    20,20,
	    hDCTemp,0,0,SRCPAINT,false);
        
    DrawBitmapX(hdc,
	    E->Pos.x-10,
	    E->Pos.y-10,
	    20,20,
	    hDCTemp,20,0,SRCAND,false);

    } // wp in task
  } // for all waypoint, searching for those in task

  // now draw normal waypoints in order of range (furthest away last)
  // without writing over each other (or the task ones)
  for (j=0; j<MapWaypointLabelListCount; j++) {

    MapWaypointLabel_t *E = &MapWaypointLabelList[j];

    if (!E->inTask && !E->isLandable ) {
      if ( TextInBox(hdc, E->Name, E->Pos.x, E->Pos.y, 0, &(E->Mode), true) == true) {

	// If we are at low zoom, use a dot for icons, so we dont clutter the screen
	if(MapWindow::zoom.RealScale() > 4) {
		if (BlackScreen) 
	 		 SelectObject(hDCTemp,hInvSmall);
		else
	 		 SelectObject(hDCTemp,hSmall);
	} else {
		// We switch all styles in the correct order, to force a jump table by the compiler
		// It would be much better to use an array of bitmaps, but no time to do it for 3.0
		switch(E->style) {
			case STYLE_NORMAL:
				goto turnpoint;
				break;

			// These are not used here in fact
			case STYLE_AIRFIELDGRASS:
			case STYLE_OUTLANDING:
			case STYLE_GLIDERSITE:
			case STYLE_AIRFIELDSOLID:
				goto turnpoint;
				break;

			case STYLE_MTPASS:
				SelectObject(hDCTemp,hMountpass);
				break;

			case STYLE_MTTOP:
				SelectObject(hDCTemp,hMountop);
				break;

			case STYLE_SENDER:
				SelectObject(hDCTemp,hSender);
				break;

			case STYLE_VOR:
				goto turnpoint;
				break;

			case STYLE_NDB:
				SelectObject(hDCTemp,hNdb);
				break;

			case STYLE_COOLTOWER:
				goto turnpoint;
				break;

			case STYLE_DAM:
				SelectObject(hDCTemp,hDam);
				break;

			case STYLE_TUNNEL:
				goto turnpoint;
				break;

			case STYLE_BRIDGE:
				SelectObject(hDCTemp,hBridge);
				break;

			case STYLE_POWERPLANT:
			case STYLE_CASTLE:
				goto turnpoint;
				break;

			case STYLE_INTERSECTION:
				SelectObject(hDCTemp,hIntersect);
				break;

			case STYLE_TRAFFIC:
				goto turnpoint;
				break;
			case STYLE_THERMAL:
				SelectObject(hDCTemp,hLKThermal);
				break;

			case STYLE_MARKER:
				SelectObject(hDCTemp,hBmpMarker);
				break;

			default:
turnpoint:
				if (BlackScreen)
					SelectObject(hDCTemp,hInvTurnPoint);
				else
					SelectObject(hDCTemp,hTurnPoint);
				break;

		} // switch estyle
	} // below zoom threshold

	// We dont do stretching here. We are using different bitmaps for hi res.
	// The 20x20 size is large enough to make much bigger icons than the old ones.
	DrawBitmapX(hdc,
		    E->Pos.x-10,
		    E->Pos.y-10,
		    20,20,
		    hDCTemp,0,0,SRCPAINT,false);
        
	DrawBitmapX(hdc,
		    E->Pos.x-10,
		    E->Pos.y-10,
		    20,20,
		    hDCTemp,20,0,SRCAND,false);
      }
    }
  }

} // end DrawWaypoint



void DrawRunway(const HDC hdc,const WAYPOINT* wp, const RECT rc, const double scale)
{
  int solid= false;
  HPEN    oldPen  ;
  HBRUSH  oldBrush ;
  bool bGlider = false;
  bool bOutland = false;
  bool bRunway = false;
  static double rwl = 8;
  static double rwb = 1;
  static double cir = 6;
  int l,p,b;

  double fScaleFact=scale;
  static double maxscale=6000.0;
  static double minscale=2500.0;
  static double resolution_factor=2000.0;

  #if 0 // For reference only
  /* 
   On 800x480 only.
   Km Zoom  Scale
    10.0     943
     7.5    1245 
     5.0    1868
     3.5    2668
     2.0    4670
     1.5    6226
     1.0    9340
     0.75  12453

   Nm Zoom  Scale
     3.5    1441
     2.0    2521
     etc.etc.

     We use Scale values, and they must be tuned for each resolution
     also considering screen size, which is normally 3.5 inches at low resolutions
     and 5 inches at high resolution screens.

     StartupStore(_T("scale=%.1f \n"),scale);
  */
  #endif


  if (DoInit[MDI_MAPWPVECTORS])
  {
    switch(ScreenSize)
    {
      //
      // portrait
      //
      case ss240x320: 
		rwl = 8.0; rwb = 2.0;cir = 4.0; break;
      case ss240x400: 
		rwl = 8.0; rwb = 2.0;cir = 4.0; break;
      case ss272x480: 
		rwl = 8.0; rwb = 2.0;cir = 4.0; break;
      case ss480x640: 
		rwl = 6.0; rwb = 1.0;cir = 5.0; break;
      case ss480x800: 
		rwl = 6.0; rwb = 1.2;cir = 5.0; break;
      //
      // landscape
      //
      case ss320x240: 
		rwl = 8.0; rwb = 2.0;cir = 4.0; break;
      case ss400x240: 
		rwl = 7.0; rwb = 1.0;cir = 4.0; break;
      case ss480x234: 
		rwl = 7.0; rwb = 1.0;cir = 4.0; break;
      case ss480x272: 
		maxscale=9000.0;
		minscale=2000.0;
		resolution_factor=2000.0;
		rwl = 8.0; rwb = 2.0;cir = 5.0; break;
      case ss640x480: 
		rwl = 6.0; rwb = 1.0;cir = 5.0; break;
      case ss800x480: 
		maxscale=9000.0;
		minscale=2000.0;
		resolution_factor=2000.0;
		rwl = 6.0; rwb = 1.2;cir = 5.0; break;
      case ss896x672: 
		rwl = 6.0; rwb = 1.5;cir = 5.0; break;
      default: break;
    }
    DoInit[MDI_MAPWPVECTORS]=false;
  }

  if(fScaleFact > maxscale)	fScaleFact = maxscale; // limit to prevent huge airfiel symbols
  if(fScaleFact <= minscale)	fScaleFact = minscale; // limit to prevent tiny airfiel symbols
  fScaleFact /= resolution_factor;

  // use minimum runway lenght, threshold limit, and this must be visible!
  #define WPR_LOW 500
  #define WPR_MID 750
  #define WPR_HIGH 1500
  int wpr=wp->RunwayLen;

  if (wpr>WPR_HIGH) wpr=WPR_HIGH;
  if (wpr==0) wpr=WPR_MID;;
  if (wpr<WPR_LOW) wpr=WPR_LOW;

  l = (int) (rwl * (1.0+ ((double)wpr/(double)WPR_MID-1.0)/4.0)); // 4?? why 4?
  //l = (int) (rwl * (1.0+ ((double)wpr/(double)WPR_MID-1.0)/cir));

  l = (int)(l * fScaleFact); if(l==0) l=1;
  b = (int)(rwb  * fScaleFact); if(b==0) b=1;
  p = (int)(cir * 2.0 * fScaleFact); if(p==0) p=1;


  // Reminder: DAT files will show only outlanding and airfieldsolid
  switch(wp->Style) {
	case STYLE_AIRFIELDSOLID: solid = true;  bRunway  = true;  bOutland = false;  bGlider  = false;	break;
	case STYLE_AIRFIELDGRASS: solid = false; bRunway  = true;  bOutland = false;  bGlider  = false;	break;
	case STYLE_OUTLANDING	: solid = false; bRunway  = true;  bOutland = true;   bGlider  = false;	break;
	case STYLE_GLIDERSITE	: solid = false; bRunway  = true;  bOutland = false;  bGlider  = true;	break;
	default: return; break;
  }

  int Brg =0;
  if (  DisplayOrientation == TRACKUP ) Brg = (int)GPS_INFO.TrackBearing;


  // Defaults: thin black pen and red brush. 
  // We need to save olds, so we set them here.
  oldPen   = (HPEN) SelectObject(hdc, LKPen_Black_N0);
  oldBrush = (HBRUSH)SelectObject(hdc, LKBrush_Red);

  if( wp->Reachable == TRUE) SelectObject(hdc, LKBrush_Green);

  if (bOutland) {
	#if 0
	if( wp->Reachable == TRUE) {
		SelectObject(hdc, LKPen_Green_N1);
	} else {
		SelectObject(hdc, LKPen_Red_N1);
	}
	#else
	SelectObject(hdc, LKPen_Black_N0);
	#endif
	Circle( hdc, wp->Screen.x, wp->Screen.y, p,  rc,true, false);
  } else {
	if (solid)
		SelectObject(hdc, LKPen_Black_N1);
	Circle( hdc, wp->Screen.x, wp->Screen.y, p,  rc,true, true);
  }

  if(bRunway)
  {
	if( (wp->RunwayDir >=0) &&   ( wp->RunwayLen> 0))
	{
		POINT Runway[5] = {
		  { b, l },  // 1
		  {-b, l },  // 2
		  {-b,-l },  // 3
		  { b,-l },  // 4
		  { b,l  }   // 5
		};

		// Currently we use thicker line for solid airfields
		#if 0
		if(!bOutland)
		{
			// NOTICE: GREY IS NOT ENOUGH VISIBLE IN THE SUN
			// Choices: BLACK for solid, white for not solid
			if(solid)
				SelectObject(hdc, LKBrush_Grey );
			else
				SelectObject(hdc, LKBrush_White);
		}
		#else
		if(!bOutland) SelectObject(hdc, LKBrush_White);
		#endif
		SelectObject(hdc, LKPen_Black_N0);
		PolygonRotateShift(Runway, 5,  wp->Screen.x, wp->Screen.y,  wp->RunwayDir-Brg);
		Polygon(hdc,Runway ,5 );
	}
  } // bRunway

  // THIS IS UNTESTED
  if(fScaleFact >= 0.9)
  {
	if(bGlider)
	{
	    int iScale = l/3; if(iScale==0) iScale=1;
	    POINT WhiteWing [15]  = {
	      { 0 * iScale, 0 * iScale },   // 1
	      { 1 * iScale,-1 * iScale },   // 2
	      { 2 * iScale,-1 * iScale },   // 3
	      { 3 * iScale, 0 * iScale },   // 4
	      { 3 * iScale, 1 * iScale },   // 5
	      { 2 * iScale, 0 * iScale },   // 6
	      { 1 * iScale, 0 * iScale },   // 7
	      { 0 * iScale, 1 * iScale },   // 8
	      {-1 * iScale, 0 * iScale },   // 9
	      {-2 * iScale, 0 * iScale },   // 10
	      {-3 * iScale, 1 * iScale },   // 11
	      {-3 * iScale, 0 * iScale },   // 12
	      {-2 * iScale,-1 * iScale },   // 13
	      {-1 * iScale,-1 * iScale },   // 14
	      { 0 * iScale, 0 * iScale }    // 15
	    };

	    PolygonRotateShift(WhiteWing, 15,  wp->Screen.x, wp->Screen.y,  0/*+ wp->RunwayDir-Brg*/);
	    Polygon(hdc,WhiteWing ,15 );
	  }

  } // fScaleFact>=0.9

  SelectObject(hdc, oldPen);
  SelectObject(hdc, oldBrush);

#ifdef PRINT_FREQUENCY
  // Probably we should use TextInBox and/or something decluttering labels, or use it only at very high zooms
  if (MapWindow::zoom.RealScale()<5.4)
  {
	SIZE tsize;
	SetTextColor(hdc, RGB_BLACK);
	HFONT hfOld = (HFONT)SelectObject(hdc, LK8PanelSmallFont);
	GetTextExtentPoint(hdc, wp->Freq, _tcslen(wp->Freq), &tsize);
	ExtTextOut(hdc,wp->Screen.x/*-tsize.cx/2*/ ,wp->Screen.y+(15), ETO_OPAQUE, NULL, wp->Freq, _tcslen( wp->Freq), NULL);
	SelectObject(hdc, hfOld);
  }
#endif

}


