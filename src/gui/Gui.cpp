///
/// Created by Anonymous275 on 12/27/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///
// clang-format off
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/dc.h>
#include <wx/dcbuffer.h>
#include <wx/fs_inet.h>
#include <wx/graphics.h>
#include <wx/hyperlink.h>
#include <wx/statline.h>
#include <wx/webview.h>
#include <wx/wx.h>
#include <wx/wxhtml.h.>
#include <wx/filepicker.h>
#include <tomlplusplus/toml.hpp>
#include <comutil.h>
#include <ShlObj.h>
#include <fstream>
#include "Json.h"
#include "Launcher.h"
#include "Logger.h"
#include "HttpAPI.h"
#include <thread>
#endif
// clang-format on

/////////// Inherit App class ///////////
class MyApp : public wxApp {
   public:
   bool OnInit() override;
};

/////////// AccountFrame class ///////////
class MyAccountFrame : public wxFrame {
   public:
   MyAccountFrame();

   private:
   static inline wxTextCtrl *ctrlUsername, *ctrlPassword;
   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
   void OnClickRegister(wxCommandEvent& event);
   void OnClickLogout(wxCommandEvent& event);
   void OnClickLogin(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};

/////////// MainFrame class ///////////
class MyMainFrame : public wxFrame {
   public:
   MyMainFrame();
   static void GameVersionLabel();
   static inline MyAccountFrame* AccountFrame;
   static inline MyMainFrame* MainFrameInstance;
   static inline std::thread UpdateThread;
   wxGauge* UpdateBar;
   wxStaticText* txtUpdate;
   wxBitmapButton* BitAccount;
   void OnClickAccount(wxCommandEvent& event);
   wxButton* btnLaunch;

   private:
   wxStaticText* txtStatusResult;
   static inline wxStaticText *txtGameVersion, *txtPlayers, *txtModVersion, *txtServers;

   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
   void GetStats();

   void OnClickSettings(wxCommandEvent& event);
   void OnClickLaunch(wxCommandEvent& event);
   void OnClickLogo(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};

/////////// SettingsFrame class ///////////
class MySettingsFrame : public wxFrame {
   public:
   MySettingsFrame();
   void UpdateInfo();
   void UpdateGameDirectory(const std::string& path);
   void UpdateProfileDirectory(const std::string& path);
   void UpdateCacheDirectory(const std::string& path);

   private:
   wxCheckBox* checkConsole;
   wxDirPickerCtrl *ctrlGameDirectory, *ctrlProfileDirectory, *ctrlCacheDirectory;
   wxChoice* choiceController;

   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();

