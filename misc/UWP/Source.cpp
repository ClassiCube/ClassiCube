using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::Graphics::Display;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Platform;

void Launch_Browser(void) {
    wchar_t* tmp = L"https://google.com";
    auto str = ref new String(tmp);
    auto uri = ref new Uri(str);

    auto options = ref new Windows::System::LauncherOptions();
    options->TreatAsUntrusted = true;
    Windows::System::Launcher::LaunchUriAsync(uri, options);
}

void OpenFileDialog(void) {
    auto picker = ref new Windows::Storage::Pickers::FileOpenPicker();
    picker->FileTypeFilter->Append(ref new String(L".jpg"));
    picker->FileTypeFilter->Append(ref new String(L".jpeg"));
    picker->FileTypeFilter->Append(ref new String(L".png"));

    //Windows::Storage::StorageFile file = picker->PickSingleFileAsync();
}

ref class CCApp sealed : IFrameworkView
{
public:
    virtual void Initialize(CoreApplicationView^ view)
    {
    }

    virtual void Load(String^ EntryPoint)
    {
    }

    virtual void Uninitialize()
    {
    }

    virtual void Run()
    {
        CoreWindow^ window = CoreWindow::GetForCurrentThread();
        window->Activate();
        Launch_Browser();

        CoreDispatcher^ dispatcher = window->Dispatcher;
        dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
    }

    virtual void SetWindow(CoreWindow^ win)
    {
    }
};

ref class CCAppSource sealed : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView()
    {
        return ref new CCApp();
    }
};

[MTAThread]
int main(Array<String^>^ args)
{
    //init_apartment();
    auto source = ref new CCAppSource();
    CoreApplication::Run(source);
}