/*
  xdrv_100_1_robonomics.ino.ino - Robonomics Network support for Tasmota
*/

#define USE_ROBONOMICS
#ifdef USE_ROBONOMICS
/*********************************************************************************************\
 * My IoT Device with command support
 *
 *
\*********************************************************************************************/

#warning **** Robonomics Driver is included... ****

#define XDRV_100 100

/* Не #include <Ed25519.h>: на Windows совпадает с tasmota/include/ed25519.h */
#include <address.h>
#include <Robonomics.h>
#include "Utils.h"
#include <nvs.h>
#include <nvs_flash.h>
// #include <RpcRobonomics.h>

#define D_CMND_ROBONOMICSEP "SetEndpointHost"
#define D_CMND_ROBONOMICSRWSO "SetRWSOwner"
#define D_CMND_ROBONOMICS_USE_RWS "UseRWS"

#define ROBONOMICS_STORAGE "robonomics"
#define USE_RWS_STORAGE_KEY "use_rws"
#define RWS_OWNER_STORAGE_KEY "rws_owner"
#define PRIVATE_KEY_STORAGE_KEY "private_key"
#define ENDPOINT_STORAGE_KEY "endpoint"

uint8_t robonomicsPrivateKey[32];
uint8_t robonomicsPublicKey[32];
std::string robonomicsSs58Address;
// std::string robonomics_url = "polkadot.rpc.robonomics.network";
String robonomics_host;
bool use_rws = true;
bool owner_is_set = false;
char rws_owner[50];

Robonomics robonomics;

/*********************************************************************************************\
 * My IoT Device Functions
\*********************************************************************************************/

// This variable will be set to true after initialization
bool initSuccess = false;

/*
  Commands:
    PrintAddress         - Return and print ESP Robonomics address
    SendDatalog          - Send Datalog eith the text from parameter, example: `SendDatalog Hello, World!`
    GeneratePrivateKey   - Generate new account on ESP
    UseRWS               - Use RWS for sending datalogs (UseRWS 1), or use pay for each transaction (UseRWS 0)
    SetRWSOwner          - Set owner for RWS transactions (SetRWSOwner <address>)
    SetEndpointHost      - Set public node endpoint (SetEndpointHost polkadot.rpc.robonomics.network)
    GetParams            - Return JSON with current Robonomics driver settings
*/

void initializeNVS()
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
}

void saveString(const char *key, const char *value)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(ROBONOMICS_STORAGE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return;
  }

  err = nvs_set_str(my_handle, key, value);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) writing string!\n", esp_err_to_name(err));
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) committing changes!\n", esp_err_to_name(err));
  }

  nvs_close(my_handle);
}

bool loadString(const char *key, char *value, size_t maxLength)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(ROBONOMICS_STORAGE, NVS_READONLY, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return false;
  }

  size_t required_size = 0;
  err = nvs_get_str(my_handle, key, NULL, &required_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
  {
    Serial.printf("Error (%s) reading string size!\n", esp_err_to_name(err));
    nvs_close(my_handle);
    return false;
  }

  if (required_size > 0 && required_size <= maxLength)
  {
    err = nvs_get_str(my_handle, key, value, &required_size);
    if (err != ESP_OK)
    {
      Serial.printf("Error (%s) reading string!\n", esp_err_to_name(err));
      nvs_close(my_handle);
      return false;
    }
    nvs_close(my_handle);
    return true;
  }

  nvs_close(my_handle);
  return false;
}

void saveBool(const char *key, bool value)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(ROBONOMICS_STORAGE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return;
  }

  err = nvs_set_u8(my_handle, key, value ? 1 : 0);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) writing boolean!\n", esp_err_to_name(err));
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) committing changes!\n", esp_err_to_name(err));
  }

  nvs_close(my_handle);
}

bool loadBool(const char *key, bool *value)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(ROBONOMICS_STORAGE, NVS_READONLY, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return false;
  }

  uint8_t stored_value = 0;
  err = nvs_get_u8(my_handle, key, &stored_value);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
  {
    Serial.printf("Error (%s) reading boolean!\n", esp_err_to_name(err));
    nvs_close(my_handle);
    return false;
  }

  *value = (stored_value != 0);

  nvs_close(my_handle);
  return true;
}

