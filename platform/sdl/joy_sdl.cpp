#include <stdio.h>
#include <algorithm>
#include "platform/joy.h"
#include "platform/timer.h"
#include "misc/error.h"

#ifdef USE_SDL

#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"
#include "SDL_events.h"

bool usingGamepad = false; //Gamepads need special handling to contort into the Descent Joystick API

int numJoysticks;
SDL_Joystick *joysticks[2];
SDL_GameController *controller;

void I_InitSDLJoysticks()
{
	int i;
	numJoysticks = SDL_NumJoysticks();
	if (numJoysticks == 0) return; 
	if (numJoysticks > 2) numJoysticks = 2;

	//Special gamepad handling
	if (SDL_IsGameController(0))
	{
		usingGamepad = true;
		controller = SDL_GameControllerOpen(0);
		if (!controller)
		{
			Warning("I_InitSDLJoysticks: Failed to open game controller: %s\n", SDL_GetError());
		}
	}
	else
	{
		for (i = 0; i < numJoysticks; i++)
		{
			joysticks[i] = SDL_JoystickOpen(i);
			if (!joysticks[i])
			{
				Warning("I_InitSDLJoysticks: Failed to open joystick %d: %s\n", i, SDL_GetError());
			}
		}
	}
}

#define joybtn(x) (1 << x)

void I_JoystickHandler()
{
	if (usingGamepad) return;
	int axisState[4];
	int buttons = 0;
	//Simulate dpJudas's device simulation from the Windows code
	if (numJoysticks >= 1 && SDL_JoystickNumHats(joysticks[0]) > 0) //Simulate ThrustMaster FCS
	{
		axisState[0] = SDL_JoystickGetAxis(joysticks[0], 0) * 127 / 32767;
		axisState[1] = SDL_JoystickGetAxis(joysticks[0], 1) * 127 / 32767;

		int hatdir = SDL_JoystickGetHat(joysticks[0], 0);
		//this is dumb
		if (hatdir & SDL_HAT_DOWN)
			axisState[2] = 1;
		else if (hatdir & SDL_HAT_UP)
			axisState[2] = 20;
		else if (hatdir & SDL_HAT_RIGHT)
			axisState[2] = 50;
		else if (hatdir & SDL_HAT_LEFT)
			axisState[2] = 81;
		else
			axisState[2] = 127;

		int numButtons = std::min(20, SDL_JoystickNumButtons(joysticks[0]));
		for (int i = 0; i < numButtons; i++)
		{
			if (SDL_JoystickGetButton(joysticks[0], i))
				buttons |= joybtn(i);
		}
	}
	else if (numJoysticks > 1) //2 joysticks
	{
		axisState[0] = SDL_JoystickGetAxis(joysticks[0], 0) * 127 / 32767;
		axisState[1] = SDL_JoystickGetAxis(joysticks[0], 1) * 127 / 32767;
		axisState[2] = SDL_JoystickGetAxis(joysticks[1], 0) * 127 / 32767;
		axisState[3] = SDL_JoystickGetAxis(joysticks[1], 1) * 127 / 32767;

		if (SDL_JoystickGetButton(joysticks[0], 0))
			buttons |= joybtn(0);
		if (SDL_JoystickGetButton(joysticks[0], 1))
			buttons |= joybtn(1);
		if (SDL_JoystickGetButton(joysticks[1], 0))
			buttons |= joybtn(2);
		if (SDL_JoystickGetButton(joysticks[1], 1))
			buttons |= joybtn(3);
	}
	else if (numJoysticks == 1) //just one
	{
		axisState[0] = SDL_JoystickGetAxis(joysticks[0], 0) * 127 / 32767;
		axisState[1] = SDL_JoystickGetAxis(joysticks[0], 1) * 127 / 32767;
		axisState[2] = SDL_JoystickGetAxis(joysticks[1], 0) * 127 / 32767;
		axisState[3] = SDL_JoystickGetAxis(joysticks[1], 1) * 127 / 32767;

		if (SDL_JoystickGetButton(joysticks[0], 0))
			buttons |= joybtn(0);
		if (SDL_JoystickGetButton(joysticks[0], 1))
			buttons |= joybtn(1);
		if (SDL_JoystickGetButton(joysticks[0], 2))
			buttons |= joybtn(2);
		if (SDL_JoystickGetButton(joysticks[0], 3))
			buttons |= joybtn(3);
	}
	JoystickInput(buttons, axisState, JOY_ALL_AXIS);
}

//[ISB] This should probably handle events, but at the same time I could just read everything and do it in one go...
//Probably slower and shittier but...
void I_ControllerHandler()
{
	if (!controller || !usingGamepad) return;
	int axisState[4];
	int buttons = 0;
	axisState[0] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) * 127 / 32767;
	axisState[1] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) * 127 / 32767;
	axisState[2] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) * 127 / 32767;
	axisState[3] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) * 127 / 32767;

	//These numbers start at 4. Gamepads are currently treated as a Flightstick Pro, which doesn't support buttons 1-4
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))
		buttons |= joybtn(4);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))
		buttons |= joybtn(5);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X))
		buttons |= joybtn(6);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y))
		buttons |= joybtn(7);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
		buttons |= joybtn(8);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
		buttons |= joybtn(9);
	//XInput trigger hack
	if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > (32767 / 3))
		buttons |= joybtn(10);
	if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > (32767 / 3))
		buttons |= joybtn(11);

	//printf("btns: %d axises: %d %d %d %d\n", buttons, axisState[0], axisState[1], axisState[2], axisState[3]);
	JoystickInput(buttons, axisState, JOY_ALL_AXIS);
}

#endif