#pragma once

#include <SDL.h>

#include "../core/Delegate.h"
#include "../core/LinkedList.h"
#include "../core/Vector.h"

#include "DebugMenu.h"
#include "Singleton.h"

//\brief Input manager defines its own constants for input events in case there are input
//		 requirements outside of SDL
enum class InputType : int
{
	None = -1,
	KeyDown,			// A keyboard key was depressed
	KeyUp,				// A keyboard key was released
	MouseDown,			// Mouse button depressed
	MouseUp,			// Mouse button released
	MouseMotion,		// Mouse moved to some coord
	Count,
};

//\brief Easy to use mouse button constants, independant from SDLs numbered buttons
enum class MouseButton : int
{
	None = -1,
	Left,
	Right,
	Middle,
	Count,
};

class InputManager : public Singleton<InputManager>
{
public:
	InputManager()
		: m_focus(true)
		, m_fullScreen(false)
		, m_mousePos(0.0f)
		, m_mouseDir(0.0f)
		, m_lastKeyPress(SDLK_CLEAR)
		, m_lastKeyRelease(SDLK_CLEAR)
		, m_numGamepads(0)
		, m_mouseEnabled(true)
		, m_gamePadCheckTimer(0.0f)
		, m_quit(false)
	{
		// Init the list of depressed keys
		memset(&m_depressedKeys[0], SDLK_UNKNOWN, sizeof(SDL_Keycode) * s_maxDepressedKeys);
		memset(&m_depressedMouseButtons[0], 0, sizeof(bool) * static_cast<int>(MouseButton::Count));
		
		// Init gamepad pointers
		for (int i = 0; i < s_maxGamepads; ++i)
		{
			m_gamepads[i] = nullptr;
			memset(&m_depressedGamepadButtons[i], 0, sizeof(bool) * s_maxGamepadButtons);
		}
	}

	~InputManager() { Shutdown(); }

	//\brief Startup will set the input manager with what state the app window is in
	//\param a_fullScreen bool if the app window is being shown fullscreen
    bool Startup(bool a_fullScreen = false);

	//\brief Shutdown will clean up memory for input event list
	bool Shutdown();
	bool Update(float a_dt);
	bool EventPump(const SDL_Event & a_event);

	//\brief Enable or disable the app for OS focus. This will toggle rendering and input grab
	//\param The new app focus setting
	void SetFocus(bool a_focus);

	//\brief Access for the mouse coords for any part in the engine
	inline Vector2 GetMousePosAbsolute() const { return m_mousePos; }
	inline void SetMousePosAbsolute(const Vector2 & a_newPos) { m_mousePos = a_newPos; }
	Vector2 GetMousePosRelative() const;
	inline Vector2 GetMouseDirection() const { return m_mouseDir; }
	void SetMousePosRelative(const Vector2 & a_newPos);
	inline void DisableMouseInput() { m_mouseEnabled = false; }
	inline void EnableMouseInput() { m_mouseEnabled = true; }
	inline bool IsMouseButtonDepressed(MouseButton a_button) { return m_depressedMouseButtons[static_cast<int>(a_button)]; }

	//\brief Utility function to get the last key pressed or released
	//\param a_keyPress if the last key to be pressed or released is required, optional
	inline SDL_Keycode GetLastKey(bool a_keyPress = true) { return a_keyPress ? m_lastKeyPress : m_lastKeyRelease; }

	//\brief Utility function to find if an arbitrary key is pressed or released
	//\param a_keyVal the SDL key value of the key to check
	//\return bool true if the button is currently down
	bool IsKeyDepressed(SDL_Keycode a_key);

	inline void Quit() { m_quit = true; }

