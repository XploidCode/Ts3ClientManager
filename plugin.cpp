/*
* TeamSpeak 3 demo plugin
*
* Copyright (c) 2008-2017 TeamSpeak Systems GmbH
*/

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <thread>
#include <windows.h>




static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 22

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if (WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif




/*********************************** Required functions ************************************/
/*
* If any of these required functions is not implemented, TS3 will refuse to load the plugin
*/

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if (!result) {
		const wchar_t* name = L"ClientManagement";
		if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "ClientManagement";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "ClientManagement";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
	return "1.2";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "!Xploid";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "Use this Plugin to manage the Clientcontrol.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
	ts3Functions = funcs;
}

/*
* Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
* If the function returns 1 on failure, the plugin will be unloaded again.
*/
int ts3plugin_init() {
	char appPath[PATH_BUFSIZE];
	char resourcesPath[PATH_BUFSIZE];
	char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

	/* Your plugin init code here */
	printf("PLUGIN: init\n");

	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0);
	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_4, 0);


	/* Example on how to query application, resources and configuration paths from client */
	/* Note: Console client returns empty string for app and resources path */
	ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
	ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
	ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);
	ts3Functions.printMessageToCurrentTab("[color=#FF0000]C[/color][color=#F70007]l[/color][color=#F0000E]i[/color][color=#E90015]e[/color][color=#E1001D]n[/color][color=#DA0024]t[/color][color=#D3002B]M[/color][color=#CC0033]a[/color][color=#C4003A]n[/color][color=#BD0041]a[/color][color=#B60048]g[/color][color=#AE0050]e[/color][color=#A70057]r[/color] [color=#990066]b[/color][color=#91006D]y[/color] [color=#83007B]![/color][color=#7B0083]X[/color][color=#74008A]p[/color][color=#6D0091]l[/color][color=#660099]o[/color][color=#5E00A0]i[/color][color=#5700A7]d[/color] [color=#4800B6]a[/color][color=#4100BD]c[/color][color=#3A00C4]t[/color][color=#3300CB]i[/color][color=#2B00D3]v[/color][color=#2400DA]a[/color][color=#1D00E1]t[/color][color=#1500E9]e[/color][color=#0E00F0]d[/color][color=#0700F7].[/color]");
	return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
			   /* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
			   * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
			   * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}
bool movename = false;
/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
	/* Your plugin cleanup code here */
	printf("PLUGIN: shutdown\n");
	if (movename) {
		movename = false;
	}
	ts3Functions.printMessageToCurrentTab("[color=#FF0000]C[/color][color=#F2000C]l[/color][color=#E60018]i[/color][color=#DA0024]e[/color][color=#CE0030]n[/color][color=#C2003C]t[/color][color=#B60048]M[/color][color=#AA0054]a[/color][color=#9D0061]n[/color][color=#91006D]a[/color][color=#850079]g[/color][color=#790085]e[/color][color=#6D0091]r[/color] [color=#5500A9]c[/color][color=#4800B6]l[/color][color=#3C00C2]o[/color][color=#3000CE]s[/color][color=#2400DA]e[/color][color=#1800E6]d[/color][color=#0C00F2].[/color]");
	/*
	* Note:
	* If your plugin implements a settings dialog, it must be closed and deleted here, else the
	* TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	*/

	/* Free pluginID if we registered it */
	if (pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
* Following functions are optional, if not needed you don't need to implement them.
*/

/*
* If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
* automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
* Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
*/
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

const char* ts3plugin_commandKeyword() {
	return "ClientManager";
}

void MoveName(uint64, char*, char*);

int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	char* commandbuffer = strdup(command);



	char* splitted = strtok(commandbuffer, " ");
	

	if (strcmp(splitted, "movename") == 0) {


		if (movename) {
			movename = !movename;
		}
		else {
			movename = !movename;
			std::thread movenamethread(MoveName, serverConnectionHandlerID, strtok(NULL, " "), strtok(NULL, " "));
			movenamethread.detach();
		}
		return 0;
	}

	if (strcmp(splitted, "commands") == 0 || strcmp(splitted, "Kommandos") == 0 || strcmp(splitted, "hilfe") == 0 || strcmp(splitted, "Hilfe") == 0 || strcmp(splitted, "help") == 0){
		ts3Functions.printMessageToCurrentTab("Commands: \n\t /ClientManager movename firsttag lasttag\t\tanimates Tags between your Nickname. Stop with same Command.");
		return 0;
	}
	


}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
* Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
* the user manually disabled it in the plugin dialog.
* This function is optional. If missing, no autoload is assumed.
*/
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

/*
* Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
* ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
* These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
*/

