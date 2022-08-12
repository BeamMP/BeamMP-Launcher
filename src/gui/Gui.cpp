///
/// Created by Anonymous275 on 12/27/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///
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
#include <nlohmann/json.hpp>
#include "Launcher.h"
#include "Logger.h"
#include <thread>
#endif

/*/////////// TestFrame class ///////////
class MyTestFrame : public wxFrame {
   public:
   MyTestFrame();

   private:
   // Here you put the frame functions:
   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
};*/

/////////// Inherit App class ///////////
class MyApp : public wxApp {
   public:
   bool OnInit() override;
   //static inline MyTestFrame* TestFrame;
};

/////////// MainFrame class ///////////
class MyMainFrame : public wxFrame {
   public:
   MyMainFrame();

   private:
   // Here you put the frame functions:
   wxStaticText* txtStatusResult;
   wxButton* btnLaunch;
   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
   void OnClickAccount(wxCommandEvent& event);
   void OnClickSettings(wxCommandEvent& event);
   void OnClickLaunch(wxCommandEvent& event);
   void OnClickLogo(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};

/////////// AccountFrame class ///////////
class MyAccountFrame : public wxFrame {
   public:
   MyAccountFrame();

   private:
   // Here you put the frame functions:

   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
   void OnClickRegister(wxCommandEvent& event);
   void OnClickLogout(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};

/////////// SettingsFrame class ///////////
class MySettingsFrame : public wxFrame {
   public:
   MySettingsFrame();
   void LoadConfig();

   private:
   // Here you put the frame functions:
   wxDirPickerCtrl* ctrlGameDirectory, *ctrlProfileDirectory, *ctrlCacheDirectory;
   wxCheckBox* checkConsole;
   wxChoice* choiceController;
   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
   void OnClickConsole(wxCommandEvent& event);
   void OnChangedGameDir (wxFileDirPickerEvent& event);
   void OnChangedBuild (wxCommandEvent& event);
   void OnAutoDetectGame(wxCommandEvent& event);
   void OnAutoDetectProfile(wxCommandEvent& event);
   void OnResetCache(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};



enum { ID_Hello = 1 };

/////////// MainFrame Event Table ///////////
wxBEGIN_EVENT_TABLE(MyMainFrame, wxFrame)
   EVT_BUTTON(39, MyMainFrame::OnClickAccount)
   EVT_BUTTON(40, MyMainFrame::OnClickSettings)
   EVT_BUTTON(41, MyMainFrame::OnClickLaunch)
   EVT_BUTTON(42, MyMainFrame::OnClickLogo)
wxEND_EVENT_TABLE()

/////////// AccountFrame Event Table ///////////
wxBEGIN_EVENT_TABLE(MyAccountFrame, wxFrame)
   EVT_BUTTON(43, MyAccountFrame::OnClickRegister)
   EVT_BUTTON(44, MyAccountFrame::OnClickLogout)
wxEND_EVENT_TABLE()

