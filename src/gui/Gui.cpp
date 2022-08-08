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
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/webview.h>
#include <wx/wx.h>
#include <wx/wxhtml.h.>

#include "Launcher.h"
#include "Logger.h"
#endif

/////////// Inherit App class ///////////
class MyApp : public wxApp {
   public:
   bool OnInit() override;
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

   private:
   // Here you put the frame functions:
   wxCheckBox* checkConsole;
   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
   void OnClickConsole(wxCommandEvent& event);
   wxDECLARE_EVENT_TABLE();
};

/////////// TestFrame class ///////////
class MyTestFrame : public wxFrame {
   public:
   MyTestFrame();

   private:
   // Here you put the frame functions:
   bool DarkMode = wxSystemSettings::GetAppearance().IsDark();
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
wxEND_EVENT_TABLE()

/////////// OnInit function to show frame ///////////
bool MyApp::OnInit() {
   auto* MainFrame = new MyMainFrame();
   MainFrame->SetIcon(wxIcon("beammp_original.png",wxBITMAP_TYPE_PNG));

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
/*   auto* TestFrame = new MyTestFrame();
   TestFrame->SetIcon(wxIcon("beammp_original.png",wxBITMAP_TYPE_PNG));
   TestFrame->SetSize(1000, 650);
   TestFrame->Center();

   if (wxSystemSettings::GetAppearance().IsDark()) {
      TestFrame->SetBackgroundColour(wxColour(40, 40, 40));
      TestFrame->SetForegroundColour(wxColour(255, 255, 255));
   }

   else {
      TestFrame->SetBackgroundColour(wxColour("white"));
      TestFrame->SetForegroundColour(wxColour("white"));
   }
   TestFrame->Show(true);*/

   return true;
}

bool isSignedIn () {
   return false;
}

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

/////////// MainFrame Function ///////////
MyMainFrame::MyMainFrame() :
    wxFrame(nullptr, wxID_ANY, "BeamMP Launcher V3", wxDefaultPosition,wxDefaultSize,
            wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX) {
   //News:
   wxWebView::New() ->Create(this, wxID_ANY, "https://beammp.com", wxPoint(10, 70), wxSize(950, 400));
   auto* txtNews = new wxStaticText(this, wxID_ANY, wxT("News"), wxPoint(10, 40));
   MyMainFrame::SetFocus();

   auto* HorizontalLine1 = new wxStaticLine(this, wxID_ANY, wxPoint(10, 60), wxSize(950, 1));
   auto* HorizontalLine2 = new wxStaticLine(this, wxID_ANY, wxPoint(10, 480), wxSize(950, 1));

   //PNG Handler:
   auto *handler = new wxPNGHandler;
   wxImage::AddHandler(handler);

   //Hyperlinks:
   auto* HyperForum = new wxHyperlinkCtrl(this, wxID_ANY, wxT("Forum"), wxT("https://forum.beammp.com"), wxPoint(10, 10));
   auto* txtSeparator1 = new wxStaticText(this, wxID_ANY, wxT("|"), wxPoint(55, 10));

   auto* HyperDiscord = new wxHyperlinkCtrl(this, wxID_ANY, wxT("Discord"), wxT("https://discord.gg/beammp"), wxPoint(70, 10));
   auto* txtSeparator2 = new wxStaticText(this, wxID_ANY, wxT("|"), wxPoint(120, 10));

   auto* HyperGithub = new wxHyperlinkCtrl(this, wxID_ANY, wxT("GitHub"), wxT("https://github.com/BeamMP"), wxPoint(130, 10));
   auto* txtSeparator3 = new wxStaticText(this, wxID_ANY, wxT("|"), wxPoint(180, 10));

   auto* HyperWiki = new wxHyperlinkCtrl(this, wxID_ANY, wxT("Wiki"), wxT("https://wiki.beammp.com"), wxPoint(195, 10));
   auto* txtSeparator4 = new wxStaticText(this, wxID_ANY, wxT("|"), wxPoint(230, 10));

   auto* HyperPatreon = new wxHyperlinkCtrl(this, wxID_ANY, wxT("Patreon"), wxT("https://www.patreon.com/BeamMP"), wxPoint(240, 10));

   //Update:
   auto* txtUpdate = new wxStaticText(this, wxID_ANY, wxT("Updating BeamMP "), wxPoint(10, 490));

   auto* UpdateBar = new wxGauge(this, wxID_ANY, 100, wxPoint(10, 520), wxSize(127, -1));
   UpdateBar->SetValue(0);
   while (UpdateBar->GetValue() <76) {
      txtUpdate->SetLabel(wxT("Updating BeamMP: " + std::to_string(UpdateBar->GetValue()) + "%"));
      UpdateBar->SetValue(UpdateBar->GetValue() + 1);
   }

   //Information:
   auto* txtGameVersion = new wxStaticText(this, wxID_ANY, wxT("Game Version: NA"), wxPoint(160, 490));
   auto* txtPlayers = new wxStaticText(this, wxID_ANY, wxT("Currently Playing: NA"), wxPoint(300, 490));
   auto* txtPatreon = new wxStaticText(this, wxID_ANY, wxT("Special thanks to our Patreon Members!"), wxPoint(570, 490));

   auto* txtModVersion = new wxStaticText(this, wxID_ANY, wxT("Mod Version: NA"), wxPoint(160, 520));
   auto* txtServers = new wxStaticText(this, wxID_ANY, wxT("Available Servers: NA"), wxPoint(300, 520));
   auto* txtStatus = new wxStaticText(this, wxID_ANY, wxT("Status: "), wxPoint(880, 520));
   txtStatusResult = new wxStaticText(this, wxID_ANY, wxT("Online"), wxPoint(920, 520));

   auto* HorizontalLine3 = new wxStaticLine(this, wxID_ANY, wxPoint(10, 550), wxSize(950, 1));

   //Account:
   auto* bitmap = new wxBitmapButton(this, 39, wxBitmapBundle(wxImage("deer.png", wxBITMAP_TYPE_PNG).Scale(45,45, wxIMAGE_QUALITY_HIGH)), wxPoint(20, 560), wxSize(45,45));

   if (isSignedIn())
   bitmap->SetBitmap(wxBitmapBundle(wxImage("deer.png", wxBITMAP_TYPE_PNG).Scale(45,45, wxIMAGE_QUALITY_HIGH)));
   else
   bitmap->SetBitmap(wxBitmapBundle(wxImage("default.png", wxBITMAP_TYPE_PNG).Scale(45,45, wxIMAGE_QUALITY_HIGH)));

   //Buttons:
   auto btnSettings = new wxButton(this, 40, wxT("Settings"), wxPoint(730,570), wxSize(110, 25));
   btnLaunch = new wxButton(this, 41, wxT("Launch"), wxPoint(850,570), wxSize(110, 25));

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
      txtGameVersion->SetForegroundColour("white");
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
      auto* logo = new wxBitmapButton(this, 42, wxBitmapBundle(wxImage("beammp_original.png", wxBITMAP_TYPE_PNG).Scale(100,100, wxIMAGE_QUALITY_HIGH)), wxPoint(850, -15), wxSize(100,100), wxBORDER_NONE);
      logo->SetBackgroundColour(wxColour(40, 40, 40));
   }
   else {
      //Logo:
      auto* logo = new wxBitmapButton(this, 42, wxBitmapBundle(wxImage("BeamMP.png", wxBITMAP_TYPE_PNG).Scale(100,100, wxIMAGE_QUALITY_HIGH)), wxPoint(850, -15), wxSize(100,100), wxBORDER_NONE);
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
   image = new wxStaticBitmap( this, wxID_ANY, wxBitmapBundle(wxImage("default.png", wxBITMAP_TYPE_PNG).Scale(120,120, wxIMAGE_QUALITY_HIGH)), wxPoint(180,20), wxSize(120, 120));

   if (isSignedIn()) {
      image->SetBitmap(wxBitmapBundle(wxImage("deer.png", wxBITMAP_TYPE_PNG).Scale(120,120, wxIMAGE_QUALITY_HIGH)));

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
      image->SetBitmap(wxBitmapBundle(wxImage("default.png", wxBITMAP_TYPE_PNG).Scale(120,120, wxIMAGE_QUALITY_HIGH)));

      auto* txtLogin = new wxStaticText(this, wxID_ANY, wxT("Login with your BeamMP account."), wxPoint(150, 200));

      auto* ctrlUsername = new wxTextCtrl (this, wxID_ANY, wxT(""), wxPoint(131, 230), wxSize(220,25));
      auto* ctrlPassword = new wxTextCtrl (this, wxID_ANY, wxT(""), wxPoint(131, 300), wxSize(220,25), wxTE_PASSWORD);
      ctrlUsername->SetHint("Username / Email");
      ctrlPassword->SetHint("Password");

      auto btnLogin = new wxButton(this, wxID_ANY, wxT("Login"), wxPoint(120,375), wxSize(110, 25));
      auto btnRegister = new wxButton(this, 43, wxT("Register"), wxPoint(250,375), wxSize(110, 25));

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

   auto* txtDirectory = new wxStaticText(this, wxID_ANY, wxT("Game Directory: "), wxPoint(30, 100));
   auto* ctrlDirectory = new wxTextCtrl (this, wxID_ANY, wxT("D:/PATH/BeamNG.exe"), wxPoint(130, 100), wxSize(220,-1));
   MySettingsFrame::SetFocus();
   auto btnGame = new wxButton(this, 40, wxT("Change Directory"), wxPoint(185,140), wxSize(110, 25));

   //auto* bitmapGame = new wxBitmapButton(this, wxID_ANY, wxBitmapBundle(wxImage("explorer.png", wxBITMAP_TYPE_PNG).Scale(15,15, wxIMAGE_QUALITY_HIGH)), wxPoint(350, 100), wxSize(15,15), wxBORDER_NONE);

   auto* txtProfileDirectory = new wxStaticText(this, wxID_ANY, wxT("Profile Directory: "), wxPoint(30, 200));
   auto* ctrlProfileDirectory = new wxTextCtrl (this, wxID_ANY, wxT("D:/PATH/BeamNG.exe"), wxPoint(130, 200), wxSize(220,-1));
   auto btnProfile = new wxButton(this, 40, wxT("Change Directory"), wxPoint(185,240), wxSize(110, 25));
   //auto* bitmapProfile = new wxBitmapButton(this, wxID_ANY, wxBitmapBundle(wxImage("explorer.png", wxBITMAP_TYPE_PNG).Scale(20,20, wxIMAGE_QUALITY_HIGH)), wxPoint(350, 175), wxSize(25,25));

   auto* txtCacheDirectory = new wxStaticText(this, wxID_ANY, wxT("Cache Directory: "), wxPoint(30, 300));
   auto* ctrlCacheDirectory = new wxTextCtrl (this, wxID_ANY, wxT("D:/PATH/Cache"), wxPoint(130, 300), wxSize(220,-1));
   auto btnCache = new wxButton(this, 40, wxT("Change Directory"), wxPoint(185,340), wxSize(110, 25));
   //auto* bitmapCache = new wxBitmapButton(this, wxID_ANY, wxBitmapBundle(wxImage("explorer.png", wxBITMAP_TYPE_PNG).Scale(20,20, wxIMAGE_QUALITY_HIGH)), wxPoint(350, 250), wxSize(20,20), wxBORDER_NONE);

   auto* txtBranch = new wxStaticText(this, wxID_ANY, wxT("Branch: "), wxPoint(30, 400));
   wxArrayString BranchChoices;
   BranchChoices.Add("Public");
   BranchChoices.Add("Development");
   BranchChoices.Add("Early Access");
   auto* choiceController = new wxChoice (this, wxID_ANY, wxPoint(85, 400), wxSize(120, 20), BranchChoices);
   choiceController->Select(0);

   checkConsole = new wxCheckBox (this, 45, " Show Console", wxPoint(30, 450));

   //UI Colors:
   if (DarkMode) {
      //Text:
      txtDirectory->SetForegroundColour("white");
      txtProfileDirectory->SetForegroundColour("white");
      txtCacheDirectory->SetForegroundColour("white");
      txtBranch->SetForegroundColour("white");
      checkConsole->SetForegroundColour("white");

      //Style:
      ctrlCacheDirectory->SetWindowStyle(wxBORDER_NONE);
      ctrlProfileDirectory->SetWindowStyle(wxBORDER_NONE);
      ctrlDirectory->SetWindowStyle(wxBORDER_NONE);
   }
}

/////////// OnClick Account Event ///////////
void MyMainFrame::OnClickAccount(wxCommandEvent& event WXUNUSED(event)) {
   auto* AccountFrame = new MyAccountFrame();
   AccountFrame->SetSize(500, 650);
   AccountFrame->Center();
   AccountFrame->SetIcon(wxIcon("beammp_original.png",wxBITMAP_TYPE_PNG));

   if (wxSystemSettings::GetAppearance().IsDark()) {
      AccountFrame->SetBackgroundColour(wxColour(40, 40, 40));
      AccountFrame->SetForegroundColour(wxColour(255, 255, 255));
   }
   else {
      AccountFrame->SetBackgroundColour(wxColour("white"));
      AccountFrame->SetForegroundColour(wxColour("white"));
   }
   AccountFrame->Show(true);
}

/////////// OnClick Settings Event ///////////
void MyMainFrame::OnClickSettings(wxCommandEvent& event WXUNUSED(event)) {
   auto* SettingsFrame = new MySettingsFrame();
   SettingsFrame->SetSize(500, 650);
   SettingsFrame->Center();
   SettingsFrame->SetIcon(wxIcon("beammp_original.png",wxBITMAP_TYPE_PNG));

   if (wxSystemSettings::GetAppearance().IsDark()) {
      SettingsFrame->SetBackgroundColour(wxColour(40, 40, 40));
      SettingsFrame->SetForegroundColour(wxColour(255, 255, 255));
   }
   else {
      SettingsFrame->SetBackgroundColour(wxColour("white"));
      SettingsFrame->SetForegroundColour(wxColour("white"));
   }
   SettingsFrame->Show(true);
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
   if (Launcher::EntryThread.joinable()) Launcher::EntryThread.join();
   Launcher::EntryThread = std::thread([&]() {
      entry();
      txtStatusResult->SetLabelText(wxT("Online"));
      txtStatusResult->SetForegroundColour("green");
      btnLaunch->Enable();
   });
   txtStatusResult->SetLabelText(wxT("In-Game"));
   txtStatusResult->SetForegroundColour("purple");
   btnLaunch->Disable();
}

/////////// OnClick Console Event ///////////
void MySettingsFrame::OnClickConsole(wxCommandEvent& event WXUNUSED(event)) {
      WindowsConsole(checkConsole->IsChecked());
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
