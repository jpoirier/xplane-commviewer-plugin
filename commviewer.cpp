// Copyright (c) 2014 Joseph D Poirier
// Distributable under the terms of The New BSD License
// that can be found in the LICENSE file.

#ifdef _WIN32 /* this is set for 64 bit as well */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#ifdef _APPLE_
 #pragma clang diagnostic ignored "-Wall"
 #include <gl.h>
#else
 //#include <GL/gl.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "./SDK/CHeaders/XPLM/XPLMPlugin.h"
#include "./SDK/CHeaders/XPLM/XPLMProcessing.h"
#include "./SDK/CHeaders/XPLM/XPLMDataAccess.h"
#include "./SDK/CHeaders/XPLM/XPLMUtilities.h"
#include "./SDK/CHeaders/XPLM/XPLMDisplay.h"
#include "./SDK/CHeaders/XPLM/XPLMGraphics.h"

#include "./include/defs.h"
#include "./include/commviewer.h"

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

// To define, pass -DVERSION=vX.Y.X when building 
#ifndef VERSION
#define VERSION "vX.Y.Z" 
#endif

// sigh, two levels of macros are needed to stringify
//  the result  of expansion of a macro argument
#define DESC() STR(VERSION)
#define STR(v) "CommViewer " #v  " " __DATE__ " (jdpoirier@gmail.com)"

#pragma message (DESC())


#ifdef TOGGLE_TEST_FEATURE
static XPLMHotKeyID gHotKey = NULL;
static void HotKeyCallback(void* inRefcon);
#endif

static XPLMWindowID gCommWindow = NULL;
static bool gPluginEnabled = false;
static int gPlaneLoaded = 0;
static const float FL_CB_INTERVAL = -1.0;
static bool gPTT_On = false;
static bool gPilotEdgePlugin = false;

#define WINDOW_WIDTH (290)
#define WINDOW_HEIGHT (60)
static int gCommWinPosX;
static int gCommWinPosY;
static int gLastMouseX;
static int gLastMouseY;

// general & misc
enum {
    PLUGIN_PLANE_ID = 0
    ,CMD_CONTACT_ATC
    ,COMMVIEWER_WINDOW
#ifdef TOGGLE_TEST_FEATURE

#endif
};

// Command Refs
#define sCONTACT_ATC "sim/operation/contact_atc"

#define PILOTEDGE_SIG "com.pilotedge.plugin.xplane"

XPLMDataRef avionics_power_on_dataref;
XPLMDataRef audio_selection_com1_dataref;
XPLMDataRef audio_selection_com2_dataref;

XPLMDataRef pilotedge_rx_status_dataref = NULL;
XPLMDataRef pilotedge_tx_status_dataref = NULL;
XPLMDataRef pilotedge_connected_dataref = NULL;

XPLMDataRef radio_volume_ratio_dataref;

#ifdef TOGGLE_TEST_FEATURE
XPLMDataRef artificial_stability_on_dataref;
XPLMDataRef artificial_stability_pitch_on_dataref;
XPLMDataRef artificial_stability_roll_on_dataref;
#endif

XPLMDataRef panel_visible_win_t_dataref;