    /////////// SettingsFrame Event Table ///////////
wxBEGIN_EVENT_TABLE(MySettingsFrame, wxFrame)
        EVT_CHECKBOX(45, MySettingsFrame::OnClickConsole)
        EVT_DIRPICKER_CHANGED(46, MySettingsFrame::OnChangedGameDir)
        EVT_CHOICE(47, MySettingsFrame::OnChangedBuild)
        EVT_BUTTON(10, MySettingsFrame::OnAutoDetectGame)
        EVT_BUTTON(11, MySettingsFrame::OnAutoDetectProfile)
        EVT_BUTTON(12, MySettingsFrame::OnResetCache)
wxEND_EVENT_TABLE()

/////////// OnInit function to show frame ///////////
bool MyApp::OnInit() {
   auto* MainFrame = new MyMainFrame();
   MainFrame->SetIcon(wxIcon("icons/BeamMP_black.png",wxBITMAP_TYPE_PNG));

   // Set MainFrame properties:
   MainFrame->SetSize(1000, 650);
   MainFrame->Center();

   if (wxSystemSettings::GetAppearance().IsDark()) {
      MainFrame->SetBackgroundColour(wxColour(40, 40, 40));
      MainFrame->SetForegroundColour(wxColour(255, 255, 255));
   }
   else {
      MainFrame->SetBackgroundColour(wxColour("white"));
      MainFrame->SetForegroundColour(wxColour("white"));
   }
   wxFileSystem::AddHandler(new wxInternetFSHandler);
   MainFrame->Show(true);

   //Test Frame Properties:
   /*TestFrame = new MyTestFrame();

   TestFrame->SetIcon(wxIcon("icons/BeamMP_black.png",wxBITMAP_TYPE_PNG));
   TestFrame->SetSize(1000, 650);
   TestFrame->Center();

   if (wxSystemSettings::GetAppearance().IsDark()) {
      TestFrame->SetBackgroundColour(wxColour(40, 40, 40));
      TestFrame->SetForegroundColour(wxColour(255, 255, 255));
   }

   else {
      TestFrame->SetBackgroundColour(wxColour("white"));
      TestFrame->SetForegroundColour(wxColour("white"));
   }*/

   return true;
}

bool isSignedIn () {
   return false;
}

/////////// Load Config function ///////////
void MySettingsFrame:: LoadConfig() {
   if (fs::exists("Launcher.toml")) {
      toml::parse_result config = toml::parse_file("Launcher.toml");
      auto ui                   = config["UI"];
      auto build                = config["Build"];
      auto GamePath             = config["GamePath"];
      auto ProfilePath          = config["ProfilePath"];
      auto CachePath            = config["CachePath"];

      if (GamePath.is_string()) {
         ctrlGameDirectory->SetPath(GamePath.as_string()->get());
      } else wxMessageBox("Game path not found!", "Error");

      if (ProfilePath.is_string()) {
         ctrlProfileDirectory->SetPath(ProfilePath.as_string()->get());
      } else wxMessageBox("Profile path not found!", "Error");

      if (CachePath.is_string()) {
         ctrlCacheDirectory->SetPath(CachePath.as_string()->get());
      } else wxMessageBox("Cache path not found!", "Error");

   } else {
      std::ofstream tml("Launcher.toml");
      if (tml.is_open()) {
         tml << "UI = true\n"
                "Build = 'Default'\n"
                "GamePath = 'C:\\Program Files'\n"
                "ProfilePath = 'C:\\Program Files'\n"
                "CachePath = 'Resources'\n"
                "Console = false";
         tml.close();
         LoadConfig();
      } else wxMessageBox("Failed to create config file", "Error");
   }
}

/////////// Windows Console Function ///////////
void WindowsConsole (bool isChecked) {
   if (isChecked) {
      AllocConsole();
      FILE* pNewStdout = nullptr;
      FILE* pNewStderr = nullptr;
      FILE* pNewStdin  = nullptr;

      ::freopen_s(&pNewStdout, "CONOUT$", "w", stdout);
      ::freopen_s(&pNewStderr, "CONOUT$", "w", stderr);
      ::freopen_s(&pNewStdin, "CONIN$", "r", stdin);
   }
   else {
      FreeConsole();
      ::fclose(stdout);
      ::fclose(stderr);
      ::fclose(stdin);
   }
   // Clear the error state for all of the C++ standard streams. Attempting to accessing the streams before they refer
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

/////////// TestFrame Function ///////////
/*MyTestFrame::MyTestFrame() :
    wxFrame(nullptr, wxID_ANY, "BeamMP Launcher V3", wxDefaultPosition,wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {

   auto* file = new wxFileDialog (this, wxT("Test"), wxT(""),wxT(""));
   file->SetPosition(wxPoint(250,250));
   file->SetForegroundColour("white");
}*/

/////////// Read json function ///////////
std::string jsonRead () {

   std::ifstream ifs(R"(D:\BeamNG.Drive.v0.25.4.0.14071\Game\integrity.json)");
   nlohmann::json jf = nlohmann::json::parse(ifs);
   if (!jf.empty()) return jf["version"];
   else return "Game Version: NA";
}

/////////// MainFrame Function ///////////
MyMainFrame::MyMainFrame() :
    wxFrame(nullptr, wxID_ANY, "BeamMP Launcher V3", wxDefaultPosition,wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   auto* panel = new wxPanel(this, wxID_ANY, wxPoint(), wxSize(1000,650));

   //News:
   wxWebView::New() ->Create(panel, wxID_ANY, "https://beammp.com", wxPoint(10, 70), wxSize(950, 400));
   auto* txtNews = new wxStaticText(panel, wxID_ANY, wxT("News"), wxPoint(10, 40));
   MyMainFrame::SetFocus();

   auto* HorizontalLine1 = new wxStaticLine(panel, wxID_ANY, wxPoint(10, 60), wxSize(950, 1));
   auto* HorizontalLine2 = new wxStaticLine(panel, wxID_ANY, wxPoint(10, 480), wxSize(950, 1));

   //PNG Handler:
   auto *handler = new wxPNGHandler;
   wxImage::AddHandler(handler);

   //Hyperlinks:
   auto* HyperForum = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Forum"), wxT("https://forum.beammp.com"), wxPoint(10, 10));
   auto* txtSeparator1 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(55, 10));

   auto* HyperDiscord = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Discord"), wxT("https://discord.gg/beammp"), wxPoint(70, 10));
   auto* txtSeparator2 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(120, 10));