	//\ingroup Gamepad functions
	//\brief Get the number of connected gamepads
	inline int GetNumGamePads() { return m_numGamepads; }
	inline bool IsGamePadConnected(int a_gamepadId) { return m_gamepads[a_gamepadId] != nullptr; }
	inline bool IsGamePadButtonDepressed(int a_gamepadId, int a_buttonId)
	{
		return a_gamepadId < s_maxGamepads && a_buttonId < s_maxGamepadButtons ? m_depressedGamepadButtons[a_gamepadId][a_buttonId] : false;
	}
	inline float GetGamePadAxis(int a_gamepadId, int a_axisId)
	{
		float axisVal = 0.0f;
		if (m_gamepads[a_gamepadId] != nullptr)
		{
			axisVal = (float)(SDL_JoystickGetAxis(m_gamepads[a_gamepadId], a_axisId)) / 32768.0f;
		}
		return axisVal;
	}

	//\brief Register a function pointer to be called when the app receives a mouse event
	//\param a_callback is the object that contains the member function to call back
	//\param a_button is the button that is being depressed or released
	//\param a_type defaults to mouse up but can be changed to to down or motion
	//\param a_oneShot bool defines if the event should be deleted after the callback function is called
	template <typename TObj, typename TMethod>
	void RegisterMouseCallback(TObj * a_callerObject, TMethod a_callback, MouseButton a_button, InputType a_type = InputType::MouseUp, bool a_oneShot = false)
	{
		// Add an event to the list of items to be processed
		// TODO memory management! Kill std new with a rusty fork
		InputEventNode * newInputNode = new InputEventNode();
		newInputNode->SetData(new InputEvent());
	
		// Set data for the new event
		InputEvent * newInput = newInputNode->GetData();
		newInput->m_src.m_mouseButton = a_button;
		newInput->m_type = a_type;
		newInput->m_oneShot = a_oneShot;
		newInput->m_delegate.SetCallback(a_callerObject, a_callback);
		m_events.Insert(newInputNode);
	}

	//\brief Register a function pointer to be called when the app receives a keyboard event
	//\param a_callback is the object that contains the member function to call back
	//\param a_key is the keyboard key that is being depressed or released
	//\param a_type defaults to mouse up but can be changed to to down or motion
	//\param a_oneShot bool defines if the event should be deleted after the callback function is called
	template <typename TObj, typename TMethod>
	void RegisterKeyCallback(TObj * a_callerObject, TMethod a_callback, SDL_Keycode a_key, InputType a_type = InputType::KeyDown, bool a_oneShot = false)
	{
		// Add an event to the list of items to be processed
		InputEventNode * newInputNode = new InputEventNode();
		newInputNode->SetData(new InputEvent());
	
		// Set data for the new event
		InputEvent * newInput = newInputNode->GetData();
		newInput->m_src.m_key = a_key;
		newInput->m_type = a_type;
		newInput->m_oneShot = a_oneShot;
		newInput->m_delegate.SetCallback(a_callerObject, a_callback);
		m_events.Insert(newInputNode);
	}

	//\brief Register a function pointer to be called when the app receives a keyboard for any alphanumeric key
	//\param a_callback is the object that contains the member function to call back
	//\param a_oneShot bool defines if the event should be deleted after the callback function is called
	template <typename TObj, typename TMethod>
	void RegisterAlphaKeyCallback(TObj * a_callerObject, TMethod a_callback, InputType a_type = InputType::KeyDown, bool a_oneShot = false)
	{
		// Set data for the new global event
		if (a_type == InputType::KeyDown)
		{
			m_alphaKeysDown.m_src.m_key = SDLK_CLEAR;
			m_alphaKeysDown.m_type = a_type;
			m_alphaKeysDown.m_oneShot = a_oneShot;
			m_alphaKeysDown.m_delegate.SetCallback(a_callerObject, a_callback);
		}
		else if (a_type == InputType::KeyUp)
		{
			m_alphaKeysUp.m_src.m_key = SDLK_CLEAR;
			m_alphaKeysUp.m_type = a_type;
			m_alphaKeysUp.m_oneShot = a_oneShot;
			m_alphaKeysUp.m_delegate.SetCallback(a_callerObject, a_callback);
		}
	}

private:

	//\brief An input event can come from a number of sources but only one at once hence the union
	union InputSource
	{
		SDL_Keycode m_key;						///< A keyboard button
		MouseButton m_mouseButton;	///< A mouse click
	};

	//\brief Storage for an input event and it's callback
	struct InputEvent
	{
		InputEvent() 
			: m_src(InputSource())
			, m_delegate()
			, m_type(InputType::None)
			, m_oneShot(false) {}

		InputSource m_src;					///< What event happened
		Delegate<bool, bool> m_delegate;	///< Pointer to object to call when it happens
		InputType	m_type;				///< What type of event to respond to
		bool		m_oneShot;				///< If the event should only be responded to once
	};

	//\brief Input handling helper functions are split up so there is no one huge
	//		 switch statement going on. All these just process SDL input
	bool ProcessKeyDown(SDL_Keycode a_key);
	bool ProcessKeyUp(SDL_Keycode a_key);
	bool ProcessMouseDown(MouseButton a_button);
	bool ProcessMouseUp(MouseButton a_button);
    bool ProcessMouseMove();
	bool ProcessGamepadButtonDown(int a_gamepadId, int a_buttonId);
	bool ProcessGamepadButtonUp(int a_gamepadId, int a_buttonId);

	//\brief Alias to store a list of registered input events
	typedef LinkedListNode<InputEvent> InputEventNode;
	typedef LinkedList<InputEvent> InputEventList;

	//\brief Helper function to iterate list of events and find first matching event
	//\param a_type the type of event that is being searched for
	//\param a_src the matching input key or button to search for
	//\return a pointer to an input event or nullptr if no event found
	InputEvent * GetFirstEvent(InputType a_type, InputSource a_src);

	//\brief Helper function to iterate list of events and find all matching events
	//\param a_type the type of event that is being searched for
	//\param a_src the matching input key or button to search for
	//\param a_events_OUT is a ref to an event list to be populated by this function
	//\return true if at least one event was found
	bool GetEvents(InputType a_type, InputSource a_src, InputEventList & a_events_OUT);

	static const int s_maxDepressedKeys = 8;								///< How many keys can be held on the keyboard at once
	static const int s_maxGamepads = 8;										///< How many gamepads can be connected and playing a game
	static const int s_maxGamepadButtons = 16;								///< How many buttons are supported on each gamepad
	static const float s_gamePadCheckTime;									///< Interval for waiting to check for new gamepads
	static constexpr int s_maxMouseButtons = static_cast<int>(MouseButton::Count);

	InputEvent m_alphaKeysDown;												///< Special input event to catch all keys being pressed
	InputEvent m_alphaKeysUp;												///< Special input event to catch all keys being released
	InputEventList m_events;												///< List of events to match up to actions
	bool m_quit;															///< Set if we want to shutdown
	bool m_focus;															///< If the app currently has OS focus
	bool m_fullScreen;														///< If the app is fullscreen, input manager needs to handle focus
	bool m_mouseEnabled;													///< If the mouse is enabled for input processing
	float m_gamePadCheckTimer;												///< Timer to check for new plugged in gamepads
	Vector2 m_mousePos;														///< Cache of mouse coords for convenience
	Vector2 m_mouseDir;														///< Cache of mouse motion relative input
	SDL_Keycode m_lastKeyPress;												///< Cache off last key for convenience
	SDL_Keycode m_lastKeyRelease;											///< Cache off last key for convenience
	SDL_Keycode m_depressedKeys[s_maxDepressedKeys];						///< List of all the keys that are depressed 
	SDL_Joystick * m_gamepads[s_maxGamepads];								///< Pointers to all open gamepads
	int m_numGamepads;														///< How many gamepads are connected
	bool m_depressedGamepadButtons[s_maxGamepads][s_maxGamepadButtons];		///< If each of the gamepad buttons are pressed
	bool m_depressedMouseButtons[s_maxMouseButtons];						///< If each of the mouse buttons are pressed
};
