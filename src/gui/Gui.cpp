///
/// Created by Anonymous275 on 12/27/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define _CRT_SECURE_NO_WARNINGS
#include <set>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/animate.h>
#include <wx/mstream.h>
#include "gifs.h"

#endif
class MyApp : public wxApp {
public:
    virtual bool OnInit();
};
class MyFrame : public wxFrame {
public:
    MyFrame();
private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    static wxSize FixedSize;
};
enum {
    ID_Hello = 1
};
wxSize MyFrame::FixedSize(370,400);

bool MyApp::OnInit() {
    auto *frame = new MyFrame();
    frame->Show(true);
    return true;
}

MyFrame::MyFrame()
        : wxFrame(nullptr, wxID_ANY, "BeamMP V3.0", wxDefaultPosition, FixedSize) {
    //SetMaxSize(FixedSize);
    //SetMinSize(FixedSize);
    Center();

    //27 35 35

    wxColour Colour(27,35,35,1);
    SetBackgroundColour(Colour);

    auto* menuFile = new wxMenu;
    menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
                     "Help string shown in status bar for this menu item");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    auto* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    auto* menuBar = new wxMenuBar;

    menuBar->SetOwnBackgroundColour(Colour);
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    //SetMenuBar(menuBar);

    auto* m_ani = new wxAnimationCtrl(this, wxID_ANY);
    wxMemoryInputStream stream(gif::Logo, sizeof(gif::Logo));
    if (m_ani->Load(stream))  m_ani->Play();



    Bind(wxEVT_MENU, &MyFrame::OnHello, this, ID_Hello);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
}
void MyFrame::OnExit(wxCommandEvent& event) {
    Close(true);
}
void MyFrame::OnAbout(wxCommandEvent& event) {
    wxMessageBox("This is a wxWidgets Hello World example",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}
void MyFrame::OnHello(wxCommandEvent& event) {
    wxLogMessage("Hello world from wxWidgets!");
}


int GUIEntry (int argc, char *argv[]) {
    new MyApp();
    return wxEntry(argc, argv);
}