   auto* HyperGithub = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("GitHub"), wxT("https://github.com/BeamMP"), wxPoint(130, 10));
   auto* txtSeparator3 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(180, 10));

   auto* HyperWiki = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Wiki"), wxT("https://wiki.beammp.com"), wxPoint(195, 10));
   auto* txtSeparator4 = new wxStaticText(panel, wxID_ANY, wxT("|"), wxPoint(230, 10));

   auto* HyperPatreon = new wxHyperlinkCtrl(panel, wxID_ANY, wxT("Patreon"), wxT("https://www.patreon.com/BeamMP"), wxPoint(240, 10));

   //Update:
   auto* txtUpdate = new wxStaticText(panel, wxID_ANY, wxT("Updating BeamMP "), wxPoint(10, 490));

   auto* UpdateBar = new wxGauge(panel, wxID_ANY, 100, wxPoint(10, 520), wxSize(127, -1));
   UpdateBar->SetValue(0);
   while (UpdateBar->GetValue() <76) {
      txtUpdate->SetLabel(wxT("Updating BeamMP: " + std::to_string(UpdateBar->GetValue()) + "%"));
      UpdateBar->SetValue(UpdateBar->GetValue() + 1);
   }

   //Information:
   auto* txtGameVersionTitle = new wxStaticText(panel, wxID_ANY, wxT("Game Version: "), wxPoint(160, 490));
   auto* txtGameVersion = new wxStaticText(panel, wxID_ANY, jsonRead(), wxPoint(240, 490));
   auto* txtPlayers = new wxStaticText(panel, wxID_ANY, wxT("Currently Playing: NA"), wxPoint(300, 490));
   auto* txtPatreon = new wxStaticText(panel, wxID_ANY, wxT("Special thanks to our Patreon Members!"), wxPoint(570, 490));

   auto* txtModVersion = new wxStaticText(panel, wxID_ANY, wxT("Mod Version: NA"), wxPoint(160, 520));
   auto* txtServers = new wxStaticText(panel, wxID_ANY, wxT("Available Servers: NA"), wxPoint(300, 520));
   auto* txtStatus = new wxStaticText(panel, wxID_ANY, wxT("Status: "), wxPoint(880, 520));
   txtStatusResult = new wxStaticText(panel, wxID_ANY, wxT("Online"), wxPoint(920, 520));

   auto* HorizontalLine3 = new wxStaticLine(panel, wxID_ANY, wxPoint(10, 550), wxSize(950, 1));