   void OnClickConsole(wxCommandEvent& event);
   void OnChangedGameDir(wxFileDirPickerEvent& event);
   void OnChangedProfileDir(wxFileDirPickerEvent& event);
   void OnChangedCacheDir(wxFileDirPickerEvent& event);
   void OnChangedBuild(wxCommandEvent& event);
   void OnAutoDetectGame(wxCommandEvent& event);
   void OnAutoDetectProfile(wxCommandEvent& event);
   void OnResetCache(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};

// clang-format off
/////////// Event Tables ///////////
// MainFrame (ID range 1 to 99):
wxBEGIN_EVENT_TABLE(MyMainFrame, wxFrame)
   EVT_BUTTON(1, MyMainFrame::OnClickAccount)
   EVT_BUTTON(2, MyMainFrame::OnClickSettings)
   EVT_BUTTON(3, MyMainFrame::OnClickLaunch)
   EVT_BUTTON(4, MyMainFrame::OnClickLogo)
wxEND_EVENT_TABLE()

// AccountFrame (ID range 100 to 199):
wxBEGIN_EVENT_TABLE(MyAccountFrame, wxFrame)
   EVT_BUTTON(100, MyAccountFrame::OnClickLogout)
   EVT_BUTTON(101, MyAccountFrame::OnClickRegister)
   EVT_BUTTON(102, MyAccountFrame::OnClickLogin)
wxEND_EVENT_TABLE()

// SettingsFrame (ID range 200 to 299):
wxBEGIN_EVENT_TABLE(MySettingsFrame, wxFrame)
   EVT_DIRPICKER_CHANGED(200, MySettingsFrame::OnChangedGameDir)
   EVT_DIRPICKER_CHANGED(201, MySettingsFrame::OnChangedProfileDir)
   EVT_DIRPICKER_CHANGED(202, MySettingsFrame::OnChangedCacheDir)
   EVT_BUTTON(203, MySettingsFrame::OnAutoDetectGame)
   EVT_BUTTON(204, MySettingsFrame::OnAutoDetectProfile)
   EVT_BUTTON(205, MySettingsFrame::OnResetCache)
   EVT_CHOICE(206, MySettingsFrame::OnChangedBuild)
   EVT_CHECKBOX(207, MySettingsFrame::OnClickConsole)
wxEND_EVENT_TABLE();
// clang-format on

/////////// Get Stats Function ///////////
void MyMainFrame::GetStats() {
   std::string results = HTTP::Get("https://backend.beammp.com/stats_raw");

   nlohmann::json jf = nlohmann::json::parse(results, nullptr, false);
   if (!jf.is_discarded() && !jf.empty()) {
      txtPlayers->SetLabel(to_string(jf["Players"]));
      txtServers->SetLabel(to_string(jf["PublicServers"]));

      if (jf["Players"].get<int>() < 559)
         txtPlayers->SetForegroundColour("green");
      else
         txtPlayers->SetForegroundColour(wxColour(255, 173, 0));

      if (jf["PublicServers"].get<int>() > 679)
         txtServers->SetForegroundColour("green");
      else
         txtServers->SetForegroundColour(wxColour(255, 173, 0));

      if (!jf["ModVersion"].is_null()) {
         txtModVersion->SetForegroundColour("green");
         txtModVersion->SetLabel(to_string(jf["ModVersion"]));
      } else {
         txtModVersion->SetLabel("NA");
         txtModVersion->SetForegroundColour("red");
      }
   } else {
      txtPlayers->SetLabel("NA");
      txtPlayers->SetForegroundColour("red");
      txtServers->SetLabel("NA");
      txtServers->SetForegroundColour("red");
      txtModVersion->SetLabel("NA");
      txtModVersion->SetForegroundColour("red");
   }
}

/////////// Update Info Function ///////////
void MySettingsFrame::UpdateInfo() {
   ctrlGameDirectory->SetPath(UIData::GamePath);
   ctrlProfileDirectory->SetPath(UIData::ProfilePath);
   ctrlCacheDirectory->SetPath(UIData::CachePath);
   checkConsole->SetValue(UIData::Console);
   choiceController->SetStringSelection(UIData::Build);
}

/////////// Update Game Directory Function ///////////
void MySettingsFrame::UpdateGameDirectory(const std::string& path) {
   ctrlGameDirectory->SetPath(path);
   UIData::GamePath = path;
   MyMainFrame::GameVersionLabel();
}

/////////// Update Profile Directory Function ///////////
void MySettingsFrame::UpdateProfileDirectory(const std::string& path) {
   ctrlProfileDirectory->SetPath(path);
   UIData::ProfilePath = path;
}

/////////// Update Cache Directory Function ///////////
void MySettingsFrame::UpdateCacheDirectory(const std::string& path) {
   ctrlCacheDirectory->SetPath(path);
   UIData::CachePath = path;
}

/////////// UpdateConfig Function ///////////
template<typename ValueType>
void UpdateConfig(const std::string& key, ValueType&& value) {
   if (fs::exists("Launcher.toml")) {
      toml::parse_result config = toml::parse_file("Launcher.toml");
      config.insert_or_assign(key, value);

      std::ofstream tml("Launcher.toml");
      if (tml.is_open()) {
         tml << config;
         tml.close();
      } else wxMessageBox("Failed to modify config file", "Error");
   }
}

/////////// Auto Detect Game Function ///////////
std::string AutoDetectGame() {
   HKEY BeamNG;
   std::string GamePath;
   LONG RegRes =
       RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\BeamNG\BeamNG.drive)", 0,
                     KEY_READ, &BeamNG);
   if (RegRes == ERROR_SUCCESS) {
      GamePath = Launcher::QueryValue(BeamNG, "rootpath");
      RegCloseKey(BeamNG);
   }
   if (!GamePath.empty()) {
      if (GamePath.ends_with('\\')) GamePath.pop_back();
      UpdateConfig("GamePath", GamePath);
      return GamePath;
   } else {
      wxMessageBox("Please launch the game at least once, failed to read registry key Software\\BeamNG\\BeamNG.drive", "Error");
      return "";
   }
}

/////////// Auto Detect Profile Function ///////////
std::string AutoDetectProfile() {
   HKEY BeamNG;
   std::string ProfilePath;
   LONG RegRes =
       RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\BeamNG\BeamNG.drive)", 0,
                     KEY_READ, &BeamNG);
   if (RegRes == ERROR_SUCCESS) {
      ProfilePath = Launcher::QueryValue(BeamNG, "userpath_override");
      RegCloseKey(BeamNG);
   }
   if (ProfilePath.empty()) {
      PWSTR folderPath = nullptr;
      HRESULT hr =
          SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folderPath);

      if (!SUCCEEDED(hr)) {
         wxMessageBox(
             "Please launch the game at least once, failed to read registry key Software\\BeamNG\\BeamNG.drive",
             "Error");
         return "";
      } else {
         _bstr_t bstrPath(folderPath);
         std::string Path((char*)bstrPath);
         CoTaskMemFree(folderPath);
         ProfilePath = Path + "\\BeamNG.drive";
      }
   }
   UpdateConfig("ProfilePath", ProfilePath);
   return ProfilePath;
}

/////////// Reset Cache Function ///////////
std::string ResetCache() {
   std::string CachePath = fs::current_path().append("Resources").string();
   UpdateConfig("CachePath", CachePath);
   return CachePath;
}

