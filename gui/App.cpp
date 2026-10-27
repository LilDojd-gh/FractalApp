// App.cpp — точка входа приложения.
#include <wx/wx.h>

#include "MainWindow.h"

class App : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;
        SetAppName("FractalSurf");
        wxInitAllImageHandlers();  // PNG/BMP для сохранения изображения
        (new MainWindow)->Show();
        return true;
    }
};

wxIMPLEMENT_APP(App);
