// Copyright (c) 2014 Joseph D Poirier
// Distributable under the terms of The New BSD License
// that can be found in the LICENSE file.

#ifdef _WIN32 /* this is true for 64 bit as well */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#ifdef _APPLE_
#include <OpenGl/gl.h>
#else
#include <GL/gl.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "XPLMDefs.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"

#include "defs.h"
#include "commviewer.h"

static int CommandHandler(XPLMCommandRef inCommand,
                          XPLMCommandPhase inPhase,
                          void* inRefcon);

static void DrawWindowCallback(XPLMWindowID inWindowID,
                               void* inRefcon);

static void HandleKeyCallback(XPLMWindowID inWindowID,
                              char inKey,
                              XPLMKeyFlags inFlags,
                              char inVirtualKey,
                              void* inRefcon,
                              int losingFocus);

static int HandleMouseCallback(XPLMWindowID inWindowID,
                               int x,
                               int y,
                               XPLMMouseStatus inMouse,
                               void* inRefcon);

static void HotKeyCallback(void* inRefcon);

static XPLMWindowID gWindow = NULL;
static bool gPluginEnabled = false;
static int gPlaneLoaded = 0;
static const float FL_CB_INTERVAL = -1.0;
static bool gPTT_On = false;
static XPLMHotKeyID gHotKey = NULL;

#define WINDOW_WIDTH (200)
#define WINDOW_HEIGHT (40)
static int gWinPosX;
static int gWinPosY;
static int gLastMouseX;
static int gLastMouseY;

// general & misc
enum {
    PLUGIN_PLANE_ID = 0,
    CMD_CONTACT_ATC,
};

// Command Refs
#define sCONTACT_ATC   "sim/operation/contact_atc"

XPLMDataRef avionics_power_on_dataref;
XPLMDataRef audio_selection_com1_dataref;
XPLMDataRef audio_selection_com2_dataref;

XPLMDataRef artificial_stability_on_dataref;
XPLMDataRef artificial_stability_pitch_on_dataref;
XPLMDataRef artificial_stability_roll_on_dataref;

#ifdef TOGGLE_TEST_FEATURE
XPLMDataRef visibility_reported_m_dataref;
XPLMDataRef cloud_coverage_1_dataref;
XPLMDataRef cloud_coverage_2_dataref;
XPLMDataRef cloud_coverage_3_dataref;
#endif

XPLMDataRef panel_visible_win_t_dataref;

/*
 *
 *
 */
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc) {

    LPRINTF("CommView Plugin: XPluginStart\n");
    strcpy(outName, "CommView");
    strcpy(outSig , "jdp.comm.view");
    strcpy(outDesc, "CommView Plugin.");

    // sim/cockpit/switches/audio_panel_out

    // sim/cockpit2/radios/actuators/audio_com_selection
    // sim/cockpit2/radios/actuators/audio_selection_com1 int   y   boolean is com1 selected for listening
    // sim/cockpit2/radios/actuators/audio_selection_com2 int   y   boolean is com2 selected for listening
    // sim/cockpit2/radios/actuators/com1_power int y   boolean Com radio 1 off or on, 0 or 1.
    // sim/cockpit2/radios/actuators/com2_power int y   boolean Com radio 2 off or on, 0 or 1.

    //avionics_power_on_dataref = XPLMFindDataRef("sim/cockpit2/switches/avionics_power_on");
    //LPRINTF("CommView Plugin: data/command refs initialized\n");

    audio_selection_com1_dataref = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_com1");
    audio_selection_com2_dataref = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_com2");

#ifdef TOGGLE_TEST_FEATURE
    artificial_stability_on_dataref = XPLMFindDataRef("sim/cockpit2/switches/artificial_stability_on");
    artificial_stability_pitch_on_dataref = XPLMFindDataRef("sim/cockpit2/switches/artificial_stability_pitch_on");
    artificial_stability_roll_on_dataref = XPLMFindDataRef("sim/cockpit2/switches/artificial_stability_roll_on");
    XPLMSetDatai(artificial_stability_on_dataref, 1);
    XPLMSetDatai(artificial_stability_pitch_on_dataref, 1);
    XPLMSetDatai(artificial_stability_roll_on_dataref, 1);
#endif

    XPLMCommandRef cmd_ref;
    cmd_ref = XPLMCreateCommand(sCONTACT_ATC, "Contact ATC");
    XPLMRegisterCommandHandler(cmd_ref, CommandHandler, CMD_HNDLR_EPILOG, (void*)CMD_CONTACT_ATC);

    // XPLMRegisterFlightLoopCallback(FlightLoopCallback, FL_CB_INTERVAL, NULL);
    panel_visible_win_t_dataref = XPLMFindDataRef("sim/graphics/view/panel_visible_win_t");

    int top = (int)XPLMGetDataf(panel_visible_win_t_dataref);
    gWinPosX = 0;
    gWinPosY = top - 200;
    gWindow = XPLMCreateWindow(gWinPosX,                // left
                               gWinPosY,                // top
                               gWinPosX+WINDOW_WIDTH,   // right
                               gWinPosY-WINDOW_HEIGHT,  // bottom
                               true,                    // is visible
                               DrawWindowCallback,
                               HandleKeyCallback,
                               HandleMouseCallback,
                               NULL);                   // Refcon

#ifdef TOGGLE_TEST_FEATURE
    visibility_reported_m_dataref = XPLMFindDataRef("sim/weather/visibility_reported_m");
    cloud_coverage_1_dataref = XPLMFindDataRef("sim/weather/cloud_coverage[0]");
    cloud_coverage_2_dataref = XPLMFindDataRef("sim/weather/cloud_coverage[1]");
    cloud_coverage_3_dataref = XPLMFindDataRef("sim/weather/cloud_coverage[2]");
    gHotKey = XPLMRegisterHotKey(XPLM_VK_F3,
                                 xplm_DownFlag,
                                 "Toggle IFR COnditions'",
                                 HotKeyCallback,
                                 NULL);
#endif

    LPRINTF("CommView Plugin: startup completed\n");

    return PROCESSED_EVENT;
}