void savePrivateKey(const uint8_t *privateKey, size_t length)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(ROBONOMICS_STORAGE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return;
  }

  err = nvs_set_blob(my_handle, PRIVATE_KEY_STORAGE_KEY, privateKey, length);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) writing private key!\n", esp_err_to_name(err));
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) committing changes!\n", esp_err_to_name(err));
  }

  nvs_close(my_handle);
}

bool loadPrivateKey(uint8_t *privateKey, size_t length)
{
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(ROBONOMICS_STORAGE, NVS_READONLY, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    return false;
  }

  size_t required_size = 0; // value will default to 0, if not set yet in NVS
  err = nvs_get_blob(my_handle, PRIVATE_KEY_STORAGE_KEY, NULL, &required_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
  {
    Serial.printf("Error (%s) reading private key size!\n", esp_err_to_name(err));
    nvs_close(my_handle);
    return false;
  }

  if (required_size == length)
  {
    err = nvs_get_blob(my_handle, PRIVATE_KEY_STORAGE_KEY, privateKey, &required_size);
    if (err != ESP_OK)
    {
      Serial.printf("Error (%s) reading private key!\n", esp_err_to_name(err));
      nvs_close(my_handle);
      return false;
    }
    nvs_close(my_handle);
    return true;
  }

  nvs_close(my_handle);
  return false;
}

const char RobonomicsCommands[] PROGMEM = "|"
  "PrintAddress|SendDatalog|GeneratePrivateKey|UseRWS|SetRWSOwner|SetEndpointHost|GetParams";

void (*const RobonomicsCommand[])(void) PROGMEM = {
  &CmdPrint_Address, 
  &CmdSend_Datalog, 
  &CmdGenerate_Private_Key, 
  &CmdUse_RWS, 
  &CmdSet_RWS_Owner, 
  &CmdSet_Endpoint_Host,
  &CmdGet_Params
};

void CmdPrint_Address(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("Robonomics address: %s"), robonomics.getSs58Address());
  ResponseCmndChar_P(robonomics.getSs58Address());
}

void CmdSend_Datalog(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("Sending Datalog: %s, use RWS: %d, owner is set: %d"), XdrvMailbox.data, use_rws, owner_is_set);
  const char *res;
  if (use_rws && owner_is_set) {
    res = robonomics.sendRWSDatalogRecord(XdrvMailbox.data, rws_owner);
  } else {
    res = robonomics.sendDatalogRecord(XdrvMailbox.data);
  }
  ResponseCmndChar(res);
}

void CmdUse_RWS(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR(strcmp(XdrvMailbox.data, "1") == 0. ? "Use RWS for sending datalogs" : "Don't use RWS for sending datalogs"));
  use_rws = strcmp(XdrvMailbox.data, "1") == 0;
  saveBool(USE_RWS_STORAGE_KEY, use_rws);
  ResponseCmndDone();
}

void CmdSet_RWS_Owner(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("Set new RWS owner: %s"), XdrvMailbox.data);
  strcpy(rws_owner, XdrvMailbox.data);
  saveString(RWS_OWNER_STORAGE_KEY, rws_owner);
  owner_is_set = true;
  ResponseCmndDone();
}

void CmdSet_Endpoint_Host(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("Set new Robonomics endpoint: %s"), XdrvMailbox.data);
  robonomics_host = String(XdrvMailbox.data);
  robonomics.setup(robonomics_host);
  saveString(ENDPOINT_STORAGE_KEY, robonomics_host.c_str());
  ResponseCmndDone();
}

void CmdGenerate_Private_Key(void)
{
  AddLog(LOG_LEVEL_INFO, PSTR("Regenerate privete key"));
  robonomics.generateAndSetPrivateKey();
  hex2bytes(robonomics.getPrivateKey(), robonomicsPrivateKey);
  savePrivateKey(robonomicsPrivateKey, sizeof(robonomicsPrivateKey));
  ResponseCmndChar_P(robonomics.getSs58Address());
}

void CmdGet_Params(void)
{
  JSONVar params;
  params["endpoint"] = robonomics_host;
  params["use_rws"] = use_rws;
  params["address"] = robonomics.getSs58Address();
  params["rws_owner"] = rws_owner;
  String params_str = JSON.stringify(params);
  AddLog(LOG_LEVEL_INFO, PSTR("Rarameters: %s"), params_str.c_str());
  ResponseCmndChar(params_str.c_str());
}

