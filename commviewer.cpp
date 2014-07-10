// Copyright (c) 2014 Joseph D Poirier
// Distributable under the terms of The New BSD License
// that can be found in the LICENSE file.

#ifdef _WIN32 /* this is set for 64 bit as well */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#ifdef _APPLE_
//#include <OpenGl/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdio.h>
#include <math.h>

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

static void HotKeyCallback(void* inRefcon);

static XPLMWindowID gCommWindow = NULL;
static bool gPluginEnabled = false;
static int gPlaneLoaded = 0;
static const float FL_CB_INTERVAL = -1.0;
static bool gPTT_On = false;
static XPLMHotKeyID gHotKey = NULL;

#define WINDOW_WIDTH (200)
#define WINDOW_HEIGHT (40)
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
#define sCONTACT_ATC   "sim/operation/contact_atc"

XPLMDataRef avionics_power_on_dataref;
XPLMDataRef audio_selection_com1_dataref;
XPLMDataRef audio_selection_com2_dataref;

XPLMDataRef pilotedge_rx_status_dataref;
XPLMDataRef pilotedge_tx_status_dataref;
XPLMDataRef pilotedge_connected_dataref;

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
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc) {

    LPRINTF("CommView Plugin: XPluginStart\n");
    strcpy(outName, "CommViewer");
    strcpy(outSig , "jdp.comm.viewer");
    strcpy(outDesc, "CommViewer Plugin.");

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

    pilotedge_rx_status_dataref = XPLMFindDataRef("pilotedge/radio/rx_status");
    pilotedge_tx_status_dataref = XPLMFindDataRef("pilotedge/radio/tx_status");
    pilotedge_connected_dataref = XPLMFindDataRef("pilotedge/status/connected");

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

#ifdef TOGGLE_TEST_FEATURE
    //gHotKey = XPLMRegisterHotKey(XPLM_VK_F3,
    //                             xplm_DownFlag,
    //                             "Toggle Window Ice",
    //                             HotKeyCallback,
    //                             NULL);
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

    switch (reinterpret_cast<int>(inRefcon)) {
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
            if (inparam != PLUGIN_PLANE_ID) { break; }
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_CRASHED\n");
            break;
        case XPLM_MSG_PLANE_UNLOADED:
            if (inparam != PLUGIN_PLANE_ID) { break; }
            LPRINTF("CommView Plugin: XPluginReceiveMessage XPLM_MSG_PLANE_UNLOADED\n");
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
void DrawWindowCallback(XPLMWindowID inWindowID, void* inRefcon) {

    int left;
    int top;
    int right;
    int bottom;
    static char str[100];
    static float commviewer_color[] = {1.0, 1.0, 1.0};  // RGB White


    // XXX: are inWindowIDs our XPLMCreateWindow return pointers
    XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);
    XPLMDrawTranslucentDarkBox(left, top, right, bottom);

    switch (reinterpret_cast<int>(inRefcon)) {
    case COMMVIEWER_WINDOW:
        char rx_status = (pilotedge_rx_status_dataref ?
                          XPLMGetDatab(pilotedge_rx_status_dataref) : false) ?
                          (char)\x254 : " ");

        char tx_status = (pilotedge_tx_status_dataref ?
                          XPLMGetDatab(pilotedge_tx_status_dataref) : false) ?
                          (char)\x254 : " ");

        char connected = (pilotedge_tx_status_dataref ?
                          XPLMGetDatab(pilotedge_connected_dataref) : false) ?
                          "YES" : "NO";

        // text to window, NULL indicates no word wrap
#if false
        sprintf(str,"%s\t\t\tCOM1: %d\t\t\tCOM2: %d",
                (char*)(gPTT_On ? "PTT: ON" : "PTT: OFF"),
                XPLMGetDatai(audio_selection_com1_dataref),
                XPLMGetDatai(audio_selection_com2_dataref));
#else
        sprintf(str,"[PilotEdge] Connected: %s,\t\t\tTX: %c\t\t\tRX: %c\n%s\t\t\tCOM1: %d\t\t\tCOM2: %d",
                connected,
                tx_status,
                rx_status,
                (char*)(gPTT_On ? "PTT: ON" : "PTT: OFF"),
                XPLMGetDatai(audio_selection_com1_dataref),
                XPLMGetDatai(audio_selection_com2_dataref));
#endif
        XPLMDrawString(commviewer_color,
                       left+5,
                       top-20,
                       str,
                       NULL,
                       xplmFont_Basic);
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
 #define COMMS_UNCHANGED    (0)
 #define COM1_CHANGED       (1)
 #define COM2_CHANGED       (2)
 #define COMM_UNSELECTED    (0)
 #define COMM_SELECTED      (1)
int HandleMouseCallback(XPLMWindowID inWindowID,
                        int x,
                        int y,
                        XPLMMouseStatus inMouse,
                        void* inRefcon) {

    static int com_changed = COMMS_UNCHANGED;
    static int MouseDownX;
    static int MouseDownY;

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