#ifdef TOGGLE_TEST_FEATURE
/*
 *
 *
 */
void HotKeyCallback(void* inRefcon) {
    static bool isSaved = false;
    static float visibility_reported = 0.0;
    static float cloud_coverage_1 = 0.0;
    static float cloud_coverage_2 = 0.0;
    static float cloud_coverage_3 = 0.0;

    if (isSaved) {
        isSaved = false;
        XPLMSetDataf(visibility_reported_m_dataref, visibility_reported);
        XPLMSetDataf(cloud_coverage_1_dataref, cloud_coverage_1);
        XPLMSetDataf(cloud_coverage_2_dataref, cloud_coverage_2);
        XPLMSetDataf(cloud_coverage_3_dataref, cloud_coverage_3);
        visibility_reported = cloud_coverage_1 = cloud_coverage_2 = cloud_coverage_3 = 0.0;
    } else {
        isSaved = true;
        visibility_reported = XPLMGetDataf(visibility_reported_m_dataref);
        XPLMSetDataf(visibility_reported_m_dataref, 0.0);

        cloud_coverage_1 = XPLMGetDataf(cloud_coverage_1_dataref);
        cloud_coverage_2 = XPLMGetDataf(cloud_coverage_2_dataref);
        cloud_coverage_3 = XPLMGetDataf(cloud_coverage_3_dataref);
        XPLMSetDataf(cloud_coverage_1_dataref, 6.0);
        XPLMSetDataf(cloud_coverage_2_dataref, 6.0);
        XPLMSetDataf(cloud_coverage_3_dataref, 6.0);
    }
}
#endif

/*
 *
 *
 */
float FlightLoopCallback(float inElapsedSinceLastCall,
                         float inElapsedTimeSinceLastFlightLoop,
                         int inCounter,
                         void* inRefcon) {

//   if ((gFlCbCnt % PANEL_CHECK_INTERVAL) == 0) {
//   }

    if (!gPluginEnabled) {

    }

    return 1.0;
}

/*
 *
 *
 */
int CommandHandler(XPLMCommandRef inCommand,
                   XPLMCommandPhase inPhase,
                   void* inRefcon) {

//    if ((gFlCbCnt % PANEL_CHECK_INTERVAL) == 0) {
//    }
//    if (!gPluginEnabled) {
//        return IGNORED_EVENT;
//    }

    switch (reinterpret_cast<uint32_t>(inRefcon)) {
    case CMD_CONTACT_ATC:
        switch (inPhase) {
        case xplm_CommandBegin:
        case xplm_CommandContinue:
            gPTT_On = true;
            break;
        case xplm_CommandEnd:
        default:
            gPTT_On = false;
            break;
        }
        break;
    default:
        break;
    }

    return IGNORED_EVENT;
}

/*
 *
 */
PLUGIN_API void XPluginStop(void) {

    gPluginEnabled = false;
    //XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL);
    LPRINTF("CommView Plugin: XPluginStop\n");
}

/*
 *
 */
PLUGIN_API void XPluginDisable(void) {

    gPluginEnabled = false;
    LPRINTF("CommView Plugin: XPluginDisable\n");
}

/*
 *
 */
PLUGIN_API int XPluginEnable(void) {

    gPluginEnabled = true;
    LPRINTF("CommView Plugin: XPluginEnable\n");

    return PROCESSED_EVENT;
}

