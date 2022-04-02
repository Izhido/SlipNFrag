#pragma once

#define JOY_ABSOLUTE_AXIS    0x00000000        // control like a joystick
#define JOY_RELATIVE_AXIS    0x00000010        // control like a mouse, spinner, trackball
#define    JOY_MAX_AXES        6                // X, Y, Z, R, U, V
#define JOY_AXIS_X            0
#define JOY_AXIS_Y            1
#define JOY_AXIS_Z            2
#define JOY_AXIS_R            3
#define JOY_AXIS_U            4
#define JOY_AXIS_V            5

enum _ControlList
{
    AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn
};

extern int dwAxisMap[JOY_MAX_AXES];
extern int dwControlMap[JOY_MAX_AXES];
extern float pdwRawValue[JOY_MAX_AXES];

extern float mx, my;
extern qboolean mouseinitialized;
extern qboolean joy_initialized, joy_avail;

extern float   in_forwardmove;
extern float   in_sidestepmove;

extern float   in_pitchangle;
extern float   in_rollangle;

void Joy_AdvancedUpdate_f(void);