   //Account:
   auto* bitmap = new wxBitmapButton(panel, 39, wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(45,45, wxIMAGE_QUALITY_HIGH)), wxPoint(20, 560), wxSize(45,45));

   if (isSignedIn())
   bitmap->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(45,45, wxIMAGE_QUALITY_HIGH)));
   else
   bitmap->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(45,45, wxIMAGE_QUALITY_HIGH)));

   //Buttons:
   auto btnSettings = new wxButton(panel, 40, wxT("Settings"), wxPoint(730,570), wxSize(110, 25));
   btnLaunch = new wxButton(panel, 41, wxT("Launch"), wxPoint(850,570), wxSize(110, 25));

   //UI Colors:
   if (DarkMode) {
      //Text Separators:
      txtSeparator1->SetForegroundColour("white");
      txtSeparator2->SetForegroundColour("white");
      txtSeparator3->SetForegroundColour("white");
      txtSeparator4->SetForegroundColour("white");

      //Texts:
      txtNews->SetForegroundColour("white");
      txtUpdate->SetForegroundColour("white");
      txtGameVersionTitle->SetForegroundColour("white");

      if (jsonRead() > "0.25.4.0")
      txtGameVersion->SetForegroundColour("orange");
      else if (jsonRead() < "0.25.4.0")
         txtGameVersion->SetForegroundColour("red");
      else
         txtGameVersion->SetForegroundColour("green");

      txtPlayers->SetForegroundColour("white");
      txtModVersion->SetForegroundColour("white");
      txtServers->SetForegroundColour("white");
      txtPatreon->SetForegroundColour("white");
      txtStatus->SetForegroundColour("white");

      //Line Separators:
      HorizontalLine1->SetForegroundColour("white");
      HorizontalLine2->SetForegroundColour("white");
      HorizontalLine3->SetForegroundColour("white");

      //Logo:
      auto* logo = new wxBitmapButton(panel, 42, wxBitmapBundle(wxImage("icons/BeamMP_white.png", wxBITMAP_TYPE_PNG).Scale(100,100, wxIMAGE_QUALITY_HIGH)), wxPoint(850, -15), wxSize(100,100), wxBORDER_NONE);
      logo->SetBackgroundColour(wxColour(40, 40, 40));
   }
   else {
      //Logo:
      auto* logo = new wxBitmapButton(panel, 42, wxBitmapBundle(wxImage("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG).Scale(100,100, wxIMAGE_QUALITY_HIGH)), wxPoint(850, -15), wxSize(100,100), wxBORDER_NONE);
      logo->SetBackgroundColour("white");
   }
   txtStatusResult->SetForegroundColour("green");
}

/////////// Account Frame Content ///////////
MyAccountFrame::MyAccountFrame() : wxFrame(nullptr, wxID_ANY, "Account Manager", wxDefaultPosition,wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   MyAccountFrame::SetFocus();
   auto *handler = new wxPNGHandler;
   wxImage::AddHandler(handler);
   wxStaticBitmap *image;
   image = new wxStaticBitmap( this, wxID_ANY, wxBitmapBundle(wxImage("icons/BeamMP_black.png", wxBITMAP_TYPE_PNG).Scale(120,120, wxIMAGE_QUALITY_HIGH)), wxPoint(180,20), wxSize(120, 120));

   if (isSignedIn()) {
      image->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(120,120, wxIMAGE_QUALITY_HIGH)));

      auto* txtName = new wxStaticText(this, wxID_ANY, wxT("Username: BeamMP"), wxPoint(180, 200));
      auto* txtEmail = new wxStaticText(this, wxID_ANY, wxT("Email: beamMP@gmail.com"), wxPoint(180, 250));
      auto btnLogout = new wxButton(this, 44, wxT("Logout"), wxPoint(185,550), wxSize(110, 25));

      //UI Colors:
      if (DarkMode) {
         //Text:
         txtName->SetForegroundColour("white");
         txtEmail->SetForegroundColour("white");
      }
   }
   else {
      auto* panel = new wxPanel(this, wxID_ANY, wxPoint(), wxSize(500,650));
      image->SetBitmap(wxBitmapBundle(wxImage("icons/default.png", wxBITMAP_TYPE_PNG).Scale(120,120, wxIMAGE_QUALITY_HIGH)));

      auto* txtLogin = new wxStaticText(panel, wxID_ANY, wxT("Login with your BeamMP account."), wxPoint(150, 200));

      auto* ctrlUsername = new wxTextCtrl (panel, wxID_ANY, wxT(""), wxPoint(131, 230), wxSize(220,25));
      auto* ctrlPassword = new wxTextCtrl (panel, wxID_ANY, wxT(""), wxPoint(131, 300), wxSize(220,25), wxTE_PASSWORD);
      ctrlUsername->SetHint("Username / Email");
      ctrlPassword->SetHint("Password");

      auto btnLogin = new wxButton(panel, wxID_ANY, wxT("Login"), wxPoint(120,375), wxSize(110, 25));
      auto btnRegister = new wxButton(panel, 43, wxT("Register"), wxPoint(250,375), wxSize(110, 25));

      //UI Colors:
      if (DarkMode) {
         //Text:
         txtLogin->SetForegroundColour("white");
      }
   }
}