/*
 *
 *
 */
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
    LPRINTF("CommViewer Plugin: XPluginStart\n");
    strcpy(outName, "CommViewer");
    strcpy(outSig , "jdp.comm.viewer");
    strcpy(outDesc, DESC());

    // sim/cockpit/switches/audio_panel_out

    // sim/cockpit2/radios/actuators/audio_com_selection
    // sim/cockpit2/radios/actuators/audio_selection_com1 int   y   boolean is com1 selected for listening
    // sim/cockpit2/radios/actuators/audio_selection_com2 int   y   boolean is com2 selected for listening
    // sim/cockpit2/radios/actuators/com1_power int y   boolean Com radio 1 off or on, 0 or 1.
    // sim/cockpit2/radios/actuators/com2_power int y   boolean Com radio 2 off or on, 0 or 1.

    //avionics_power_on_dataref = XPLMFindDataRef("sim/cockpit2/switches/avionics_power_on");
    //LPRINTF("CommViewer Plugin: data/command refs initialized\n");

    audio_selection_com1_dataref = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_com1");
    audio_selection_com2_dataref = XPLMFindDataRef("sim/cockpit2/radios/actuators/audio_selection_com2");
    radio_volume_ratio_dataref = XPLMFindDataRef("sim/operation/sound/radio_volume_ratio");  // 0.0 - 1.0

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
    XPLMRegisterCommandHandler(cmd_ref,
                               CommandHandler,
                               CMD_HNDLR_EPILOG,
                               (void*)CMD_CONTACT_ATC);

    // XPLMRegisterFlightLoopCallback(FlightLoopCallback, FL_CB_INTERVAL, NULL);
    panel_visible_win_t_dataref = XPLMFindDataRef("sim/graphics/view/panel_visible_win_t");

    int top = (int)XPLMGetDataf(panel_visible_win_t_dataref);
    gCommWinPosX = 0;
    gCommWinPosY = top - 200;
    gCommWindow = XPLMCreateWindow(gCommWinPosX,                // left
                                   gCommWinPosY,                // top
                                   gCommWinPosX+WINDOW_WIDTH,   // right
                                   gCommWinPosY-WINDOW_HEIGHT,  // bottom
                                   true,                        // is visible
                                   DrawWindowCallback,
                                   HandleKeyCallback,
                                   HandleMouseCallback,
                                   (void*)COMMVIEWER_WINDOW);    // Refcon

    // printf("CommViewer, left:%d, right:%d, top:%d, bottom:%d\n",
    //     gCommWinPosX, gCommWinPosX+WINDOW_WIDTH, gCommWinPosY, gCommWinPosY-WINDOW_HEIGHT);

#ifdef TOGGLE_TEST_FEATURE
    //gHotKey = XPLMRegisterHotKey(XPLM_VK_F3,
    //                             xplm_DownFlag,
    //                             "Toggle Window Ice",
    //                             HotKeyCallback,
    //                             NULL);
#endif

    LPRINTF("CommViewer Plugin: startup completed\n");

    return PROCESSED_EVENT;
}

#ifdef TOGGLE_TEST_FEATURE
/*
 *
 *
 */
void HotKeyCallback(void* inRefcon)
{
    static bool isSaved = false;

    if (isSaved) {
        isSaved = false;
    } else {
        isSaved = true;
    }
}
#endif

/*
 *
 *
 */