/*
* Initialize plugin menus.
* This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
* Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
* If plugin menus are not used by a plugin, do not implement this function or return NULL.
*/
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	* Create the menus
	* There are three types of menu items:
	* - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	* - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	* - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	*
	* Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	*
	* The menu text is required, max length is 128 characters
	*
	* The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	* Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	* plugin filename, without dll/so/dylib suffix
	* e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	*/

	BEGIN_CREATE_MENUS(9);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_1, "Copy ID", "1.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_2, "Client item 2", "2.png");

	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_1, "Channel item 1", "1.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_2, "Channel item 2", "2.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_3, "Channel item 3", "3.png");


	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_1, "activate Antipoke", "1.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_2, "deactivate Antipoke", "1.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_3, "activate Antikick", "1.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_4, "deactivate Antikick", "1.png");






	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

					   /*
					   * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
					   * If unused, set menuIcon to NULL
					   */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

	/*
	* Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
	* Test it with plugin command: /test enablemenu <menuID> <0|1>
	* Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
	* ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
	* menu is displayed.
	*/
	/* For example, this would disable MENU_ID_GLOBAL_2: */
	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}



void ts3plugin_onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID) {
}

void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
}

void ts3plugin_onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility) {
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage) {
}




bool antikick = false;

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {


	if (antikick) {

		anyID MyID;
		if (!ts3Functions.getClientID(serverConnectionHandlerID, &MyID)) {

			int clientid = (int64_t)clientID;
			char clientSID[TS3_MAX_SIZE_TEXTMESSAGE];
			itoa(clientid, clientSID, 10);



			char kickstr[TS3_MAX_SIZE_TEXTMESSAGE] = "[COLOR=#dd001d]Du wurdest von [/COLOR]";

			strcat_s(kickstr, "[URL=client://");
			strcat_s(kickstr, clientSID);
			strcat_s(kickstr, "/");
			strcat_s(kickstr, kickerUniqueIdentifier);
			strcat_s(kickstr, "]");
			strcat_s(kickstr, kickerName);
			strcat_s(kickstr, "[/URL]");


			strcat_s(kickstr, "[COLOR=#dd001d] gekickt![/COLOR]");



			if (MyID == clientID) {
				ts3Functions.printMessage(serverConnectionHandlerID, kickstr, PLUGIN_MESSAGE_TARGET_CHANNEL);
				ts3Functions.requestClientMove(serverConnectionHandlerID, clientID, oldChannelID, "", NULL);
			}
		}





	}
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientIDsEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, anyID clientID, const char* clientName) {
}

void ts3plugin_onClientIDsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerEditedEvent(uint64 serverConnectionHandlerID, anyID editerID, const char* editerName, const char* editerUniqueIdentifier) {
}

void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID) {
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
	return 0;
}

void ts3plugin_onServerStopEvent(uint64 serverConnectionHandlerID, const char* shutdownMessage) {
}

int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored) {
	return 0;
}

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
}

void ts3plugin_onConnectionInfoEvent(uint64 serverConnectionHandlerID, anyID clientID) {
}

void ts3plugin_onServerConnectionInfoEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelSubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelUnsubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelUnsubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelDescriptionUpdateEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelPasswordChangedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onPlaybackShutdownCompleteEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onSoundDeviceListChangedEvent(const char* modeID, int playOrCap) {
}

void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels) {
}

void ts3plugin_onEditPostProcessVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditCapturedVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, int* edited) {
}

void ts3plugin_onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance, float* volume) {
}

void ts3plugin_onCustom3dRolloffCalculationWaveEvent(uint64 serverConnectionHandlerID, uint64 waveHandle, float distance, float* volume) {
}

void ts3plugin_onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString) {
}

/* Clientlib rare */

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time, const char* kickMessage) {
}

bool ignorepoke = false;

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored) {


	if (ignorepoke) {

		char empty[TS3_MAX_SIZE_TEXTMESSAGE] = "";
		if (!(message == empty)) {
			char clientSID[TS3_MAX_SIZE_TEXTMESSAGE];
			itoa((int64_t)fromClientID, clientSID, 10);

			char pokemsg[TS3_MAX_SIZE_TEXTMESSAGE] = "[URL=client://";

			strcat_s(pokemsg, clientSID);
			strcat_s(pokemsg, "/");
			strcat_s(pokemsg, pokerUniqueIdentity);
			strcat_s(pokemsg, "]");
			strcat_s(pokemsg, pokerName);
			strcat_s(pokemsg, "[/URL] stupst sie an: ");
			strcat_s(pokemsg, message);

			ts3Functions.printMessage(serverConnectionHandlerID, pokemsg, PLUGIN_MESSAGE_TARGET_CHANNEL);

			return 1;
		}
		else {

		}


		return 0;
	}
	/* 0 = handle normally, 1 = client will ignore the poke */
}

