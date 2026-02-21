#include "Actions.h"
#include "AppState.h"
#include "Utils.h"
#include "Logger.h"

void Actions::Create(AppState& appState, XrInstance instance)
{
	XrActionSetCreateInfo actionSetInfo { XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy(actionSetInfo.actionSetName, "gameplay");
	strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
	CHECK_XRCMD(xrCreateActionSet(instance, &actionSetInfo, &ActionSet));

	XrActionCreateInfo actionInfo { XR_TYPE_ACTION_CREATE_INFO };

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy(actionInfo.actionName, "play_1");
	strcpy(actionInfo.localizedActionName, "Play 1");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Play1));

	strcpy(actionInfo.actionName, "play_2");
	strcpy(actionInfo.localizedActionName, "Play 2");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Play2));

	strcpy(actionInfo.actionName, "jump_left_handed");
	strcpy(actionInfo.localizedActionName, "Jump (left handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &JumpLeftHanded));

	strcpy(actionInfo.actionName, "jump_right_handed");
	strcpy(actionInfo.localizedActionName, "Jump (right handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &JumpRightHanded));

	strcpy(actionInfo.actionName, "swim_down_left_handed");
	strcpy(actionInfo.localizedActionName, "Swim down (left handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &SwimDownLeftHanded));

	strcpy(actionInfo.actionName, "swim_down_right_handed");
	strcpy(actionInfo.localizedActionName, "Swim down (right handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &SwimDownRightHanded));

	strcpy(actionInfo.actionName, "run");
	strcpy(actionInfo.localizedActionName, "Run");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Run));

	strcpy(actionInfo.actionName, "fire");
	strcpy(actionInfo.localizedActionName, "Fire");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Fire));

	actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	strcpy(actionInfo.actionName, "move_x");
	strcpy(actionInfo.localizedActionName, "Move X");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &MoveX));

	strcpy(actionInfo.actionName, "move_y");
	strcpy(actionInfo.localizedActionName, "Move Y");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &MoveY));

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy(actionInfo.actionName, "switch_weapon");
	strcpy(actionInfo.localizedActionName, "Switch weapon");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &SwitchWeapon));

	strcpy(actionInfo.actionName, "menu");
	strcpy(actionInfo.localizedActionName, "Menu");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Menu));

	strcpy(actionInfo.actionName, "menu_left_handed");
	strcpy(actionInfo.localizedActionName, "Menu (left handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &MenuLeftHanded));

	strcpy(actionInfo.actionName, "menu_right_handed");
	strcpy(actionInfo.localizedActionName, "Menu (right handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &MenuRightHanded));

	strcpy(actionInfo.actionName, "enter_trigger");
	strcpy(actionInfo.localizedActionName, "Enter (with triggers)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &EnterTrigger));

	strcpy(actionInfo.actionName, "enter_non_trigger_left_handed");
	strcpy(actionInfo.localizedActionName, "Enter (without triggers, left handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &EnterNonTriggerLeftHanded));

	strcpy(actionInfo.actionName, "enter_non_trigger_right_handed");
	strcpy(actionInfo.localizedActionName, "Enter (without triggers, right handed)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &EnterNonTriggerRightHanded));

	strcpy(actionInfo.actionName, "escape_y");
	strcpy(actionInfo.localizedActionName, "Escape (plus Y)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &EscapeY));

	strcpy(actionInfo.actionName, "escape_non_y");
	strcpy(actionInfo.localizedActionName, "Escape (minus Y)");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &EscapeNonY));

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy(actionInfo.actionName, "quit");
	strcpy(actionInfo.localizedActionName, "Quit");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Quit));

	actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy(actionInfo.actionName, "hand_pose");
	strcpy(actionInfo.localizedActionName, "Hand pose");
	actionInfo.countSubactionPaths = (uint32_t)appState.SubactionPaths.size();
	actionInfo.subactionPaths = appState.SubactionPaths.data();
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &Pose));

	actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy(actionInfo.actionName, "left_key_press");
	strcpy(actionInfo.localizedActionName, "Left key press");
	actionInfo.countSubactionPaths = 0;
	actionInfo.subactionPaths = nullptr;
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &LeftKeyPress));

	strcpy(actionInfo.actionName, "right_key_press");
	strcpy(actionInfo.localizedActionName, "Right key press");
	CHECK_XRCMD(xrCreateAction(ActionSet, &actionInfo, &RightKeyPress));

	XrPath aClick;
	XrPath bClick;
	XrPath xClick;
	XrPath yClick;
	XrPath leftTrigger;
	XrPath rightTrigger;
	XrPath leftSqueeze;
	XrPath rightSqueeze;
	XrPath leftThumbstickX;
	XrPath leftThumbstickY;
	XrPath rightThumbstickX;
	XrPath rightThumbstickY;
	XrPath leftThumbstickClick;
	XrPath rightThumbstickClick;
	XrPath menuClick;
	XrPath leftPose;
	XrPath rightPose;
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/a/click", &aClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/b/click", &bClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/x/click", &xClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/y/click", &yClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/trigger/value", &leftTrigger));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/trigger/value", &rightTrigger));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/squeeze/value", &leftSqueeze));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/squeeze/value", &rightSqueeze));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/thumbstick/x", &leftThumbstickX));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/thumbstick/y", &leftThumbstickY));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/thumbstick/x", &rightThumbstickX));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/thumbstick/y", &rightThumbstickY));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/thumbstick/click", &leftThumbstickClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/thumbstick/click", &rightThumbstickClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/menu/click", &menuClick));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/aim/pose", &leftPose));
	CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/aim/pose", &rightPose));

	XrPath interaction;
	CHECK_XRCMD(xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &interaction));
	std::vector<XrActionSuggestedBinding> bindings
	{
		{ Play1, xClick },
		{ Play2, aClick },
		{ JumpLeftHanded, yClick },
		{ JumpRightHanded, bClick },
		{ SwimDownLeftHanded, xClick },
		{ SwimDownRightHanded, aClick },
		{ Run, leftSqueeze },
		{ Run, rightSqueeze },
		{ Fire, leftTrigger },
		{ Fire, rightTrigger },
		{ MoveX, leftThumbstickX },
		{ MoveX, rightThumbstickX },
		{ MoveY, leftThumbstickY },
		{ MoveY, rightThumbstickY },
		{ SwitchWeapon, leftThumbstickClick },
		{ SwitchWeapon, rightThumbstickClick },
		{ Menu, menuClick },
		{ MenuLeftHanded, aClick },
		{ MenuRightHanded, xClick },
		{ EnterTrigger, leftTrigger },
		{ EnterTrigger, rightTrigger },
		{ EnterNonTriggerLeftHanded, xClick },
		{ EnterNonTriggerRightHanded, aClick },
		{ EscapeY, leftSqueeze },
		{ EscapeY, rightSqueeze },
		{ EscapeY, bClick },
		{ EscapeY, yClick },
		{ EscapeNonY, leftSqueeze },
		{ EscapeNonY, rightSqueeze },
		{ EscapeNonY, bClick },
		{ Quit, yClick },
		{ Pose, leftPose },
		{ Pose, rightPose },
		{ LeftKeyPress, leftTrigger },
		{ RightKeyPress, rightTrigger }
	};

	XrInteractionProfileSuggestedBinding suggestedBindings { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	suggestedBindings.interactionProfile = interaction;
	suggestedBindings.suggestedBindings = bindings.data();
	suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
	CHECK_XRCMD(xrSuggestInteractionProfileBindings(instance, &suggestedBindings));
}