/*********************************************************************************************\
 * Tasmota Functions
\*********************************************************************************************/

void RobonomicsInit()
{
  AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("Robonomics init..."));
  initializeNVS();

  if (!loadBool(USE_RWS_STORAGE_KEY, &use_rws)) {
    AddLog(LOG_LEVEL_INFO, PSTR("No use_rws parameter in flash"));
    use_rws = false;
  }
  AddLog(LOG_LEVEL_INFO, PSTR("use_rws: %d"), use_rws);

  if (!loadString(RWS_OWNER_STORAGE_KEY, rws_owner, sizeof(rws_owner))) {
    AddLog(LOG_LEVEL_INFO, PSTR("No rws_owner parameter in flash"));
#ifdef DEFAULT_RWS_OWNER
    strncpy(rws_owner, DEFAULT_RWS_OWNER, sizeof(rws_owner) - 1);
    rws_owner[sizeof(rws_owner) - 1] = '\0';
    saveString(RWS_OWNER_STORAGE_KEY, rws_owner);
    owner_is_set = true;
    AddLog(LOG_LEVEL_INFO, PSTR("rws_owner (default): %s"), rws_owner);
#else
    use_rws = false;
    owner_is_set = false;
    AddLog(LOG_LEVEL_INFO, PSTR("use_rws: %d"), use_rws);
#endif
  } else {
    AddLog(LOG_LEVEL_INFO, PSTR("rws_owner: %s"), rws_owner);
    owner_is_set = true;
  }

  char endpoint[80];
  if (!loadString(ENDPOINT_STORAGE_KEY, endpoint, sizeof(endpoint))) {
    AddLog(LOG_LEVEL_INFO, PSTR("No endpoint parameter in flash"));
    robonomics_host = "kusama.rpc.robonomics.network";
  } else {
    robonomics_host = String(endpoint);
  }

  AddLog(LOG_LEVEL_INFO, PSTR("Robonomics endpoint: %s"), robonomics_host.c_str());
  if (!loadPrivateKey(robonomicsPrivateKey, sizeof(robonomicsPrivateKey)))
  {
    robonomics.generateAndSetPrivateKey();
    hex2bytes(robonomics.getPrivateKey(), robonomicsPrivateKey);
    savePrivateKey(robonomicsPrivateKey, sizeof(robonomicsPrivateKey));
  }
  robonomics.setPrivateKey(robonomicsPrivateKey);
  robonomics.setup(robonomics_host);
  initSuccess = true;
}

void RobonomicsProcessing(void)
{

  /*
    Here goes My Project code.
    Usually this part is included into loop() function
  */
}

/*********************************************************************************************\
 * Presentation
\*********************************************************************************************/

#ifdef USE_WEBSERVER

#define WEB_HANDLE_ROBONOMICS "rob"

const char S_CONFIGURE_ROBONOMICS[] PROGMEM = "Configure Robonomics";

const char HTTP_BTN_MENU_ROBONOMICS[] PROGMEM =
  "<p><form action='" WEB_HANDLE_ROBONOMICS "' method='get'><button>Configure Robonomics</button></form></p>";

const char HTTP_FORM_ROBONOMICS1[] PROGMEM =
  "<fieldset><legend><b>&nbsp;Robonomics Parameters&nbsp;</b></legend>"
  "<form method='get' action='" WEB_HANDLE_ROBONOMICS "'>"
  "<p><b>Endpoint</b> (polkadot.rpc.robonomics.network)<br><select id='endpoint'>"
  "<option value='polkadot.rpc.robonomics.network'%s>polkadot.rpc.robonomics.network</option>"
  "<option value='kusama.rpc.robonomics.network'%s>kusama.rpc.robonomics.network</option>"
  "</select></p>"
  "<p><label><input id='use_rws' type='checkbox'%s><b>Use RWS</b></label></p>"
  "<p><b>RWS Owner</b><br><input id='rws_owner' placeholder='RWS Owner' value='%s'></p>";