/////////// Load Config Function ///////////
void LoadConfig() {
   if (fs::exists("Launcher.toml")) {
      toml::parse_result config = toml::parse_file("Launcher.toml");
      auto ui                   = config["UI"];
      auto Build                = config["Build"];
      auto GamePath             = config["GamePath"];
      auto ProfilePath          = config["ProfilePath"];
      auto CachePath            = config["CachePath"];
      auto Console              = config["Console"];

      if (GamePath.is_string()) {
         UIData::GamePath = GamePath.as_string()->get();
      } else wxMessageBox("Game path not found!", "Error");

      if (ProfilePath.is_string()) {
         UIData::ProfilePath = ProfilePath.as_string()->get();
      } else wxMessageBox("Profile path not found!", "Error");

      if (CachePath.is_string()) {
         UIData::CachePath = CachePath.as_string()->get();
      } else wxMessageBox("Cache path not found!", "Error");

      if (Console.is_boolean()) {
         UIData::Console = Console.as_boolean()->get();
      } else wxMessageBox("Unable to retrieve console state!", "Error");

      if (Build.is_string()) {
         UIData::Build = Build.as_string()->get();
      } else wxMessageBox("Unable to retrieve build state!", "Error");

   } else {
      std::ofstream tml("Launcher.toml");
      if (tml.is_open()) {
         tml << "UI = true\n"
                "Build = 'Default'\n"
                "GamePath = ''\n"
                "ProfilePath = ''\n"
                "CachePath = ''\n"
                "Console = false";
         tml.close();
         AutoDetectGame();
         AutoDetectProfile();
         ResetCache();
         LoadConfig();
      } else wxMessageBox("Failed to create config file", "Error");
   }
}

/////////// Login Function ///////////
bool Login(const std::string& fields) {
   if (fields == "LO") {
      UIData::LoginAuth = false;
      UpdateKey("");
      return false;
   }
   std::string Buffer = HTTP::Post("https://auth.beammp.com/userlogin", fields);
   Json d             = Json::parse(Buffer, nullptr, false);

   if (Buffer == "-1") {
      wxMessageBox("Failed to communicate with the auth system!", "Error");
      return false;
   }

   if (Buffer.at(0) != '{' || d.is_discarded()) {
      wxMessageBox(
          "Invalid answer from authentication servers, please try again later!",
          "Error");
      return false;
   }

   if (!d["success"].is_null() && d["success"].get<bool>()) {
      UIData::LoginAuth = true;
      if (!d["private_key"].is_null()) {
         UpdateKey(d["private_key"].get<std::string>());
      }
      if (!d["public_key"].is_null()) {
         UIData::PublicKey = d["public_key"].get<std::string>();
      }
      if (!d["username"].is_null()) {
         UIData::Username = d["username"].get<std::string>();
      }
      if (!d["role"].is_null()) {
         UIData::UserRole = d["role"].get<std::string>();
      }
      return true;
   } else if (!d["message"].is_null()) wxMessageBox(d["message"].get<std::string>(), "Error");
   return false;
}

/////////// Check Key Function ///////////
void CheckKey() {
   if (fs::exists("key") && fs::file_size("key") < 100) {
      std::ifstream Key("key");
      if (Key.is_open()) {
         auto Size = fs::file_size("key");
         std::string Buffer(Size, 0);
         Key.read(&Buffer[0], std::streamsize(Size));
         Key.close();

         Buffer = HTTP::Post("https://auth.beammp.com/userlogin",
                             R"({"pk":")" + Buffer + "\"}");

         Json d = Json::parse(Buffer, nullptr, false);
         if (Buffer == "-1" || Buffer.at(0) != '{' || d.is_discarded()) {
            wxMessageBox("Couldn't connect to auth server, you might be offline!", "Warning", wxICON_WARNING);
            return;
         }
         if (d["success"].get<bool>()) {
            UIData::LoginAuth = true;
            UpdateKey(d["private_key"].get<std::string>());
            UIData::PublicKey = d["public_key"].get<std::string>();
            UIData::UserRole  = d["role"].get<std::string>();
            UIData::Username  = d["username"].get<std::string>();
         } else UpdateKey("");
      } else UpdateKey("");
   } else UpdateKey("");
}

