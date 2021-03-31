#include "GpInputDriver_SDL_Gamepad.h"
#include "GpVOSEvent.h"
#include "IGpVOSEventQueue.h"

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"

#include <stdlib.h>
#include <algorithm>
#include <new>
#include <vector>

class GpInputDriverSDLGamepad final : public IGpInputDriverSDLGamepad
{
public:
	void ProcessInput() override;
	void Shutdown() override;

	IGpPrefsHandler *GetPrefsHandler() const override;

	static GpInputDriverSDLGamepad *Create(const GpInputDriverProperties &props);

	void ProcessSDLEvent(const SDL_Event &evt) override;

private:
	static const int kMaxPlayers = 2;

	GpInputDriverSDLGamepad(const GpInputDriverProperties &props);
	~GpInputDriverSDLGamepad();

	bool FindJoystickPlayer(uint8_t &playerNum, SDL_JoystickID joystickID);

	void HandleDeviceAdded(SDL_JoystickID joystickID);
	void HandleDeviceRemoved(SDL_JoystickID joystickID);

	std::vector<GpVOSEvent> m_pendingEvents;

	GpInputDriverProperties m_properties;

	SDL_JoystickID m_playerJoystickIDs[kMaxPlayers];
	SDL_GameController *m_playerControllers[kMaxPlayers];
	bool m_playerButtonDown[kMaxPlayers][GpGamepadButtons::kCount];
};

static GpInputDriverSDLGamepad *gs_instance = nullptr;

IGpInputDriverSDLGamepad *IGpInputDriverSDLGamepad::GetInstance()
{
	return gs_instance;
}

void GpInputDriverSDLGamepad::ProcessInput()
{
	for (size_t i = 0; i < m_pendingEvents.size(); i++)
	{
		if (GpVOSEvent *evt = m_properties.m_eventQueue->QueueEvent())
			*evt = m_pendingEvents[i];
	}

	m_pendingEvents.clear();
}

void GpInputDriverSDLGamepad::Shutdown()
{
	this->~GpInputDriverSDLGamepad();
	free(this);

	gs_instance = nullptr;
}

IGpPrefsHandler *GpInputDriverSDLGamepad::GetPrefsHandler() const
{
	return nullptr;
}

GpInputDriverSDLGamepad *GpInputDriverSDLGamepad::Create(const GpInputDriverProperties &props)
{
	void *storage = malloc(sizeof(GpInputDriverSDLGamepad));
	if (!storage)
		return nullptr;

	GpInputDriverSDLGamepad *driver = new (storage) GpInputDriverSDLGamepad(props);
	gs_instance = driver;
	return driver;
}

GpInputDriverSDLGamepad::GpInputDriverSDLGamepad(const GpInputDriverProperties &props)
	: m_properties(props)
{
	for (int i = 0; i < kMaxPlayers; i++)
	{
		m_playerJoystickIDs[i] = -1;
		m_playerControllers[i] = nullptr;

		for (int j = 0; j < GpGamepadButtons::kCount; j++)
			m_playerButtonDown[i][j] = false;
	}
}

GpInputDriverSDLGamepad::~GpInputDriverSDLGamepad()
{
	for (int i = 0; i < kMaxPlayers; i++)
	{
		if (m_playerControllers[i])
			SDL_GameControllerClose(m_playerControllers[i]);
	}
}

bool GpInputDriverSDLGamepad::FindJoystickPlayer(uint8_t &playerNum, SDL_JoystickID joystickID)
{
	for (int i = 0; i < kMaxPlayers; i++)
	{
		if (m_playerJoystickIDs[i] == joystickID)
		{
			playerNum = static_cast<uint8_t>(i);
			return true;
		}
	}

	return false;
}

void GpInputDriverSDLGamepad::HandleDeviceAdded(SDL_JoystickID joystickID)
{
	for (int i = 0; i < kMaxPlayers; i++)
	{
		if (m_playerJoystickIDs[i] == -1)
		{
			SDL_GameController *controller = SDL_GameControllerOpen(joystickID);
			if (controller)
			{
				m_playerJoystickIDs[i] = joystickID;
				m_playerControllers[i] = controller;
			}
			return;
		}
	}
}

void GpInputDriverSDLGamepad::HandleDeviceRemoved(SDL_JoystickID joystickID)
{
	int playerNum = 0;
	bool foundPlayer = false;
	for (int i = 0; i < kMaxPlayers; i++)
	{
		if (m_playerJoystickIDs[i] == joystickID)
		{
			SDL_GameControllerClose(m_playerControllers[i]);

			m_playerJoystickIDs[i] = -1;
			m_playerControllers[i] = nullptr;

			playerNum = i;
			foundPlayer = true;
		}
	}

	if (!foundPlayer)
		return;

	for (int axis = 0; axis < GpGamepadAxes::kCount; axis++)
	{
		GpVOSEvent evt;
		evt.m_eventType = GpVOSEventTypes::kGamepadInput;
		evt.m_event.m_gamepadInputEvent.m_eventType = GpGamepadInputEventTypes::kAnalogAxisChanged;
		evt.m_event.m_gamepadInputEvent.m_event.m_analogAxisEvent.m_axis = static_cast<GpGamepadAxis_t>(axis);
		evt.m_event.m_gamepadInputEvent.m_event.m_analogAxisEvent.m_player = playerNum;
		evt.m_event.m_gamepadInputEvent.m_event.m_analogAxisEvent.m_state = 0;

		m_pendingEvents.push_back(evt);
	}

	for (int button = 0; button < GpGamepadButtons::kCount; button++)
	{
		if (m_playerButtonDown[playerNum][button])
		{
			m_playerButtonDown[playerNum][button] = false;



			GpVOSEvent evt;
			evt.m_eventType = GpVOSEventTypes::kKeyboardInput;
			evt.m_event.m_keyboardInputEvent.m_eventType = GpKeyboardInputEventTypes::kUp;
			evt.m_event.m_keyboardInputEvent.m_keyIDSubset = GpKeyIDSubsets::kGamepadButton;
			evt.m_event.m_keyboardInputEvent.m_key.m_gamepadKey.m_player = playerNum;
			evt.m_event.m_keyboardInputEvent.m_key.m_gamepadKey.m_button = static_cast<GpGamepadButton_t>(button);
			evt.m_event.m_keyboardInputEvent.m_repeatCount = 0;

			m_pendingEvents.push_back(evt);
		}
	}
}