void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue) {
}

void ts3plugin_onFileListEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path, const char* name, uint64 size, uint64 datetime, int type, uint64 incompletesize, const char* returnCode) {
}

void ts3plugin_onFileListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path) {
}

void ts3plugin_onFileInfoEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* name, uint64 size, uint64 datetime) {
}

void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupByClientIDEvent(uint64 serverConnectionHandlerID, const char* name, uint64 serverGroupList, uint64 clientDatabaseID) {
}

void ts3plugin_onServerGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onServerGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID) {
}

void ts3plugin_onServerGroupClientListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, uint64 clientDatabaseID, const char* clientNameIdentifier, const char* clientUniqueID) {
}

void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID) {
}

void ts3plugin_onChannelPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onClientPermListEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID) {
}

void ts3plugin_onChannelClientPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID) {
}

void ts3plugin_onClientChannelGroupChangedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, uint64 channelID, anyID clientID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

int ts3plugin_onServerPermissionErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, unsigned int failedPermissionID) {
	return 0;  /* See onServerErrorEvent for return code description */
}

void ts3plugin_onPermissionListGroupEndIDEvent(uint64 serverConnectionHandlerID, unsigned int groupEndID) {
}

void ts3plugin_onPermissionListEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, const char* permissionName, const char* permissionDescription) {
}

void ts3plugin_onPermissionListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onPermissionOverviewEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, uint64 channelID, int overviewType, uint64 overviewID1, uint64 overviewID2, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onPermissionOverviewFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupClientAddedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onServerGroupClientDeletedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onClientNeededPermissionsEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, int permissionValue) {
}

void ts3plugin_onClientNeededPermissionsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onFileTransferStatusEvent(anyID transferID, unsigned int status, const char* statusMessage, uint64 remotefileSize, uint64 serverConnectionHandlerID) {
}

void ts3plugin_onClientChatClosedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onClientChatComposingEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onServerLogEvent(uint64 serverConnectionHandlerID, const char* logMsg) {
}

void ts3plugin_onServerLogFinishedEvent(uint64 serverConnectionHandlerID, uint64 lastPos, uint64 fileSize) {
}

void ts3plugin_onMessageListEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, uint64 timestamp, int flagRead) {
}

void ts3plugin_onMessageGetEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, const char* message, uint64 timestamp) {
	//printf(message);
}

void ts3plugin_onClientDBIDfromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID) {
}

void ts3plugin_onClientNamefromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onClientNamefromDBIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onComplainListEvent(uint64 serverConnectionHandlerID, uint64 targetClientDatabaseID, const char* targetClientNickName, uint64 fromClientDatabaseID, const char* fromClientNickName, const char* complainReason, uint64 timestamp) {
}

void ts3plugin_onBanListEvent(uint64 serverConnectionHandlerID, uint64 banid, const char* ip, const char* name, const char* uid, uint64 creationTime, uint64 durationTime, const char* invokerName,
	uint64 invokercldbid, const char* invokeruid, const char* reason, int numberOfEnforcements, const char* lastNickName) {
}

void ts3plugin_onClientServerQueryLoginPasswordEvent(uint64 serverConnectionHandlerID, const char* loginPassword) {
}

void ts3plugin_onPluginCommandEvent(uint64 serverConnectionHandlerID, const char* pluginName, const char* pluginCommand) {
}

void ts3plugin_onIncomingClientQueryEvent(uint64 serverConnectionHandlerID, const char* commandText) {
}

void ts3plugin_onServerTemporaryPasswordListEvent(uint64 serverConnectionHandlerID, const char* clientNickname, const char* uniqueClientIdentifier, const char* description, const char* password, uint64 timestampStart, uint64 timestampEnd, uint64 targetChannelID, const char* targetChannelPW) {
}

/* Client UI callbacks */

/*
* Called from client when an avatar image has been downloaded to or deleted from cache.
* This callback can be called spontaneously or in response to ts3Functions.getAvatar()
*/
void ts3plugin_onAvatarUpdated(uint64 serverConnectionHandlerID, anyID clientID, const char* avatarPath) {
	/* If avatarPath is NULL, the avatar got deleted */
	/* If not NULL, avatarPath contains the path to the avatar file in the TS3Client cache */
	if (avatarPath != NULL) {
		printf("onAvatarUpdated: %llu %d %s\n", (long long unsigned int)serverConnectionHandlerID, clientID, avatarPath);
	}
	else {
		printf("onAvatarUpdated: %llu %d - deleted\n", (long long unsigned int)serverConnectionHandlerID, clientID);
	}
}