/*
 *
 */
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom,
                                      long inMsg,
                                      void* inParam) {

    if (inFrom == XPLM_PLUGIN_XPLANE) {
        int inparam = reinterpret_cast<int>(inParam);
        switch (inMsg) {
        case XPLM_MSG_PLANE_LOADED:
            if (inparam != PLUGIN_PLANE_ID || gPlaneLoaded) { break; }
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_LOADED\n");
            break;
        case XPLM_MSG_AIRPORT_LOADED:
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_AIRPORT_LOADED\n");
            break;
        case XPLM_MSG_SCENERY_LOADED:
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_SCENERY_LOADED\n");
            break;
        case XPLM_MSG_AIRPLANE_COUNT_CHANGED:
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_AIRPLANE_COUNT_CHANGED\n");
            break;
        // XXX: system state and procedure, what's difference between an unloaded and crashed plane?
        case XPLM_MSG_PLANE_CRASHED:
            if ((int)inParam != PLUGIN_PLANE_ID) { break; }
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_CRASHED\n");
            break;
        case XPLM_MSG_PLANE_UNLOADED:
            if ((int)inParam != PLUGIN_PLANE_ID) { break; }
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_UNLOADED\n");
            break;
        default: // unknown
            break;
        } // switch (inMsg)
    } // if (inFrom == XPLM_PLUGIN_XPLANE)
}

/*
 *
 *
 */
void DrawWindowCallback(XPLMWindowID inWindowID, void* inRefcon) {

    int left;
    int top;
    int right;
    int bottom;
    static char str[100];
    static float color[] = { 1.0, 1.0, 1.0 };    // RGB White

    //int top = (int)XPLMGetDataf(panel_visible_win_t_dataref);
    //XPLMDrawTranslucentDarkBox(0, top-200, 300, top-200-50);

    // location of the window and draw the window
    XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);
    XPLMDrawTranslucentDarkBox(left, top, right, bottom);

    // put the text into the window, NULL indicates no word wrap
#if 1
    sprintf(str,"%s\t\t\tCOM1: %d\t\t\tCOM2: %d",
        (char*)(gPTT_On ? "PTT: ON" : "PTT: OFF"),
        XPLMGetDatai(audio_selection_com1_dataref),
        XPLMGetDatai(audio_selection_com2_dataref));

    XPLMDrawString(color, left+5, top-20, str, NULL, xplmFont_Basic);
#else
     XPLMDrawString(color,
                    left+5,
                    top,
                    (char*)(gCounter ? "PTT: ON" : "PTT: OFF"),
                    NULL,
                    xplmFont_Basic);
#endif
    //glDisable(GL_TEXTURE_2D);
    //glColor3f(0.7, 0.7, 0.7);
    //glBegin(GL_LINES);
    //    glVertex2i(right-1, top-1);
    //    glVertex2i(right-7, top-7);
    //    glVertex2i(right-7, top-1);
    //    glVertex2i(right-1, top-7);
    //glEnd();
    //glEnable(GL_TEXTURE_2D);
}

/*
 *
 *
 */
void HandleKeyCallback(XPLMWindowID inWindowID,
                         char inKey,
                         XPLMKeyFlags inFlags,
                         char inVirtualKey,
                         void* inRefcon,
                         int losingFocus) {

    // nothing to do here
}

/*
 *
 *
 */
int HandleMouseCallback(XPLMWindowID inWindowID,
                        int x,
                        int y,
                        XPLMMouseStatus inMouse,
                        void* inRefcon) {

    static int com_changed = 0;
    static bool isDragging = false;

    switch (inMouse) {
    case xplm_MouseDown:
        // if ((x >= gWinPosX+WINDOW_WIDTH-8) &&
        //     (x <= gWinPosX+WINDOW_WIDTH) &&
        //     (y <= gWinPosY) && (y >= gWinPosY-8)) {
        //         windowCloseRequest = 1;
        //     } else {
                gLastMouseX = x;
                gLastMouseY = y;
        // }
        break;
    case xplm_MouseDrag:
        isDragging = true;
        gWinPosX += x - gLastMouseX;
        gWinPosY += y - gLastMouseY;
        XPLMSetWindowGeometry(gWindow,
                              gWinPosX,
                              gWinPosY,
                              gWinPosX+WINDOW_WIDTH,
                              gWinPosY-WINDOW_HEIGHT);
        gLastMouseX = x;
        gLastMouseY = y;
        break;
    case xplm_MouseUp:
        if (isDragging) {
            isDragging = false;
        } else {
            int com1 = XPLMGetDatai(audio_selection_com1_dataref);
            int com2 = XPLMGetDatai(audio_selection_com2_dataref);

            if (com1 && com2 && com_changed) {
                switch (com_changed) {
                case 1:
                    XPLMSetDatai(audio_selection_com1_dataref, 0);
                    break;
                case 2:
                    XPLMSetDatai(audio_selection_com2_dataref, 0);
                    break;
                default:
                    break;
                }
                com_changed = 0;
            } else if (!com1 && com2) {
                com_changed = 1;
                XPLMSetDatai(audio_selection_com1_dataref, 1);
            }  else if (com1 && !com2) {
                com_changed = 2;
                XPLMSetDatai(audio_selection_com2_dataref, 1);
            }
        }
        break;
    } // switch (inMouse)

    return PROCESSED_EVENT;
}