const char HTTP_FORM_ROBONOMICS2[] PROGMEM =
  "<fieldset><legend><b>&nbsp;Datalog&nbsp;</b></legend>"
  "<p><b>Datalog</b><br><input id='datalog' placeholder='Enter datalog'></p>"
  "<p><button type='submit' name='send_datalog' value='1'>Send</button></p>";


void HandleRobonomicsConfiguration(void)
{
  if (!HttpCheckPriviledgedAccess()) { return; }

  AddLog(LOG_LEVEL_INFO, PSTR(D_LOG_HTTP "Configure Robonomics"));

  if (Webserver->hasArg(F("send_datalog"))) {
    String datalog = Webserver->arg(F("datalog"));
    String cmnd = "SendDatalog " + datalog;
    ExecuteCommand(cmnd.c_str(), SRC_BUTTON);
  }

  if (Webserver->hasArg(F("save"))) {
    RobonomicsSaveSettings();
    WebRestart(1);
    return;
  }

  char str[TOPSZ];
  char endpoint_pol_selected[10] = "";
  char endpoint_kus_selected[10] = "";
  char use_rws_checked[9] = "";
  String robonomics_endpoint = robonomics_host;
  String robonomics_rws_owner = String(rws_owner);
  bool robonomics_use_rws = use_rws;

  if (robonomics_endpoint == "polkadot.rpc.robonomics.network") {
    strcpy(endpoint_pol_selected, " selected");
  } else {
    strcpy(endpoint_kus_selected, " selected");
  }

  if (robonomics_use_rws) {
    strcpy(use_rws_checked, " checked");
  }

  WSContentStart_P(S_CONFIGURE_ROBONOMICS);
  WSContentSendStyle();
  WSContentSend_P("<p><b>Address:</b> %s</p>", robonomics.getSs58Address());
  WSContentSend_P(HTTP_FORM_ROBONOMICS1, endpoint_pol_selected, endpoint_kus_selected, use_rws_checked, robonomics_rws_owner.c_str());
  WSContentSend_P(HTTP_FORM_ROBONOMICS2);
  WSContentSend_P(HTTP_FORM_END);
  WSContentSpaceButton(BUTTON_MAIN);
  WSContentStop();
}

void RobonomicsSaveSettings(void) {
  String cmnd = F(D_CMND_BACKLOG "0 ");
  cmnd += AddWebCommand(PSTR(D_CMND_ROBONOMICSEP), PSTR("endpoint"), PSTR("1"));
  cmnd += AddWebCommand(PSTR(D_CMND_ROBONOMICSRWSO), PSTR("rws_owner"), PSTR("1"));
  cmnd += F(";" D_CMND_ROBONOMICS_USE_RWS " ");
  cmnd += Webserver->hasArg(F("use_rws")) ? "1" : "0";
  ExecuteWebCommand((char*)cmnd.c_str());
}
#endif  // USE_WEBSERVER


/*********************************************************************************************\
 * Interface
\*********************************************************************************************/
  
bool Xdrv100(uint32_t function)
{

  bool result = false;

  if (FUNC_INIT == function)
  {
    RobonomicsInit();
    AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("My project init is done..."));
  }
  else if (initSuccess)
  {

    switch (function)
    {
#ifdef USE_WEBSERVER
    case FUNC_WEB_ADD_MAIN_BUTTON:
      WSContentSend_PD(HTTP_BTN_MENU_ROBONOMICS);
      break;
    case FUNC_WEB_ADD_HANDLER:
        WebServer_on(PSTR("/" WEB_HANDLE_ROBONOMICS), HandleRobonomicsConfiguration);
        break;
#endif  // USE_WEBSERVER
      // Select suitable interval for polling your function
    case FUNC_EVERY_SECOND:
      //   case FUNC_EVERY_250_MSECOND:
      //    case FUNC_EVERY_200_MSECOND:
      //    case FUNC_EVERY_100_MSECOND:
      RobonomicsProcessing();
      break;

    // Command support
    case FUNC_COMMAND:
      AddLog(LOG_LEVEL_DEBUG_MORE, PSTR("Calling Robonomics Command..."));
      result = DecodeCommand(RobonomicsCommands, RobonomicsCommand);
      break;
    }
  }

  return result;
}

#endif // USE_MY_PROJECT_CMD