float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
                         int inCounter, void* inRefcon)
{
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
int CommandHandler(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
//    if ((gFlCbCnt % PANEL_CHECK_INTERVAL) == 0) {
//    }
//    if (!gPluginEnabled) {
//        return IGNORED_EVENT;
//    }

    switch (reinterpret_cast<size_t>(inRefcon)) {
    case CMD_CONTACT_ATC:
        switch (inPhase) {
        case xplm_CommandBegin:
        case xplm_CommandContinue:
            gPTT_On = true;
            break;
        case xplm_CommandEnd:
            gPTT_On = false;
            break;
        default:
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
PLUGIN_API void XPluginStop(void)
{
    gPluginEnabled = false;
    //XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL);
    LPRINTF("CommViewer Plugin: XPluginStop\n");
}

/*
 *
 */
PLUGIN_API void XPluginDisable(void)
{
    gPluginEnabled = false;
    LPRINTF("CommViewer Plugin: XPluginDisable\n");
}

/*
 *
 */
PLUGIN_API int XPluginEnable(void)
{
    gPluginEnabled = true;
    LPRINTF("CommViewer Plugin: XPluginEnable\n");

    return PROCESSED_EVENT;
}

/*
 *
 */
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, long inMsg, void* inParam)
{
    if (inFrom == XPLM_PLUGIN_XPLANE) {
        // size_t inparam = reinterpret_cast<size_t>(inParam);
        switch (inMsg) {
        case XPLM_MSG_PLANE_LOADED:
            gPlaneLoaded = true;
            LPRINTF("CommViewer Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_LOADED\n");
            break;
        case XPLM_MSG_AIRPORT_LOADED:
            LPRINTF("CommViewer Plugin: XPluginReceiveMessage XPLM_MSG_AIRPORT_LOADED\n");
            break;
        case XPLM_MSG_SCENERY_LOADED:
            LPRINTF("CommViewer Plugin: XPluginReceiveMessage XPLM_MSG_SCENERY_LOADED\n");
            break;
        case XPLM_MSG_AIRPLANE_COUNT_CHANGED:
            LPRINTF("CommViewer Plugin: XPluginReceiveMessage XPLM_MSG_AIRPLANE_COUNT_CHANGED\n");
            break;
        case XPLM_MSG_PLANE_CRASHED:
            // XXX: system state and procedure, what's difference between
            // an unloaded and crashed plane?
            LPRINTF("CommViewer Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_CRASHED\n");
            break;
        case XPLM_MSG_PLANE_UNLOADED:
            gPlaneLoaded = false;
            LPRINTF("CommViewer Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_UNLOADED\n");
            break;
        default:
            // unknown, anything to do?
            break;
        } // switch (inMsg)
    } // if (inFrom == XPLM_PLUGIN_XPLANE)
}

/*
 *
 *
 */
void DrawWindowCallback(XPLMWindowID inWindowID, void* inRefcon)
{
    int left;
    int top;
    int right;
    int bottom;
    int rx_status;
    int tx_status;
    char* connected;
    static char str1[100];
    static char str2[100];
    static float commviewer_color[] = {1.0, 1.0, 1.0};  // RGB White

    if (inWindowID != gCommWindow)
        return;

    // XXX: are inWindowIDs our XPLMCreateWindow return pointers
    XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);
    // printf("CommViewer, gCommWindow: %p, inWindowID: %p, left:%d, right:%d, top:%d, bottom:%d\n",
    //     gCommWindow, inWindowID, left, right, top, bottom);
    XPLMDrawTranslucentDarkBox(left, top, right, bottom);

    if (!gPilotEdgePlugin) {
        if ((XPLMFindPluginBySignature(PILOTEDGE_SIG)) != XPLM_NO_PLUGIN_ID) {
            gPilotEdgePlugin = true;
            pilotedge_rx_status_dataref = XPLMFindDataRef("pilotedge/radio/rx_status");
            pilotedge_tx_status_dataref = XPLMFindDataRef("pilotedge/radio/tx_status");
            pilotedge_connected_dataref = XPLMFindDataRef("pilotedge/status/connected");
        }
    }

    switch (reinterpret_cast<size_t>(inRefcon)) {
    case COMMVIEWER_WINDOW:
#if 0
        sprintf(str1,"%s\t\t\tCOM1: %d\t\t\tCOM2: %d",
                (char*)(gPTT_On ? "PTT: ON" : "PTT: OFF"),
                XPLMGetDatai(audio_selection_com1_dataref),
                XPLMGetDatai(audio_selection_com2_dataref));
        // text to window, NULL indicates no word wrap
        XPLMDrawString(commviewer_color,
                       left+5,
                       top-20,
                       str1,
                       NULL,
                       xplmFont_Basic);
#else
        rx_status = (pilotedge_rx_status_dataref ? XPLMGetDatai(pilotedge_rx_status_dataref) : false) ? 1 : 0;
        tx_status = (pilotedge_tx_status_dataref ? XPLMGetDatai(pilotedge_tx_status_dataref) : false) ? 1 : 0;
        connected = (pilotedge_connected_dataref ? XPLMGetDatai(pilotedge_connected_dataref) : false) ? (char*)"YES" : (char*)"NO ";

        sprintf(str1, "[PilotEdge] Connected: %s \t\t\tTX: %d\t\t\tRX: %d",
                connected,
                tx_status,
                rx_status);

        sprintf(str2,"%s\t\t\tCOM1: %d\t\t\tCOM2: %d",
                (char*)(gPTT_On ? "PTT: ON " : "PTT: OFF"),
                XPLMGetDatai(audio_selection_com1_dataref),
                XPLMGetDatai(audio_selection_com2_dataref));

        // text to window, NULL indicates no word wrap
        XPLMDrawString(commviewer_color,
                       left+4,
                       top-20,
                       str1,
                       NULL,
                       xplmFont_Basic);

        XPLMDrawString(commviewer_color,
                       left+4,
                       top-40,
                       str2,
                       NULL,
                       xplmFont_Basic);
#endif
        break;
    default:
        break;
    }

#ifdef TOGGLE_TEST_FEATURE
    GLfloat x1 = (GLfloat)(right + 5);
    GLfloat y1 = (GLfloat)(top - 5);
    GLfloat x2;
    GLfloat y2;
    GLfloat radius = 0.1f;

    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 0.6f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x1, y1);  // x & y
        for (GLfloat angle = 1.0f; angle < 361.0f; angle += 0.2f) {
            x2 = x1 + (GLfloat)sin(angle) * radius;
            y2 = y1 + (GLfloat)cos(angle) * radius;
            glVertex2f(x2, y2);
        }
    glEnd();
    glEnable(GL_TEXTURE_2D);


    //glDisable(GL_TEXTURE_2D);
    //glColor3f(0.7, 0.7, 0.7);
    //glBegin(GL_LINES);
    //    glVertex2i(right-1, top-1);
    //    glVertex2i(right-7, top-7);
    //    glVertex2i(right-7, top-1);
    //    glVertex2i(right-1, top-7);
    //glEnd();
    //glEnable(GL_TEXTURE_2D);
#endif
}

/*
 *
 *
 */
void HandleKeyCallback(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags,
                       char inVirtualKey, void* inRefcon, int losingFocus)
{
    if (inWindowID != gCommWindow)
        return;
}

/*
 *
 *
 */
 #define COMMS_UNCHANGED    (0)
 #define COM1_CHANGED       (1)
 #define COM2_CHANGED       (2)
 #define COMM_UNSELECTED    (0)
 #define COMM_SELECTED      (1)
int HandleMouseCallback(XPLMWindowID inWindowID, int x, int y, XPLMMouseStatus inMouse, void* inRefcon)
{
    static int com_changed = COMMS_UNCHANGED;
    static int MouseDownX;
    static int MouseDownY;

    if (inWindowID != gCommWindow)
        return IGNORED_EVENT;

    switch (inMouse) {
    case xplm_MouseDown:
        // if ((x >= gCommWinPosX+WINDOW_WIDTH-8) &&
        //     (x <= gCommWinPosX+WINDOW_WIDTH) &&
        //     (y <= gCommWinPosY) && (y >= gCommWinPosY-8)) {
        //         windowCloseRequest = 1;
        //     } else {
                MouseDownX = gLastMouseX = x;
                MouseDownY = gLastMouseY = y;
        // }
        break;
    case xplm_MouseDrag:
        // this event fires while xplm_MouseDown
        // and whether the window is being dragged or not
        gCommWinPosX += (x - gLastMouseX);
        gCommWinPosY += (y - gLastMouseY);
        XPLMSetWindowGeometry(gCommWindow,
                              gCommWinPosX,
                              gCommWinPosY,
                              gCommWinPosX+WINDOW_WIDTH,
                              gCommWinPosY-WINDOW_HEIGHT);
        gLastMouseX = x;
        gLastMouseY = y;
        break;
    case xplm_MouseUp:
        if (MouseDownX == x || MouseDownY == y) {
            int com1 = XPLMGetDatai(audio_selection_com1_dataref);
            int com2 = XPLMGetDatai(audio_selection_com2_dataref);

            if (com1 && com2 && com_changed) {
                switch (com_changed) {
                case COM1_CHANGED:
                    XPLMSetDatai(audio_selection_com1_dataref, COMM_UNSELECTED);
                    break;
                case COM2_CHANGED:
                    XPLMSetDatai(audio_selection_com2_dataref, COMM_UNSELECTED);
                    break;
                default:
                    break;
                }
                com_changed = COMMS_UNCHANGED;
            } else if (!com1 && com2) {
                com_changed = COM1_CHANGED;
                XPLMSetDatai(audio_selection_com1_dataref, COMM_SELECTED);
            }  else if (com1 && !com2) {
                com_changed = COM2_CHANGED;
                XPLMSetDatai(audio_selection_com2_dataref, COMM_SELECTED);
            }
        }
        break;
    } // switch (inMouse)

    return PROCESSED_EVENT;
}