/////////// Settings Frame Content ///////////
MySettingsFrame::MySettingsFrame() :
    wxFrame(nullptr, wxID_ANY, "Settings", wxDefaultPosition,wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   auto* panel = new wxPanel(this, wxID_ANY, wxPoint(), wxSize(500,650));

   auto* txtGameDirectory = new wxStaticText(panel, wxID_ANY, wxT("Game Directory: "), wxPoint(30, 100));
   ctrlGameDirectory = new wxDirPickerCtrl (panel, 46, wxEmptyString, wxT("Game Directory"), wxPoint(130, 100), wxSize(300,-1));
   ctrlGameDirectory->SetLabel("GamePath");
   MySettingsFrame::SetFocus();
   auto btnDetectGameDirectory = new wxButton(panel, 10, wxT("Detect"), wxPoint(185,140), wxSize(90, 25));

   auto* txtProfileDirectory = new wxStaticText(panel, wxID_ANY, wxT("Profile Directory: "), wxPoint(30, 200));
   ctrlProfileDirectory = new wxDirPickerCtrl (panel, 46, wxEmptyString, wxT("Profile Directory"), wxPoint(130, 200), wxSize(300,-1));
   ctrlProfileDirectory->SetLabel("ProfilePath");
   auto btnDetectProfileDirectory = new wxButton(panel, 11, wxT("Detect"), wxPoint(185,240), wxSize(90, 25));

   auto* txtCacheDirectory = new wxStaticText(panel, wxID_ANY, wxT("Cache Directory: "), wxPoint(30, 300));
   ctrlCacheDirectory = new wxDirPickerCtrl (panel, 46, wxEmptyString, wxT("Cache Directory"), wxPoint(130, 300), wxSize(300,-1));
   ctrlCacheDirectory->SetLabel("CachePath");
   auto btnCacheDirectory = new wxButton(panel, 12, wxT("Reset"), wxPoint(185,340), wxSize(90, 25));

   auto* txtBuild = new wxStaticText(panel, wxID_ANY, wxT("Build: "), wxPoint(30, 400));
   wxArrayString BuildChoices;
   BuildChoices.Add("Default");
   BuildChoices.Add("Release");
   BuildChoices.Add("EA");
   BuildChoices.Add("Dev");
   choiceController = new wxChoice (panel, 47, wxPoint(85, 400), wxSize(120, 20), BuildChoices);
   choiceController->Select(0);

   checkConsole = new wxCheckBox (panel, 45, " Show Console", wxPoint(30, 450));

   //UI Colors:
   if (DarkMode) {
      //Text:
      txtGameDirectory->SetForegroundColour("white");
      txtProfileDirectory->SetForegroundColour("white");
      txtCacheDirectory->SetForegroundColour("white");
      txtBuild->SetForegroundColour("white");
      checkConsole->SetForegroundColour("white");

      //Style:
      ctrlCacheDirectory->SetWindowStyle(wxBORDER_NONE);
      ctrlProfileDirectory->SetWindowStyle(wxBORDER_NONE);
      ctrlGameDirectory->SetWindowStyle(wxBORDER_NONE);
   }
}