void GpInputDriverSDLGamepad::ProcessSDLEvent(const SDL_Event &msg)
{
	IGpVOSEventQueue *evtQueue = m_properties.m_eventQueue;

	switch (msg.type)
	{
	case SDL_CONTROLLERAXISMOTION:
		{
			const SDL_ControllerAxisEvent &axisMsg = msg.caxis;
			GpGamepadAxis_t axis = GpGamepadAxes::kCount;

			uint8_t playerNumber = 0;
			if (!FindJoystickPlayer(playerNumber, axisMsg.which))
				break;

			if (axisMsg.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
				axis = GpGamepadAxes::kLeftTrigger;
			else if (axisMsg.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
				axis = GpGamepadAxes::kRightTrigger;
			else if (axisMsg.axis == SDL_CONTROLLER_AXIS_LEFTX)
				axis = GpGamepadAxes::kLeftStickX;
			else if (axisMsg.axis == SDL_CONTROLLER_AXIS_LEFTY)
				axis = GpGamepadAxes::kLeftStickY;
			else if (axisMsg.axis == SDL_CONTROLLER_AXIS_RIGHTX)
				axis = GpGamepadAxes::kRightStickX;
			else if (axisMsg.axis == SDL_CONTROLLER_AXIS_RIGHTY)
				axis = GpGamepadAxes::kRightStickY;
			else
				break;

			GpVOSEvent evt;
			evt.m_eventType = GpVOSEventTypes::kGamepadInput;
			evt.m_event.m_gamepadInputEvent.m_eventType = GpGamepadInputEventTypes::kAnalogAxisChanged;
			evt.m_event.m_gamepadInputEvent.m_event.m_analogAxisEvent.m_axis = axis;
			evt.m_event.m_gamepadInputEvent.m_event.m_analogAxisEvent.m_player = playerNumber;
			evt.m_event.m_gamepadInputEvent.m_event.m_analogAxisEvent.m_state = std::max<int16_t>(-32767, axisMsg.value);

			m_pendingEvents.push_back(evt);
		}
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		{
			const bool isDown = (msg.type == SDL_CONTROLLERBUTTONDOWN);
			const SDL_ControllerButtonEvent &buttonMsg = msg.cbutton;

			GpGamepadButton_t gpButton = GpGamepadButtons::kCount;

			uint8_t playerNumber = 0;
			if (!FindJoystickPlayer(playerNumber, buttonMsg.which))
				break;

			if (buttonMsg.button == SDL_CONTROLLER_BUTTON_A)
				gpButton = GpGamepadButtons::kFaceRight;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_B)
				gpButton = GpGamepadButtons::kFaceDown;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_X)
				gpButton = GpGamepadButtons::kFaceUp;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_Y)
				gpButton = GpGamepadButtons::kFaceLeft;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_START)
				gpButton = GpGamepadButtons::kMisc1;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_BACK)
				gpButton = GpGamepadButtons::kMisc2;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_LEFTSTICK)
				gpButton = GpGamepadButtons::kLeftStick;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK)
				gpButton = GpGamepadButtons::kRightStick;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
				gpButton = GpGamepadButtons::kLeftBumper;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
				gpButton = GpGamepadButtons::kRightBumper;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
				gpButton = GpGamepadButtons::kDPadUp;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
				gpButton = GpGamepadButtons::kDPadDown;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
				gpButton = GpGamepadButtons::kDPadLeft;
			else if (buttonMsg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
				gpButton = GpGamepadButtons::kDPadRight;
			else
				break;

			m_playerButtonDown[playerNumber][gpButton] = isDown;

			GpVOSEvent evt;
			evt.m_eventType = GpVOSEventTypes::kKeyboardInput;
			evt.m_event.m_keyboardInputEvent.m_eventType = (isDown ? GpKeyboardInputEventTypes::kDown : GpKeyboardInputEventTypes::kUp);
			evt.m_event.m_keyboardInputEvent.m_keyIDSubset = GpKeyIDSubsets::kGamepadButton;
			evt.m_event.m_keyboardInputEvent.m_key.m_gamepadKey.m_player = playerNumber;
			evt.m_event.m_keyboardInputEvent.m_key.m_gamepadKey.m_button = gpButton;
			evt.m_event.m_keyboardInputEvent.m_repeatCount = 0;

			m_pendingEvents.push_back(evt);
		}
		break;
	case SDL_CONTROLLERDEVICEADDED:
		HandleDeviceAdded(msg.cdevice.which);
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		HandleDeviceRemoved(msg.cdevice.which);
		break;
	case SDL_CONTROLLERDEVICEREMAPPED:
		// Not really sure what to do here, just re-add it
		HandleDeviceRemoved(msg.cdevice.which);
		HandleDeviceAdded(msg.cdevice.which);
		break;
	}
}

IGpInputDriver *GpDriver_CreateInputDriver_SDL2_Gamepad(const GpInputDriverProperties &properties)
{
	return GpInputDriverSDLGamepad::Create(properties);
}