void Actions::LogAction(AppState& appState, XrAction action, const char* name)
{
	XrBoundSourcesForActionEnumerateInfo getInfo { XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO };
	getInfo.action = action;
	uint32_t pathCount = 0;
	CHECK_XRCMD(xrEnumerateBoundSourcesForAction(appState.Session, &getInfo, 0, &pathCount, nullptr));
	std::vector<XrPath> paths(pathCount);
	CHECK_XRCMD(xrEnumerateBoundSourcesForAction(appState.Session, &getInfo, uint32_t(paths.size()), &pathCount, paths.data()));

	std::string sourceName;
	for (uint32_t i = 0; i < pathCount; ++i)
	{
		constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
														XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
														XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

		XrInputSourceLocalizedNameGetInfo nameInfo { XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO };
		nameInfo.sourcePath = paths[i];
		nameInfo.whichComponents = all;

		uint32_t size = 0;
		CHECK_XRCMD(xrGetInputSourceLocalizedName(appState.Session, &nameInfo, 0, &size, nullptr));
		if (size < 1)
		{
			continue;
		}
		std::vector<char> grabSource(size);
		CHECK_XRCMD(xrGetInputSourceLocalizedName(appState.Session, &nameInfo, uint32_t(grabSource.size()), &size, grabSource.data()));
		if (!sourceName.empty())
		{
			sourceName += " and ";
		}
		sourceName += "'";
		sourceName += std::string(grabSource.data(), size - 1);
		sourceName += "'";
	}

	appState.Logger->Info((std::string(name) + " action is bound to %s").c_str(), (!sourceName.empty() ? sourceName.c_str() : "nothing"));
}

void Actions::Log(AppState& appState) const
{
	LogAction(appState, Play1, "Play 1");
	LogAction(appState, Play2, "Play 2");
	LogAction(appState, JumpLeftHanded, "Jump (left handed)");
	LogAction(appState, JumpRightHanded, "Jump (right handed)");
	LogAction(appState, SwimDownLeftHanded, "Swim down (left handed)");
	LogAction(appState, SwimDownRightHanded, "Swim down (right handed)");
	LogAction(appState, Run, "Run");
	LogAction(appState, Fire, "Fire");
	LogAction(appState, MoveX, "Move X");
	LogAction(appState, MoveY, "Move Y");
	LogAction(appState, SwitchWeapon, "Switch weapon");
	LogAction(appState, Menu, "Menu");
	LogAction(appState, MenuLeftHanded, "Menu (left handed)");
	LogAction(appState, MenuRightHanded, "Menu (right handed)");
	LogAction(appState, EnterTrigger, "Enter (with triggers)");
	LogAction(appState, EnterNonTriggerLeftHanded, "Enter (without triggers, left handed)");
	LogAction(appState, EnterNonTriggerRightHanded, "Enter (without triggers, right handed)");
	LogAction(appState, EscapeY, "Escape (plus Y)");
	LogAction(appState, EscapeNonY, "Escape (minus Y)");
	LogAction(appState, Quit, "Quit");
	LogAction(appState, Pose, "Hand pose");
	LogAction(appState, LeftKeyPress, "Left key press");
	LogAction(appState, RightKeyPress, "Right key press");
}