/////////// UpdateConfig function ///////////
template <typename ValueType>
void UpdateConfig (const std::string& key, ValueType&& value) {
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

/////////// OnClick Account Event ///////////
void MyMainFrame::OnClickAccount(wxCommandEvent& event WXUNUSED(event)) {
   auto* AccountFrame = new MyAccountFrame();
   AccountFrame->SetSize(500, 650);
   AccountFrame->Center();
   AccountFrame->SetIcon(wxIcon("icons/BeamMP_black.png",wxBITMAP_TYPE_PNG));

   if (wxSystemSettings::GetAppearance().IsDark()) {
      AccountFrame->SetBackgroundColour(wxColour(40, 40, 40));
      AccountFrame->SetForegroundColour(wxColour(255, 255, 255));
   }
   else {
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
   SettingsFrame->SetIcon(wxIcon("icons/BeamMP_black.png",wxBITMAP_TYPE_PNG));

   if (wxSystemSettings::GetAppearance().IsDark()) {
      SettingsFrame->SetBackgroundColour(wxColour(40, 40, 40));
      SettingsFrame->SetForegroundColour(wxColour(255, 255, 255));
   }
   else {
      SettingsFrame->SetBackgroundColour(wxColour("white"));
      SettingsFrame->SetForegroundColour(wxColour("white"));
   }
   SettingsFrame->Show(true);
   SettingsFrame->LoadConfig();
}

/////////// OnClick Logo Event ///////////
void MyMainFrame::OnClickLogo(wxCommandEvent& event WXUNUSED(event)) {
   wxLaunchDefaultApplication("https://beammp.com");
}

/////////// OnClick Register Event ///////////
void MyAccountFrame::OnClickRegister(wxCommandEvent& event WXUNUSED(event)) {
   wxLaunchDefaultApplication("https://forum.beammp.com/signup");
}

/////////// OnClick Logout Event ///////////
void MyAccountFrame::OnClickLogout(wxCommandEvent& event WXUNUSED(event)) {



}

/////////// OnClick Launch Event ///////////
void MyMainFrame::OnClickLaunch(wxCommandEvent& event WXUNUSED(event)) {
   static bool FirstTime = true;
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

   if(FirstTime) {
      wxMessageBox("Please launch BeamNG.drive manually in case of Steam issues.", "Alert");
      FirstTime = false;
   }
}

/////////// OnClick Console Event ///////////
void MySettingsFrame::OnClickConsole(wxCommandEvent& event) {
      WindowsConsole(checkConsole->IsChecked());
      bool status = event.IsChecked();
      UpdateConfig("Console", status);
}

/////////// OnChanged Game Path Event ///////////
void MySettingsFrame::OnChangedGameDir(wxFileDirPickerEvent& event) {
   std::string NewPath = event.GetPath().utf8_string();
   std::string key = reinterpret_cast<wxDirPickerCtrl*> (event.GetEventObject())->GetLabel();
   UpdateConfig(key, NewPath);
}

/////////// OnChanged Build Event ///////////
void MySettingsFrame::OnChangedBuild(wxCommandEvent& event) {
   std::string key = reinterpret_cast<wxChoice*> (event.GetEventObject())->GetString(event.GetSelection());
   UpdateConfig("Build", key);
}

/////////// AutoDetect Game Function ///////////
void MySettingsFrame::OnAutoDetectGame (wxCommandEvent& event) {
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
      ctrlGameDirectory->SetPath(GamePath);
   }
   else
   wxMessageBox("Please launch the game at least once, failed to read registry key Software\\BeamNG\\BeamNG.drive", "Error");
}

/////////// AutoDetect Profile Function ///////////
void MySettingsFrame::OnAutoDetectProfile(wxCommandEvent& event){
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
         return;
      }
      else {
         _bstr_t bstrPath(folderPath);
         std::string Path((char*)bstrPath);
         CoTaskMemFree(folderPath);
         ProfilePath = Path + "\\BeamNG.drive";
      }
   }
   UpdateConfig("ProfilePath", ProfilePath);
   ctrlProfileDirectory->SetPath(ProfilePath);
}

/////////// Reset Cache Function ///////////
void MySettingsFrame::OnResetCache(wxCommandEvent& event) {
   std::string CachePath = fs::current_path().append("Resources").string();
   UpdateConfig("CachePath", CachePath);
   ctrlCacheDirectory->SetPath(CachePath);
}

/////////// MAIN FUNCTION ///////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
   wxDisableAsserts();
   wxLog::SetLogLevel(wxLOG_Info);
   new MyApp();
   int result = wxEntry(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
   if (Launcher::EntryThread.joinable())
      Launcher::EntryThread.join();
   return result;
}
