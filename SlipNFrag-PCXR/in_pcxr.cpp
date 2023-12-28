#include "quakedef.h"
#include "in_pcxr.h"

float   mouse_x, mouse_y;
float   old_mouse_x, old_mouse_y;
float   mx, my;

float   in_forwardmove;
float   in_sidestepmove;

float   in_pitchangle;
float   in_rollangle;

cvar_t    m_filter = {"m_filter","1"};
cvar_t    in_anglescaling = {"in_anglescaling","0.1"};

qboolean        mouseinitialized;

int dwAxisMap[JOY_MAX_AXES];
int dwControlMap[JOY_MAX_AXES];
float pdwRawValue[JOY_MAX_AXES];

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
cvar_t    in_joystick = { "joystick","0", true };
cvar_t    joy_name = { "joyname", "joystick" };
cvar_t    joy_advanced = { "joyadvanced", "0" };
cvar_t    joy_advaxisx = { "joyadvaxisx", "0" };
cvar_t    joy_advaxisy = { "joyadvaxisy", "0" };
cvar_t    joy_advaxisz = { "joyadvaxisz", "0" };
cvar_t    joy_advaxisr = { "joyadvaxisr", "0" };
cvar_t    joy_advaxisu = { "joyadvaxisu", "0" };
cvar_t    joy_advaxisv = { "joyadvaxisv", "0" };
cvar_t    joy_forwardthreshold = { "joyforwardthreshold", "0.15" };
cvar_t    joy_sidethreshold = { "joysidethreshold", "0.15" };
cvar_t    joy_pitchthreshold = { "joypitchthreshold", "0.15" };
cvar_t    joy_yawthreshold = { "joyyawthreshold", "0.15" };
cvar_t    joy_forwardsensitivity = { "joyforwardsensitivity", "-1.0" };
cvar_t    joy_sidesensitivity = { "joysidesensitivity", "-1.0" };
cvar_t    joy_pitchsensitivity = { "joypitchsensitivity", "1.0" };
cvar_t    joy_yawsensitivity = { "joyyawsensitivity", "-1.0" };

qboolean    joy_initialized, joy_avail;
qboolean    joy_advancedinit;

/*
 ===========
 Force_CenterView_f
 ===========
 */
void Force_CenterView_f(void)
{
    cl.viewangles[PITCH] = 0;
}

/*
 ===========
 Joy_AdvancedUpdate_f
 ===========
 */
void Joy_AdvancedUpdate_f(void)
{
    // called once by IN_ReadJoystick and by user whenever an update is needed
    // cvars are now available
    int    i;
    int dwTemp;
    
    // initialize all the maps
    for (i = 0; i < JOY_MAX_AXES; i++)
    {
        dwAxisMap[i] = AxisNada;
        dwControlMap[i] = JOY_ABSOLUTE_AXIS;
        pdwRawValue[i] = 0;
    }
    
    if (joy_advanced.value == 0.0)
    {
        // default joystick initialization
        // 2 axes only with joystick control
        dwAxisMap[JOY_AXIS_X] = AxisTurn;
        // dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
        dwAxisMap[JOY_AXIS_Y] = AxisForward;
        // dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
    }
    else
    {
        if (Q_strcmp(joy_name.string.c_str(), "joystick") != 0)
        {
            // notify user of advanced controller
            Con_Printf("\n%s configured\n\n", joy_name.string.c_str());
        }
        
        // advanced initialization here
        // data supplied by user via joy_axisn cvars
        dwTemp = (int)joy_advaxisx.value;
        dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisy.value;
        dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisz.value;
        dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisr.value;
        dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisu.value;
        dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisv.value;
        dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
    }
}

/*
 ===========
 IN_StartupMouse
 ===========
 */
void IN_StartupMouse()
{
    if (COM_CheckParm("-nomouse"))
        return;
    
    mouseinitialized = true;
}

/*
 ===============
 IN_StartupJoystick
 ===============
 */
void IN_StartupJoystick()
{
    // assume no joystick
    joy_avail = false;
    
    // abort startup if user requests no joystick
    if (COM_CheckParm("-nojoy"))
        return;
    
    joy_initialized = true;
    
    joy_advancedinit = false;
}