void WindowsConsole(bool isChecked);
void UpdateCheck();
/////////// OnInit Function ///////////
bool MyApp::OnInit() {
   if (!fs::exists("icons")) {
      fs::create_directory("icons");
      std::string picture = HTTP::Get("https://forum.beammp.com/uploads/default/original/3X/c/d/cd7f8f044efe85e5a0f7bc1d296775438e800488.png");
      std::ofstream File("icons/BeamMP_white.png", std::ios::binary);
      if (File.is_open()) {
         File << picture;
         File.close();
      }
      picture = HTTP::Get("https://forum.beammp.com/uploads/default/original/3X/b/9/b95f01469238d3ec92163d1b0be79cdd1662fdcd.png");
      File.open("icons/BeamMP_black.png", std::ios::binary);
      if (File.is_open()) {
         File << picture;
         File.close();
      }
      picture = HTTP::Get("https://forum.beammp.com/uploads/default/original/3X/6/f/6f0da341a1e570dc973d7251ecd8b4bb3c17eb65.png");
      File.open("icons/default.png", std::ios::binary);
      if (File.is_open()) {
         File << picture;
         File.close();
      }
   }

   Log::Init();
   LoadConfig();
   CheckKey();

   WindowsConsole(UIData::Console);
   Log::ConsoleOutput(UIData::Console);

   auto* MainFrame                = new MyMainFrame();
   MyMainFrame::MainFrameInstance = MainFrame;
   MyMainFrame::UpdateThread      = std::thread(UpdateCheck);

   MainFrame->SetIcon(wxIcon("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG));

   // Set MainFrame properties:
   MainFrame->SetSize(1000, 650);
   MainFrame->Center();

   if (wxSystemSettings::GetAppearance().IsDark()) {
      MainFrame->SetBackgroundColour(wxColour(40, 40, 40));
      MainFrame->SetForegroundColour(wxColour(255, 255, 255));
   } else {
      MainFrame->SetBackgroundColour(wxColour("white"));
      MainFrame->SetForegroundColour(wxColour("white"));
   }
   wxFileSystem::AddHandler(new wxInternetFSHandler);
   MainFrame->Show(true);
   return true;
}

/////////// Windows Console Function ///////////
void WindowsConsole(bool isChecked) {
   if (isChecked) {
      AllocConsole();
      FILE* pNewStdout = nullptr;
      FILE* pNewStderr = nullptr;
      FILE* pNewStdin  = nullptr;

      ::freopen_s(&pNewStdout, "CONOUT$", "w", stdout);
      ::freopen_s(&pNewStderr, "CONOUT$", "w", stderr);
      ::freopen_s(&pNewStdin, "CONIN$", "r", stdin);
   } else {
      FreeConsole();
      ::fclose(stdout);
      ::fclose(stderr);
      ::fclose(stdin);
   }
   // Clear the error state for all the C++ standard streams. Attempting to accessing the streams before they refer
   // to a valid target causes the stream to enter an error state. Clearing the error state will fix this problem,
   // which seems to occur in newer version of Visual Studio even when the console has not been read from or written
   // to yet.
   std::cout.clear();
   std::cerr.clear();
   std::cin.clear();
   std::wcout.clear();
   std::wcerr.clear();
   std::wcin.clear();
}

/////////// Read json Function ///////////
std::string jsonRead() {
   fs::path path = fs::path(UIData::GamePath).append("integrity.json");
   if (fs::exists(path)) {
      std::ifstream ifs(path);
      nlohmann::json jf = nlohmann::json::parse(ifs, nullptr, false);
      if (!jf.is_discarded() && !jf.empty()) return jf["version"];
   } else wxMessageBox("Couldn't read game version, check game path in settings", "Error");
   return "";
}

/////////// Picture Type Function ///////////
// JPG 0 / PNG 1
std::string PictureType(const std::string& picture) {
   for (int i = 0; i < 15; i++) {
      if (picture[i] == 'J')
         return ".jpg";
      else if (picture[i] == 'P' && picture[i + 1] == 'N')
         return ".png";
   }
   return "";
}

/////////// Get Picture Name Function ///////////
std::string GetPictureName() {
   for (const auto& entry : fs::recursive_directory_iterator("icons")) {
      if (entry.path().filename().string().find(UIData::Username) != std::string::npos) {
         return entry.path().string();
      }
   }
   return "";
}

/////////// Progress Bar Function ///////////
bool ProgressBar(size_t c, size_t t) {
   int Percent = int(round(double(c) / double(t) * 100));
   MyMainFrame::MainFrameInstance->UpdateBar->SetValue(Percent);
   MyMainFrame::MainFrameInstance->txtUpdate->SetLabel("Downloading " + std::to_string(Percent) + "%");
   return true;
}

/////////// Admin Relaunch Functions ///////////
void AdminRelaunch(const std::string& executable) {
   system("cls");
   ShellExecuteA(nullptr, "runas", executable.c_str(), nullptr,
                 nullptr, SW_SHOWNORMAL);
   ShowWindow(GetConsoleWindow(), 0);
   throw ShutdownException("Launcher Relaunching");
}

/////////// Relaunch Functions ///////////
void Relaunch(const std::string& executable) {
   ShellExecuteA(nullptr, "open", executable.c_str(), nullptr,
                 nullptr, SW_SHOWNORMAL);
   ShowWindow(GetConsoleWindow(), 0);
   std::this_thread::sleep_for(std::chrono::seconds(1));
   throw ShutdownException("Launcher Relaunching");
}

/////////// Update Check Function ///////////
void UpdateCheck() {
   MyMainFrame::MainFrameInstance->btnLaunch->Disable();
   std::string link;
   std::string HTTP = HTTP::Get("https://beammp.com/builds/launcher?version=true");
   bool fallback    = false;
   if (HTTP.find_first_of("0123456789") == std::string::npos) {
      HTTP     = HTTP::Get("https://backup1.beammp.com/builds/launcher?version=true");
      fallback = true;
      if (HTTP.find_first_of("0123456789") == std::string::npos)
         wxMessageBox("Primary Servers Offline! sorry for the inconvenience!", "Error");
   }
   if (fallback) {
      link = "https://backup1.beammp.com/builds/launcher?download=true";
   } else link = "https://beammp.com/builds/launcher?download=true";
   const auto CurrentPath = fs::current_path();
   std::string EP((CurrentPath / "BeamMP-Launcher.exe").string()), Tmp(EP + ".tmp");
   std::string Back((CurrentPath / "BeamMP-Launcher.back").string());

   if (fs::exists(Back)) remove(Back.c_str());

   std::string RemoteVer;
   for (char& c : HTTP) {
      if (std::isdigit(c) || c == '.') {
         RemoteVer += c;
      }
   }

   if (VersionParser(RemoteVer) > VersionParser(Launcher::FullVersion)) {
      system("cls");
      MyMainFrame::MainFrameInstance->txtUpdate->SetLabel("Downloading...");

      if (!HTTP::Download(link, Tmp, ProgressBar)) {
         wxMessageBox("Launcher Update failed! trying again...", "Error");
         std::this_thread::sleep_for(std::chrono::seconds(2));

         if (!HTTP::Download(link, Tmp, ProgressBar)) {
            wxMessageBox("Launcher Update failed!", "Error");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            AdminRelaunch(EP);
         }
      }

      MyMainFrame::MainFrameInstance->Destroy();

      if (std::rename(EP.c_str(), Back.c_str()) || std::rename(Tmp.c_str(), EP.c_str())) {
         LOG(ERROR) << "Failed to create a backup!";
      }

      Relaunch(EP);
   } else MyMainFrame::MainFrameInstance->btnLaunch->Enable();
}

/////////// Main Frame Content ///////////
MyMainFrame::MyMainFrame() :
    wxFrame(nullptr, wxID_ANY, "BeamMP Launcher V3", wxDefaultPosition, wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   auto* panel = new wxPanel(this, wxID_ANY, wxPoint(), wxSize(1000, 650));

   // News:
   wxWebView::New()->Create(panel, wxID_ANY, "https://beammp.com", wxPoint(10, 70), wxSize(950, 400));
   auto* txtNews = new wxStaticText(panel, wxID_ANY, wxT("News"), wxPoint(10, 40));
   MyMainFrame::SetFocus();

   auto* HorizontalLine1 = new wxStaticLine(panel, wxID_ANY, wxPoint(10, 60), wxSize(950, 1));
   auto* HorizontalLine2 = new wxStaticLine(panel, wxID_ANY, wxPoint(10, 480), wxSize(950, 1));

   // Hyperlinks:
   auto* HyperForum    = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Forum"), wxT("https://forum.beammp.com"), wxPoint(10, 10));
   auto* txtSeparator1 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(55, 10));

   auto* HyperDiscord  = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Discord"), wxT("https://discord.gg/beammp"), wxPoint(70, 10));
   auto* txtSeparator2 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(120, 10));

   auto* HyperGithub   = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("GitHub"), wxT("https://github.com/BeamMP"), wxPoint(130, 10));
   auto* txtSeparator3 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(180, 10));

   auto* HyperWiki     = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Wiki"), wxT("https://wiki.beammp.com"), wxPoint(195, 10));
   auto* txtSeparator4 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(230, 10));

   auto* HyperPatreon = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Patreon"), wxT("https://www.patreon.com/BeamMP"), wxPoint(240, 10));

   // Update:
   txtUpdate = new wxStaticText(panel, wxID_ANY, wxT("BeamMP V" + Launcher::FullVersion), wxPoint(10, 490));

   UpdateBar = new wxGauge(panel, wxID_ANY, 100, wxPoint(10, 520), wxSize(127, -1));
   UpdateBar->SetValue(100);

   // Information:
   auto* txtGameVersionTitle = new wxStaticText(panel, wxID_ANY, wxT("Game Version: "), wxPoint(160, 490));
   txtGameVersion            = new wxStaticText(panel, wxID_ANY, "NA", wxPoint(240, 490));

   auto* txtPlayersTitle = new wxStaticText(panel, wxID_ANY, wxT("Currently Playing:"), wxPoint(300, 490));
   txtPlayers            = new wxStaticText(panel, wxID_ANY, wxT("NA"), wxPoint(400, 490));

   auto* txtPatreon = new wxStaticText(panel, wxID_ANY, wxT("Special thanks to our Patreon Members!"), wxPoint(570, 490));

   auto* txtModVersionTitle = new wxStaticText(panel, wxID_ANY, wxT("Mod Version:"), wxPoint(160, 520));
   txtModVersion            = new wxStaticText(panel, wxID_ANY, wxT("NA"), wxPoint(235, 520));

   auto* txtServersTitle = new wxStaticText(panel, wxID_ANY, wxT("Available Servers:"), wxPoint(300, 520));
   txtServers            = new wxStaticText(panel, wxID_ANY, wxT("NA"), wxPoint(395, 520));

   auto* txtStatus = new wxStaticText(panel, wxID_ANY, wxT("Status: "), wxPoint(880, 520));
   txtStatusResult = new wxStaticText(panel, wxID_ANY, wxT("Online"), wxPoint(920, 520));

   auto* HorizontalLine3 = new wxStaticLine(panel, wxID_ANY, wxPoint(10, 550), wxSize(950, 1));

   wxInitAllImageHandlers();

   // Account:
   BitAccount                = new wxBitmapButton(panel, 1, wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(45, 45, wxIMAGE_QUALITY_HIGH)), wxPoint(20, 560), wxSize(45, 45));
   std::string PictureString = GetPictureName();
   if (UIData::LoginAuth && fs::exists(PictureString))
      BitAccount->SetBitmap(wxBitmapBundle(wxImage(PictureString).Scale(45, 45, wxIMAGE_QUALITY_HIGH)));
   else
      BitAccount->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(45, 45, wxIMAGE_QUALITY_HIGH)));

   // Buttons:
   auto btnSettings = new wxButton(panel, 2, wxT("Settings"), wxPoint(730, 570), wxSize(110, 25));
   btnLaunch        = new wxButton(panel, 3, wxT("Launch"), wxPoint(850, 570), wxSize(110, 25));

   GetStats();

   // UI Colors:
   GameVersionLabel();
   if (DarkMode) {
      // Text Separators:
      txtSeparator1->SetForegroundColour("white");
      txtSeparator2->SetForegroundColour("white");
      txtSeparator3->SetForegroundColour("white");
      txtSeparator4->SetForegroundColour("white");

      // Texts:
      txtNews->SetForegroundColour("white");
      txtUpdate->SetForegroundColour("white");
      txtGameVersionTitle->SetForegroundColour("white");
      txtPlayersTitle->SetForegroundColour("white");
      txtModVersionTitle->SetForegroundColour("white");
      txtServersTitle->SetForegroundColour("white");
      txtPatreon->SetForegroundColour("white");
      txtStatus->SetForegroundColour("white");

      // Line Separators:
      HorizontalLine1->SetForegroundColour("white");
      HorizontalLine2->SetForegroundColour("white");
      HorizontalLine3->SetForegroundColour("white");

      // Logo:
      auto* logo = new wxBitmapButton(panel, 4, wxBitmapBundle(wxImage("icons/BeamMP_white.png", wxBITMAP_TYPE_PNG).Scale(100, 100, wxIMAGE_QUALITY_HIGH)), wxPoint(850, -15), wxSize(100, 100), wxBORDER_NONE);
      logo->SetBackgroundColour(wxColour(40, 40, 40));
   } else {
      // Logo:
      auto* logo = new wxBitmapButton(panel, 4, wxBitmapBundle(wxImage("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG).Scale(100, 100, wxIMAGE_QUALITY_HIGH)), wxPoint(850, -15), wxSize(100, 100), wxBORDER_NONE);
      logo->SetBackgroundColour("white");
   }
   txtStatusResult->SetForegroundColour("green");
}

/////////// Account Frame Content ///////////
MyAccountFrame::MyAccountFrame() :
    wxFrame(nullptr, wxID_ANY, "Account Manager", wxDefaultPosition, wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   MyAccountFrame::SetFocus();
   auto* handler = new wxPNGHandler;
   wxImage::AddHandler(handler);
   wxStaticBitmap* image;
   image                     = new wxStaticBitmap(this, wxID_ANY, wxBitmapBundle(wxImage("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG).Scale(120, 120, wxIMAGE_QUALITY_HIGH)), wxPoint(180, 20), wxSize(120, 120));
   auto* panel               = new wxPanel(this, wxID_ANY, wxPoint(), wxSize(500, 650));
   std::string PictureString = GetPictureName();
   if (UIData::LoginAuth) {
      if (fs::exists(PictureString))
         image->SetBitmap(wxBitmapBundle(wxImage(PictureString).Scale(120, 120, wxIMAGE_QUALITY_HIGH)));
      else
         image->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(120, 120, wxIMAGE_QUALITY_HIGH)));

      auto* txtName  = new wxStaticText(panel, wxID_ANY, wxT("Username: " + UIData::Username), wxPoint(190, 190));
      auto* txtRole  = new wxStaticText(panel, wxID_ANY, wxT("Role: " + UIData::UserRole), wxPoint(190, 230));
      auto btnLogout = new wxButton(panel, 100, wxT("Logout"), wxPoint(185, 550), wxSize(110, 25));

      // UI Colors:
      if (DarkMode) {
         // Text:
         txtName->SetForegroundColour("white");
         txtRole->SetForegroundColour("white");
      }
   } else {
      image->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(120, 120, wxIMAGE_QUALITY_HIGH)));

      auto* txtLogin = new wxStaticText(panel, wxID_ANY, wxT("Login with your BeamMP account."), wxPoint(150, 200));

      ctrlUsername = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxPoint(131, 230), wxSize(220, 25));
      ctrlPassword = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxPoint(131, 300), wxSize(220, 25), wxTE_PASSWORD);
      ctrlUsername->SetHint("Username / Email");
      ctrlPassword->SetHint("Password");

      auto btnRegister = new wxButton(panel, 101, wxT("Register"), wxPoint(250, 375), wxSize(110, 25));
      auto btnLogin    = new wxButton(panel, 102, wxT("Login"), wxPoint(120, 375), wxSize(110, 25));

      // UI Colors:
      if (DarkMode) {
         // Text:
         txtLogin->SetForegroundColour("white");
      }
   }
}