/*
* Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
*
* Parameters:
* - serverConnectionHandlerID: ID of the current server tab
* - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
* - menuItemID: Id used when creating the menu item
* - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
*/
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	printf("PLUGIN: onMenuIteEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);
	switch (type) {
	case PLUGIN_MENU_TYPE_GLOBAL:
		/* Global menu item was triggered. selectedItemID is unused and set to zero. */
		switch (menuItemID) {
		case MENU_ID_GLOBAL_1:
			/* Menu global 1 was triggered */
			ignorepoke = !ignorepoke;

			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_1, 0);
			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 1);

			break;
		case MENU_ID_GLOBAL_2:
			/* Menu global 2 was triggered */
			ignorepoke = !ignorepoke;

			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0);
			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_1, 1);

			break;
		case MENU_ID_GLOBAL_3:
			antikick = !antikick;

			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_3, 0);
			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_4, 1);

			break;
		case MENU_ID_GLOBAL_4:
			antikick = !antikick;

			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_4, 0);
			ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_3, 1);

			break;
		default:
			break;
		}
		break;
	case PLUGIN_MENU_TYPE_CHANNEL:
		/* Channel contextmenu item was triggered. selectedItemID is the channelID of the selected channel */
		switch (menuItemID) {
		case MENU_ID_CHANNEL_1:

			for (int i = 0; i < 1000; i++) {


			}


			/* Menu channel 1 was triggered */
			break;
		case MENU_ID_CHANNEL_2:
			/* Menu channel 2 was triggered */
			break;
		case MENU_ID_CHANNEL_3:
			/* Menu channel 3 was triggered */
			break;
		default:
			break;
		}
		break;
	case PLUGIN_MENU_TYPE_CLIENT:
		/* Client contextmenu item was triggered. selectedItemID is the clientID of the selected client */
		switch (menuItemID) {
		case MENU_ID_CLIENT_1: {
			/* Menu client 1 was triggered */

			char* userid;
			ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_UNIQUE_IDENTIFIER, &userid);


			size_t len = strlen(userid) + 1;
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
			memcpy(GlobalLock(hMem), userid, len);
			GlobalUnlock(hMem);
			OpenClipboard(0);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, userid);
			CloseClipboard();

			break;
		}
		case MENU_ID_CLIENT_2:
			/* Menu client 2 was triggered */
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}



void MoveName(uint64 serverConnectionHandlerID, char* firstletter = ".", char* lastletter = ":") {
	anyID myid;
	ts3Functions.getClientID(serverConnectionHandlerID, &myid);
	char Nickname[TS3_MAX_SIZE_TEXTMESSAGE];
	ts3Functions.getClientDisplayName(serverConnectionHandlerID ,myid, Nickname, 1024);
	bool nametag = true;

	std::string NewNickname = "";

	while (movename) {
		Sleep(2000);
		if (nametag) {
			nametag = !nametag;
			 NewNickname = firstletter + std::string(Nickname) + firstletter;
		}
		else {
			nametag = !nametag;
			NewNickname = lastletter + std::string(Nickname) + lastletter;
		}
		ts3Functions.setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_NICKNAME, NewNickname.c_str());
		ts3Functions.flushClientSelfUpdates(serverConnectionHandlerID, NULL);

	}
	ts3Functions.setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_NICKNAME, Nickname);
	ts3Functions.flushClientSelfUpdates(serverConnectionHandlerID, NULL);

}

/* This function is called if a plugin hotkey was pressed. Omit if hotkeys are unused. */
void ts3plugin_onHotkeyEvent(const char* keyword) {
	printf("PLUGIN: Hotkey event: %s\n", keyword);
	/* Identify the hotkey by keyword ("keyword_1", "keyword_2" or "keyword_3" in this example) and handle here... */
}

/* Called when recording a hotkey has finished after calling ts3Functions.requestHotkeyInputDialog */
void ts3plugin_onHotkeyRecordedEvent(const char* keyword, const char* key) {
}

// This function receives your key Identifier you send to notifyKeyEvent and should return
// the friendly device name of the device this hotkey originates from. Used for display in UI.
const char* ts3plugin_keyDeviceName(const char* keyIdentifier) {
	return NULL;
}

// This function translates the given key identifier to a friendly key name for display in the UI
const char* ts3plugin_displayKeyText(const char* keyIdentifier) {
	return NULL;
}

// This is used internally as a prefix for hotkeys so we can store them without collisions.
// Should be unique across plugins.
const char* ts3plugin_keyPrefix() {
	return NULL;
}

/* Called when client custom nickname changed */
void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier) {
}