void IN_Init (void)
{
    // mouse variables
    Cvar_RegisterVariable(&m_filter);
    
    // joystick variables
    Cvar_RegisterVariable(&in_joystick);
    Cvar_RegisterVariable(&joy_name);
    Cvar_RegisterVariable(&joy_advanced);
    Cvar_RegisterVariable(&joy_advaxisx);
    Cvar_RegisterVariable(&joy_advaxisy);
    Cvar_RegisterVariable(&joy_advaxisz);
    Cvar_RegisterVariable(&joy_advaxisr);
    Cvar_RegisterVariable(&joy_advaxisu);
    Cvar_RegisterVariable(&joy_advaxisv);
    Cvar_RegisterVariable(&joy_forwardthreshold);
    Cvar_RegisterVariable(&joy_sidethreshold);
    Cvar_RegisterVariable(&joy_pitchthreshold);
    Cvar_RegisterVariable(&joy_yawthreshold);
    Cvar_RegisterVariable(&joy_forwardsensitivity);
    Cvar_RegisterVariable(&joy_sidesensitivity);
    Cvar_RegisterVariable(&joy_pitchsensitivity);
    Cvar_RegisterVariable(&joy_yawsensitivity);
    
    Cvar_RegisterVariable(&in_anglescaling);
    
    Cmd_AddCommand("force_centerview", Force_CenterView_f);
    Cmd_AddCommand("joyadvancedupdate", Joy_AdvancedUpdate_f);
    
    IN_StartupMouse();
    IN_StartupJoystick();
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_MouseMove (usercmd_t *cmd)
{
    if (m_filter.value)
    {
        mouse_x = (mx + old_mouse_x) * 0.5;
        mouse_y = (my + old_mouse_y) * 0.5;
    }
    else
    {
        mouse_x = mx;
        mouse_y = my;
    }
    old_mouse_x = mx;
    old_mouse_y = my;
    mx = my = 0; // clear for next update
    
    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;
    
    // add mouse X/Y movement to cmd
    if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;
    
    if (in_mlook.state & 1)
        V_StopPitchDrift ();
    
    if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
    {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    }
    else
    {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    
    if (in_rollangle != 0.0 || in_pitchangle != 0.0)
    {
        cl.viewangles[YAW] -= in_rollangle * in_anglescaling.value * 90;
        
        V_StopPitchDrift();
        
        cl.viewangles[PITCH] += in_pitchangle * in_anglescaling.value * 90;
        
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    }
    if (key_dest == key_game && (in_forwardmove != 0.0 || in_sidestepmove != 0.0))
    {
        float speed;
        
        if (in_speed.state & 1)
            speed = cl_movespeedkey.value;
        else
            speed = 1;
        
        cmd->sidemove += in_forwardmove * speed * cl_forwardspeed.value;
        
        cmd->forwardmove += in_sidestepmove * speed * cl_sidespeed.value;
    }
}

void IN_JoyMove(usercmd_t* cmd)
{
    if (!joy_initialized)
    {
        return;
    }
    
    float    speed, aspeed;
    float    fAxisValue;
    int        i;
    
    // complete initialization if first time in
    // this is needed as cvars are not available at initialization time
    if (joy_advancedinit != true)
    {
        Joy_AdvancedUpdate_f();
        joy_advancedinit = true;
    }
    
    // verify joystick is available and that the user wants to use it
    if (!joy_avail)
    {
        return;
    }
    
    if (in_speed.state & 1)
        speed = cl_movespeedkey.value;
    else
        speed = 1;
    aspeed = speed * host_frametime;
    
    // loop through the axes
    for (i = 0; i < JOY_MAX_AXES; i++)
    {
        // get the floating point zero-centered, potentially-inverted data for the current axis
        fAxisValue = pdwRawValue[i];
        
        switch (dwAxisMap[i])
        {
            case AxisForward:
                if ((joy_advanced.value == 0.0) && (in_mlook.state & 1))
                {
                    // user wants forward control to become look control
                    if (fabs(fAxisValue) > joy_pitchthreshold.value)
                    {
                        // if mouse invert is on, invert the joystick pitch value
                        // only absolute control support here (joy_advanced is false)
                        if (m_pitch.value < 0.0)
                        {
                            cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
                        }
                        else
                        {
                            cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
                        }
                        V_StopPitchDrift();
                    }
                    else
                    {
                        // no pitch movement
                        // disable pitch return-to-center unless requested by user
                        // *** this code can be removed when the lookspring bug is fixed
                        // *** the bug always has the lookspring feature on
                        if (lookspring.value == 0.0)
                            V_StopPitchDrift();
                    }
                }
                else
                {
                    // user wants forward control to be forward control
                    if (fabs(fAxisValue) > joy_forwardthreshold.value)
                    {
                        cmd->forwardmove += (fAxisValue * joy_forwardsensitivity.value) * speed * cl_forwardspeed.value;
                    }
                }
                break;
                
            case AxisSide:
                if (fabs(fAxisValue) > joy_sidethreshold.value)
                {
                    cmd->sidemove += (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
                }
                break;
                
            case AxisTurn:
                if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
                {
                    // user wants turn control to become side control
                    if (fabs(fAxisValue) > joy_sidethreshold.value)
                    {
                        cmd->sidemove -= (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
                    }
                }
                else
                {
                    // user wants turn control to be turn control
                    if (fabs(fAxisValue) > joy_yawthreshold.value)
                    {
                        if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
                        {
                            cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * aspeed * cl_yawspeed.value;
                        }
                        else
                        {
                            cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity.value) * speed * 180.0;
                        }
                        
                    }
                }
                break;
                
            case AxisLook:
                if (in_mlook.state & 1)
                {
                    if (fabs(fAxisValue) > joy_pitchthreshold.value)
                    {
                        // pitch movement detected and pitch movement desired by user
                        if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
                        {
                            cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
                        }
                        else
                        {
                            cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.value) * speed * 180.0;
                        }
                        V_StopPitchDrift();
                    }
                    else
                    {
                        // no pitch movement
                        // disable pitch return-to-center unless requested by user
                        // *** this code can be removed when the lookspring bug is fixed
                        // *** the bug always has the lookspring feature on
                        if (lookspring.value == 0.0)
                            V_StopPitchDrift();
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    // bounds check pitch
    if (cl.viewangles[PITCH] > 80.0)
        cl.viewangles[PITCH] = 80.0;
    if (cl.viewangles[PITCH] < -70.0)
        cl.viewangles[PITCH] = -70.0;
}

void IN_Move(usercmd_t* cmd)
{
    IN_MouseMove(cmd);
    IN_JoyMove(cmd);
}