/////////// Settings Frame Content ///////////
MySettingsFrame::MySettingsFrame() :
    wxFrame(nullptr, wxID_ANY, "Settings", wxDefaultPosition, wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   auto* panel = new wxPanel(this, wxID_ANY, wxPoint(), wxSize(500, 650));

   auto* txtGameDirectory = new wxStaticText(panel, wxID_ANY, wxT("Game Directory: "), wxPoint(30, 100));
   ctrlGameDirectory      = new wxDirPickerCtrl(panel, 200, wxEmptyString, wxT("Game Directory"), wxPoint(130, 100), wxSize(300, -1));
   ctrlGameDirectory->SetLabel("GamePath");
   MySettingsFrame::SetFocus();
   auto btnDetectGameDirectory = new wxButton(panel, 203, wxT("Detect"), wxPoint(185, 140), wxSize(90, 25));

   auto* txtProfileDirectory = new wxStaticText(panel, wxID_ANY, wxT("Profile Directory: "), wxPoint(30, 200));
   ctrlProfileDirectory      = new wxDirPickerCtrl(panel, 201, wxEmptyString, wxT("Profile Directory"), wxPoint(130, 200), wxSize(300, -1));
   ctrlProfileDirectory->SetLabel("ProfilePath");
   auto btnDetectProfileDirectory = new wxButton(panel, 204, wxT("Detect"), wxPoint(185, 240), wxSize(90, 25));

   auto* txtCacheDirectory = new wxStaticText(panel, wxID_ANY, wxT("Cache Directory: "), wxPoint(30, 300));
   ctrlCacheDirectory      = new wxDirPickerCtrl(panel, 202, wxEmptyString, wxT("Cache Directory"), wxPoint(130, 300), wxSize(300, -1));
   ctrlCacheDirectory->SetLabel("CachePath");
   auto btnCacheDirectory = new wxButton(panel, 205, wxT("Reset"), wxPoint(185, 340), wxSize(90, 25));

   auto* txtBuild = new wxStaticText(panel, wxID_ANY, wxT("Build: "), wxPoint(30, 400));
   wxArrayString BuildChoices;
   BuildChoices.Add("Default");
   BuildChoices.Add("Release");
   BuildChoices.Add("EA");
   BuildChoices.Add("Dev");
   choiceController = new wxChoice(panel, 206, wxPoint(85, 400), wxSize(120, 20), BuildChoices);
   choiceController->Select(0);

   checkConsole = new wxCheckBox(panel, 207, " Show Console", wxPoint(30, 450));

   // UI Colors:
   if (DarkMode) {
      // Text:
      txtGameDirectory->SetForegroundColour("white");
      txtProfileDirectory->SetForegroundColour("white");
      txtCacheDirectory->SetForegroundColour("white");
      txtBuild->SetForegroundColour("white");
      checkConsole->SetForegroundColour("white");

      // Style:
      ctrlCacheDirectory->SetWindowStyle(wxBORDER_NONE);
      ctrlProfileDirectory->SetWindowStyle(wxBORDER_NONE);
      ctrlGameDirectory->SetWindowStyle(wxBORDER_NONE);
   }
}

/////////// Game Version Label Function ///////////
void MyMainFrame::GameVersionLabel() {
   std::string read = jsonRead();
   if (!read.empty()) {
      txtGameVersion->SetLabel(read);
      UIData::GameVer = read;
      VersionParser CurrentVersion(read);
      if (CurrentVersion > Launcher::SupportedVersion)
         txtGameVersion->SetForegroundColour(wxColour(255, 173, 0));
      else if (CurrentVersion < Launcher::SupportedVersion)
         txtGameVersion->SetForegroundColour("red");
      else txtGameVersion->SetForegroundColour("green");
   } else {
      txtGameVersion->SetLabel(wxT("NA"));
      txtGameVersion->SetForegroundColour("red");
      UIData::GameVer = "";
   }
}

/////////// OnClick Account Event ///////////
void MyMainFrame::OnClickAccount(wxCommandEvent& event WXUNUSED(event)) {
   AccountFrame = new MyAccountFrame();
   AccountFrame->SetSize(500, 650);
   AccountFrame->Center();
   AccountFrame->SetIcon(wxIcon("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG));

   if (wxSystemSettings::GetAppearance().IsDark()) {
      AccountFrame->SetBackgroundColour(wxColour(40, 40, 40));
      AccountFrame->SetForegroundColour(wxColour(255, 255, 255));
   } else {
      AccountFrame->SetBackgroundColour(wxColour("white"));
      AccountFrame->SetForegroundColour(wxColour("white"));
   }
   AccountFrame->Show();
}

/////////// OnClick Settings Event ///////////
void MyMainFrame::OnClickSettings(wxCommandEvent& event WXUNUSED(event)) {
   auto* SettingsFrame = new MySettingsFrame();
   SettingsFrame->SetSize(500, 650);
   SettingsFrame->Center();
   SettingsFrame->SetIcon(wxIcon("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG));

   if (wxSystemSettings::GetAppearance().IsDark()) {
      SettingsFrame->SetBackgroundColour(wxColour(40, 40, 40));
      SettingsFrame->SetForegroundColour(wxColour(255, 255, 255));
   } else {
      SettingsFrame->SetBackgroundColour(wxColour("white"));
      SettingsFrame->SetForegroundColour(wxColour("white"));
   }
   SettingsFrame->UpdateInfo();
   SettingsFrame->Show(true);
}

/////////// OnClick Launch Event ///////////
void MyMainFrame::OnClickLaunch(wxCommandEvent& event WXUNUSED(event)) {
   static bool FirstTime = true;
   if (UIData::GameVer.empty()) {
      wxMessageBox("Game path is invalid please check settings", "Error");
      return;
   }
   if (Launcher::EntryThread.joinable()) Launcher::EntryThread.join();
   Launcher::EntryThread = std::thread([&]() {
      entry();
      std::this_thread::sleep_for(std::chrono::seconds(2));
      txtStatusResult->SetLabelText(wxT("Online"));
      txtStatusResult->SetForegroundColour("green");
      btnLaunch->Enable();
   });
   txtStatusResult->SetLabelText(wxT("In-Game"));
   txtStatusResult->SetForegroundColour("purple");
   btnLaunch->Disable();

   if (FirstTime) {
      wxMessageBox("Please launch BeamNG.drive manually in case of Steam issues.", "Alert");
      FirstTime = false;
   }
}

/////////// OnClick Logo Event ///////////
void MyMainFrame::OnClickLogo(wxCommandEvent& event WXUNUSED(event)) {
   wxLaunchDefaultApplication("https://beammp.com");
}

/////////// OnClick Register Event ///////////
void MyAccountFrame::OnClickRegister(wxCommandEvent& event WXUNUSED(event)) {
   wxLaunchDefaultApplication("https://forum.beammp.com/signup");
}

/////////// OnClick Login Event ///////////
void MyAccountFrame::OnClickLogin(wxCommandEvent& event WXUNUSED(event)) {
   Json json;
   json["password"] = ctrlPassword->GetValue().utf8_string();
   json["username"] = ctrlUsername->GetValue().utf8_string();

   if (Login(json.dump())) {
      std::string picture = HTTP::Get("https://forum.beammp.com/user_avatar/forum.beammp.com/" + UIData::Username + "/240/4411_2.png");
      std::ofstream File("icons/" + UIData::Username + PictureType(picture), std::ios::binary);
      if (File.is_open()) {
         File << picture;
         File.close();
         MyMainFrame::MainFrameInstance->BitAccount->SetBitmap(wxBitmapBundle(wxImage(GetPictureName()).Scale(45, 45, wxIMAGE_QUALITY_HIGH)));
      }
      MyMainFrame::AccountFrame->Destroy();
      MyMainFrame::MainFrameInstance->OnClickAccount(event);
   }
}

/////////// OnClick Logout Event ///////////
void MyAccountFrame::OnClickLogout(wxCommandEvent& event WXUNUSED(event)) {
   Login("LO");
   MyMainFrame::AccountFrame->Destroy();
   MyMainFrame::MainFrameInstance->BitAccount->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(45, 45, wxIMAGE_QUALITY_HIGH)));
   MyMainFrame::MainFrameInstance->OnClickAccount(event);
}

/////////// OnClick Console Event ///////////
void MySettingsFrame::OnClickConsole(wxCommandEvent& event) {
   WindowsConsole(checkConsole->IsChecked());
   Log::ConsoleOutput(checkConsole->IsChecked());
   bool status = event.IsChecked();
   UpdateConfig("Console", status);
   UIData::Console = status;
}

/////////// OnChanged Game Path Event ///////////
void MySettingsFrame::OnChangedGameDir(wxFileDirPickerEvent& event) {
   std::string NewPath = event.GetPath().utf8_string();
   std::string key     = reinterpret_cast<wxDirPickerCtrl*>(event.GetEventObject())->GetLabel();
   UpdateConfig(key, NewPath);
   UpdateGameDirectory(NewPath);
}

/////////// OnChanged Profile Path Event ///////////
void MySettingsFrame::OnChangedProfileDir(wxFileDirPickerEvent& event) {
   std::string NewPath = event.GetPath().utf8_string();
   std::string key     = reinterpret_cast<wxDirPickerCtrl*>(event.GetEventObject())->GetLabel();
   UpdateConfig(key, NewPath);
   UpdateProfileDirectory(NewPath);
}

/////////// OnChanged Cache Path Event ///////////
void MySettingsFrame::OnChangedCacheDir(wxFileDirPickerEvent& event) {
   std::string NewPath = event.GetPath().utf8_string();
   std::string key     = reinterpret_cast<wxDirPickerCtrl*>(event.GetEventObject())->GetLabel();
   UpdateConfig(key, NewPath);
   UpdateCacheDirectory(NewPath);
}

/////////// OnChanged Build Event ///////////
void MySettingsFrame::OnChangedBuild(wxCommandEvent& event) {
   std::string key = reinterpret_cast<wxChoice*>(event.GetEventObject())->GetString(event.GetSelection());
   UpdateConfig("Build", key);
   UIData::Build = key;
}

/////////// AutoDetect Game Function ///////////
void MySettingsFrame::OnAutoDetectGame(wxCommandEvent& event) {
   UpdateGameDirectory(AutoDetectGame());
}

/////////// AutoDetect Profile Function ///////////
void MySettingsFrame::OnAutoDetectProfile(wxCommandEvent& event) {
   UpdateProfileDirectory(AutoDetectProfile());
}

/////////// Reset Cache Function ///////////
void MySettingsFrame::OnResetCache(wxCommandEvent& event) {
   UpdateCacheDirectory(ResetCache());
}

/////////// MAIN FUNCTION ///////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
   wxDisableAsserts();
   wxLog::SetLogLevel(wxLOG_Info);
   int result = 0;
   try {
      new MyApp();
      result = wxEntry(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
      if (Launcher::EntryThread.joinable())
         Launcher::EntryThread.join();
      if (MyMainFrame::UpdateThread.joinable())
         MyMainFrame::UpdateThread.join();
   } catch (const ShutdownException& e) {
      LOG(INFO) << "Launcher shutting down with reason: " << e.what();
   } catch (const std::exception& e) {
      LOG(FATAL) << e.what();
   }
   return result;
}
