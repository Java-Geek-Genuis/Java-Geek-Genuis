#include "pch.h"
#include "MainPage.xaml.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <wrl.h>
#include <unknwn.h>

struct __declspec(uuid("905A0FEF-BC53-11DF-8C49-001E4FC686DA")) ICommodoreBufferByteAccess : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Buffer(byte** value) = 0;
};

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace concurrency;

namespace CommodoreWarpUWP {

static std::string ToStd(String^ s)
{
    if (!s) return std::string();
    return std::string(s->Data(), s->Data() + s->Length());
}

static String^ ToPlatformString(const std::string& s)
{
    std::wstring w(s.begin(), s.end());
    return ref new String(w.c_str());
}

static std::string BaseName(String^ name)
{
    std::string s = ToStd(name);
    size_t dot = s.find_last_of('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    std::string r;
    for (char c : s)
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') r.push_back(c);
    return r.empty() ? "MYPROGRAM" : r;
}

static std::string Upper(const std::string& input)
{
    std::string out = input;
    for (char& c : out)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

struct BuiltInProgram
{
    const char* name;
    const char* source;
};

static const BuiltInProgram kBuiltIns[] =
{
    { "HELLO", R"BASIC(10 PRINT CHR$(147)
20 PRINT "COMMODORE WARP BASIC TEST"
30 PRINT "TYPE WITH THE PHONE KEYBOARD!"
40 PRINT
50 PRINT "1. RUN A BUILT-IN TEST"
60 PRINT "2. EDIT ME IN PROGRAM LAB"
70 PRINT
80 FOR I=1 TO 5
90 PRINT "WARP SPEED:";I;"X DEMO"
100 NEXT I
110 PRINT
120 PRINT "PRESS A KEY TO SEE INPUT."
130 GET A$
140 IF A$="" THEN GOTO 130
150 PRINT
160 PRINT "YOU PRESSED [";A$;"]"
170 END)BASIC" },

    { "COLORTEST", R"BASIC(10 PRINT CHR$(147)
20 PRINT "16-COLOR DISPLAY TEST"
30 PRINT "WATCH THE BORDER AND BACKGROUND"
40 FOR I=0 TO 15
50 POKE 53280,I
60 POKE 53281,15-I
70 PRINT CHR$(19);"COLOR ";I;" OF 15"
80 FOR D=1 TO 120
90 NEXT D
100 NEXT I
110 PRINT CHR$(19);"COLOR TEST COMPLETE"
120 END)BASIC" },

    { "MEMTEST", R"BASIC(10 PRINT CHR$(147)
20 PRINT "SCREEN MEMORY TEST"
30 PRINT "WRITING 1000 PATTERN VALUES..."
40 E=0
50 P=0
60 FOR I=0 TO 999
70 POKE 1024+I,P
80 IF PEEK(1024+I)<>P THEN E=E+1
90 P=P+1
100 IF P=256 THEN P=0
110 NEXT I
120 PRINT CHR$(19);"MEMORY TEST COMPLETE"
130 PRINT "ERRORS:";E
140 END)BASIC" },

    { "CPUBENCH", R"BASIC(10 PRINT CHR$(147)
20 PRINT "CPU / WARP BENCHMARK"
30 PRINT "RUNNING 10000 INTEGER OPERATIONS..."
40 T=TI
50 A=0
60 FOR I=1 TO 10000
70 A=A+I
80 B=I*I
90 NEXT I
100 PRINT "DONE."
110 PRINT "JIFFIES:";TI-T
120 PRINT "RESULT:";A
130 PRINT "TRY DIFFERENT CPU SPEEDS."
140 END)BASIC" },

    { "NUMBERCRUNCH", R"BASIC(10 PRINT CHR$(147)
20 PRINT "NUMBER CRUNCHER"
30 PRINT "I PICK A NUMBER FROM 1 TO 100"
40 X=INT(RND(1)*100)+1
50 N=0
60 INPUT "YOUR GUESS";G
70 N=N+1
80 IF G<X THEN PRINT "HIGHER!":GOTO 60
90 IF G>X THEN PRINT "LOWER!":GOTO 60
100 PRINT "YOU GOT IT IN";N;"TRIES!"
110 INPUT "PLAY AGAIN (Y/N)";A$
120 IF A$="Y" THEN GOTO 10
130 END)BASIC" },

    { "MATHBLITZ", R"BASIC(10 PRINT CHR$(147)
20 PRINT "MATH BLITZ"
30 PRINT "ANSWER 8 QUICK QUESTIONS"
40 Q=0
50 S=0
60 Q=Q+1
70 A=INT(RND(1)*9)+1
80 B=INT(RND(1)*9)+1
90 C=A+B
100 PRINT
110 INPUT "ANSWER ";G
120 IF G=C THEN PRINT "CORRECT!":S=S+1:GOTO 140
130 PRINT "NOPE. ANSWER WAS";C
140 IF Q<8 THEN GOTO 60
150 PRINT
160 PRINT "SCORE";S;"OUT OF 8"
170 INPUT "PLAY AGAIN (Y/N)";A$
180 IF A$="Y" THEN GOTO 10
190 END)BASIC" },

    { "TREASURE", R"BASIC(10 PRINT CHR$(147)
20 PRINT "TREASURE HUNT"
30 PRINT "FIND THE TREASURE ON AN 8X8 MAP"
40 X=INT(RND(1)*8)+1
50 Y=INT(RND(1)*8)+1
60 FOR T=1 TO 8
70 INPUT "X COORDINATE 1-8";G
80 INPUT "Y COORDINATE 1-8";H
90 IF G=X AND H=Y THEN GOTO 150
100 D=ABS(G-X)+ABS(H-Y)
110 PRINT "DISTANCE:";D
120 IF G<X THEN PRINT "GO EAST"
130 IF G>X THEN PRINT "GO WEST"
140 IF H<Y THEN PRINT "GO SOUTH"
145 IF H>Y THEN PRINT "GO NORTH"
146 NEXT T
147 PRINT "OUT OF MOVES!"
148 GOTO 170
150 PRINT
155 PRINT "TREASURE FOUND IN";T;"MOVES!"
160 PRINT "YOU WIN!"
170 INPUT "PLAY AGAIN (Y/N)";A$
180 IF A$="Y" THEN GOTO 10
190 END)BASIC" },

    { "TYPINGDASH", R"BASIC(10 PRINT CHR$(147)
20 PRINT "TYPING DASH"
30 PRINT "TYPE THE WORD I GIVE YOU"
40 R=INT(RND(1)*5)+1
50 IF R=1 THEN W$="COMMODORE"
60 IF R=2 THEN W$="WARP"
70 IF R=3 THEN W$="LUMIA"
80 IF R=4 THEN W$="BASIC"
90 IF R=5 THEN W$="PIXEL"
100 PRINT
110 PRINT "TYPE: ";W$
120 T=TI
130 INPUT "GO";A$
140 E=TI-T
150 IF A$=W$ THEN PRINT "NICE! TIME:";E;"JIFFIES":GOTO 170
160 PRINT "MISS! THE WORD WAS ";W$
170 INPUT "PLAY AGAIN (Y/N)";B$
180 IF B$="Y" THEN GOTO 10
190 END)BASIC" },

    { "LETTERHUNT", R"BASIC(10 PRINT CHR$(147)
20 PRINT "LETTER HUNT"
30 PRINT "I WILL PICK A LETTER FROM A TO Z"
40 R=INT(RND(1)*26)+65
50 PRINT "FIND IT!"
60 INPUT "YOUR LETTER";A$
70 IF A$="" THEN GOTO 60
80 IF ASC(A$)=R THEN GOTO 120
90 IF ASC(A$)<R THEN PRINT "LATER IN THE ALPHABET!":GOTO 60
100 IF ASC(A$)>R THEN PRINT "EARLIER IN THE ALPHABET!":GOTO 60
120 PRINT "CORRECT: ";CHR$(R)
130 INPUT "PLAY AGAIN (Y/N)";B$
140 IF B$="Y" THEN GOTO 10
150 END)BASIC" }
};

static int BuiltInCount()
{
    return static_cast<int>(sizeof(kBuiltIns) / sizeof(kBuiltIns[0]));
}

MainPage::MainPage()
    : core(ref new LibretroCore()), bitmap(nullptr), sdFolder(nullptr),
      accumulator(0), c128(false), warpHeld(false), running(false)
{
}

String^ MainPage::CorePath()
{
    return c128 ? L"Cores\\vice_x128_libretro.dll" : L"Cores\\vice_x64_libretro.dll";
}

void MainPage::OnLoaded(Object^, RoutedEventArgs^)
{
    if (!timer)
    {
        timer = ref new Windows::UI::Xaml::DispatcherTimer();
        Windows::Foundation::TimeSpan span;
        span.Duration = 166700;
        timer->Interval = span;
        timer->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &MainPage::TimerTick);
        timer->Start();
    }
    CpuSlider->Value = 1;
    CpuText->Text = L"1.0 MHz";
    BuiltInList->SelectedIndex = 0;
    ProgramName->Text = L"STARTUP";
    ProgramEditor->Text = L"10 PRINT CHR$(147)
20 PRINT "COMMODORE WARP"
30 PRINT "C64 CORE ONLINE"
40 PRINT
50 PRINT "OPEN PROGRAMS FOR TESTS"
60 PRINT "AND BUILT-IN GAMES"
70 PRINT
80 PRINT "CPU SLIDER: 1-64X EFFECTIVE SPEED"
90 PRINT "WARP BUTTON: 16X"
100 END";
    StatusText->Text = L"Booting C64...";

    auto values = ApplicationData::Current->LocalSettings->Values;
    if (values->HasKey(L"SdConsent") && safe_cast<bool>(values->Lookup(L"SdConsent")))
        PrepareSdFolder();
    else
        SdText->Text = L"SD workspace: not enabled.";

    auto bytes = BuildPrg();
    create_task(ApplicationData::Current->LocalFolder->CreateFileAsync(
        L"startup.prg", CreationCollisionOption::ReplaceExisting))
    .then([this, bytes](StorageFile^ file)
    {
        return create_task(FileIO::WriteBytesAsync(file, bytes));
    })
    .then([this](void)
    {
        running = false;
        core->Unload();
        bitmap = nullptr;

        if (!core->LoadCore(CorePath()))
        {
            StatusText->Text = L"BOOT ERROR: " + core->Error;
            return;
        }

        auto path = ApplicationData::Current->LocalFolder->Path + L"\\startup.prg";
        if (!core->LoadGame(path))
        {
            StatusText->Text = L"BOOT ERROR: " + core->Error;
            return;
        }

        bitmap = ref new WriteableBitmap(core->Width, core->Height);
        accumulator = 0;
        running = true;
        StatusText->Text = L"C64 ready. Startup program running.";
        core->RunFrames(2);
        Present();
    })
    .then([](task<void> t)
    {
        try { t.get(); }
        catch (Exception^ ex)
        {
            if (ex) {}
        }
    });
}

void MainPage::Machine_Toggled(Object^ sender, RoutedEventArgs^)
{
    ToggleSwitch^ toggle = dynamic_cast<ToggleSwitch^>(sender);
    if (!toggle) return;
    c128 = toggle->IsOn;
    MachineText->Text = c128 ? L"C128" : L"C64";
    if (running)
    {
        running = false;
        core->Unload();
        bitmap = nullptr;
    }
    StatusText->Text = c128 ? L"C128 selected." : L"C64 selected.";
}

void MainPage::Load_Click(Object^, RoutedEventArgs^)
{
    FileOpenPicker^ picker = ref new FileOpenPicker();
    picker->ViewMode = PickerViewMode::List;
    picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
    picker->FileTypeFilter->Append(L".prg");
    picker->FileTypeFilter->Append(L".d64");
    picker->FileTypeFilter->Append(L".g64");
    picker->FileTypeFilter->Append(L".d81");
    picker->FileTypeFilter->Append(L".t64");
    picker->FileTypeFilter->Append(L".tap");
    picker->FileTypeFilter->Append(L".crt");
    picker->FileTypeFilter->Append(L".zip");

    create_task(picker->PickSingleFileAsync()).then([this](StorageFile^ file)
    {
        if (!file) return;
        create_task(file->CopyAsync(
            ApplicationData::Current->LocalFolder,
            L"warp_input_" + file->Name,
            NameCollisionOption::ReplaceExisting))
        .then([this, file](StorageFile^ copy)
        {
            running = false;
            core->Unload();
            bitmap = nullptr;
            StatusText->Text = L"Loading " + file->Name + L"...";

            if (!core->LoadCore(CorePath()))
            {
                StatusText->Text = core->Error;
                return;
            }
            if (!core->LoadGame(copy->Path))
            {
                StatusText->Text = core->Error;
                return;
            }

            bitmap = ref new WriteableBitmap(core->Width, core->Height);
            accumulator = 0;
            running = true;
            StatusText->Text = L"Running " + file->Name + L" on " +
                (c128 ? L"C128" : L"C64") + L".";
        })
        .then([](task<void> t)
        {
            try { t.get(); } catch (Exception^) {}
        });
    });
}

void MainPage::TimerTick(Object^ sender, Object^ args)
{
    Frame(sender, args);
}

void MainPage::Frame(Object^, Object^)
{
    if (!running || !bitmap) return;
    double dt = 0.01667;
    double mhz = CpuSlider->Value;
    if (warpHeld) mhz = 16;

    accumulator += mhz * core->Fps * dt;
    int frames = static_cast<int>(accumulator);
    if (frames < 1) return;
    if (frames > 400) frames = 400;
    accumulator -= frames;
    core->RunFrames(frames);
    Present();
}

void MainPage::Present()
{
    if (!bitmap || core->Width == 0 || core->Height == 0) return;

    Microsoft::WRL::ComPtr<ICommodoreBufferByteAccess> access;
    IInspectable* inspectable = reinterpret_cast<IInspectable*>(bitmap->PixelBuffer);
    if (!inspectable) return;
    if (FAILED(inspectable->QueryInterface(IID_PPV_ARGS(&access)))) return;

    byte* destination = nullptr;
    if (FAILED(access->Buffer(&destination)) || !destination) return;

    auto source = core->GetFrameCopy();
    unsigned bytes = core->Width * core->Height * 4;
    if (source->Length < bytes) bytes = source->Length;
    if (bytes) memcpy(destination, source->Data, bytes);

    bitmap->Invalidate();
    ScreenImage->Source = bitmap;
}

void MainPage::CpuChanged(Object^, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
    CpuText->Text = ref new String(
        (std::to_wstring(e->NewValue) + L" MHz").c_str());
}

void MainPage::WarpPressed(Object^, PointerRoutedEventArgs^ e)
{
    warpHeld = true;
    e->Handled = true;
}

void MainPage::WarpReleased(Object^, PointerRoutedEventArgs^ e)
{
    warpHeld = false;
    e->Handled = true;
}

void MainPage::TouchPressed(Object^ sender, PointerRoutedEventArgs^)
{
    Button^ button = dynamic_cast<Button^>(sender);
    if (!button) return;
    String^ tag = dynamic_cast<String^>(button->Tag);
    if (tag == L"U") core->SetPad(4, true);
    else if (tag == L"D") core->SetPad(5, true);
    else if (tag == L"L") core->SetPad(6, true);
    else if (tag == L"R") core->SetPad(7, true);
    else if (tag == L"F") core->SetPad(8, true);
}

void MainPage::TouchReleased(Object^ sender, PointerRoutedEventArgs^)
{
    Button^ button = dynamic_cast<Button^>(sender);
    if (!button) return;
    String^ tag = dynamic_cast<String^>(button->Tag);
    if (tag == L"U") core->SetPad(4, false);
    else if (tag == L"D") core->SetPad(5, false);
    else if (tag == L"L") core->SetPad(6, false);
    else if (tag == L"R") core->SetPad(7, false);
    else if (tag == L"F") core->SetPad(8, false);
}

void MainPage::KeyDown(Object^, KeyRoutedEventArgs^ e)
{
    unsigned key = static_cast<unsigned>(e->Key);
    if (e->Key == VirtualKey::Enter) core->Key(true, RETROK_RETURN, '\r');
    else if (e->Key == VirtualKey::Back) core->Key(true, RETROK_BACKSPACE, '\b');
    else if (e->Key == VirtualKey::Space) core->Key(true, RETROK_SPACE, ' ');
    else if (key >= static_cast<unsigned>(VirtualKey::A) &&
             key <= static_cast<unsigned>(VirtualKey::Z))
    {
        unsigned ch = static_cast<unsigned>('a' + key - static_cast<unsigned>(VirtualKey::A));
        core->Key(true, ch, ch);
    }
}

void MainPage::KeyUp(Object^, KeyRoutedEventArgs^ e)
{
    unsigned key = static_cast<unsigned>(e->Key);
    if (e->Key == VirtualKey::Enter) core->Key(false, RETROK_RETURN, '\r');
    else if (e->Key == VirtualKey::Back) core->Key(false, RETROK_BACKSPACE, '\b');
    else if (e->Key == VirtualKey::Space) core->Key(false, RETROK_SPACE, ' ');
    else if (key >= static_cast<unsigned>(VirtualKey::A) &&
             key <= static_cast<unsigned>(VirtualKey::Z))
    {
        unsigned ch = static_cast<unsigned>('a' + key - static_cast<unsigned>(VirtualKey::A));
        core->Key(false, ch, ch);
    }
}

void MainPage::Reset_Click(Object^, RoutedEventArgs^)
{
    core->Reset();
    accumulator = 0;
    StatusText->Text = L"Reset.";
}

void MainPage::Program_Click(Object^, RoutedEventArgs^)
{
    ProgramPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;
    ProgramEditor->Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void MainPage::ProgramBack_Click(Object^, RoutedEventArgs^)
{
    ProgramPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void MainPage::NewProgram_Click(Object^, RoutedEventArgs^)
{
    ProgramName->Text = L"MYPROGRAM";
    ProgramEditor->Text = L"10 PRINT \"HELLO FROM C64\"\n20 GOTO 10";
}

void MainPage::LoadBasic_Click(Object^, RoutedEventArgs^)
{
    FileOpenPicker^ picker = ref new FileOpenPicker();
    picker->ViewMode = PickerViewMode::List;
    picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
    picker->FileTypeFilter->Append(L".bas");
    picker->FileTypeFilter->Append(L".txt");

    create_task(picker->PickSingleFileAsync()).then([this](StorageFile^ file)
    {
        if (!file) return;
        create_task(FileIO::ReadTextAsync(file)).then([this, file](String^ text)
        {
            ProgramEditor->Text = text;
            ProgramName->Text = ToPlatformString(BaseName(file->Name));
            StatusText->Text = L"Loaded " + file->Name + L".";
        })
        .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
    });
}

void MainPage::LoadBuiltIn_Click(Object^, RoutedEventArgs^)
{
    int index = BuiltInList ? BuiltInList->SelectedIndex : -1;
    if (index < 0 || index >= BuiltInCount())
    {
        StatusText->Text = L"Select a built-in program first.";
        return;
    }

    ProgramName->Text = ToPlatformString(kBuiltIns[index].name);
    ProgramEditor->Text = ToPlatformString(kBuiltIns[index].source);
    ProgramPanel->Visibility = Windows::UI::Xaml::Visibility::Visible;

    String^ kind = index < 4 ? L"test program" : L"free built-in game";
    StatusText->Text = L"Loaded built-in " + ToPlatformString(kBuiltIns[index].name) + L" (" + kind + L"). Press RUN BASIC.";
    ProgramEditor->Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void MainPage::RunProgram_Click(Object^, RoutedEventArgs^)
{
    auto bytes = BuildPrg();
    create_task(ApplicationData::Current->LocalFolder->CreateFileAsync(
        L"run_current.prg", CreationCollisionOption::ReplaceExisting))
    .then([this, bytes](StorageFile^ file)
    {
        create_task(FileIO::WriteBytesAsync(file, bytes))
        .then([this, file](void)
        {
            running = false;
            core->Unload();
            bitmap = nullptr;
            StatusText->Text = L"Starting " + file->Name + L"...";

            if (!core->LoadCore(CorePath()))
            {
                StatusText->Text = core->Error;
                return;
            }
            if (!core->LoadGame(file->Path))
            {
                StatusText->Text = core->Error;
                return;
            }

            bitmap = ref new WriteableBitmap(core->Width, core->Height);
            accumulator = 0;
            running = true;
            ProgramPanel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            StatusText->Text = L"Running " + ProgramName->Text + L".";
        })
        .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
    })
    .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
}

void MainPage::SaveLocal_Click(Object^, RoutedEventArgs^)
{
    SaveProgramTo(ApplicationData::Current->LocalFolder, false);
}

void MainPage::EnableSd_Click(Object^, RoutedEventArgs^)
{
    ContentDialog^ dialog = ref new ContentDialog();
    dialog->Title = L"Enable SD workspace?";
    dialog->Content =
        L"Create a CommodoreWarp folder on the first removable device and let this app save the programs you choose there?";
    dialog->PrimaryButtonText = L"ALLOW SD";
    dialog->CloseButtonText = L"CANCEL";

    create_task(dialog->ShowAsync()).then([this](ContentDialogResult result)
    {
        if (result == ContentDialogResult::Primary)
        {
            ApplicationData::Current->LocalSettings->Values->Insert(L"SdConsent", true);
            PrepareSdFolder();
        }
    });
}

void MainPage::PrepareSdFolder()
{
    create_task(KnownFolders::RemovableDevices->GetFoldersAsync())
    .then([this](Windows::Foundation::Collections::IVectorView<StorageFolder^>^ devices)
    {
        if (!devices || devices->Size == 0)
        {
            SdText->Text = L"SD workspace: no removable card detected. Local saves still work.";
            return;
        }

        create_task(devices->GetAt(0)->CreateFolderAsync(
            L"CommodoreWarp", CreationCollisionOption::OpenIfExists))
        .then([this](StorageFolder^ folder)
        {
            if (!folder) return;
            sdFolder = folder;
            try
            {
                StorageApplicationPermissions::FutureAccessList->AddOrReplace(
                    L"CommodoreWarpSD", folder);
            }
            catch (Exception^) {}
            SdText->Text = L"SD workspace: " + folder->Name + L" ✓";
        })
        .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
    })
    .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
}

void MainPage::SaveSd_Click(Object^, RoutedEventArgs^)
{
    if (sdFolder) SaveProgramTo(sdFolder, false);
    else SdText->Text = L"Enable SD first.";
}

void MainPage::SaveProgramTo(StorageFolder^ folder, bool prg)
{
    if (!folder) return;

    std::string base = BaseName(ProgramName->Text);
    std::wstring wide(base.begin(), base.end());
    String^ filename = ref new String(
        (wide + (prg ? L".prg" : L".bas")).c_str());

    create_task(folder->CreateFileAsync(
        filename, CreationCollisionOption::ReplaceExisting))
    .then([this, prg](StorageFile^ file)
    {
        if (prg)
        {
            auto data = BuildPrg();
            create_task(FileIO::WriteBytesAsync(file, data))
            .then([this, file](void)
            {
                StatusText->Text = L"Saved " + file->Name + L".";
            })
            .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
        }
        else
        {
            create_task(FileIO::WriteTextAsync(file, ProgramEditor->Text))
            .then([this, file](void)
            {
                StatusText->Text = L"Saved " + file->Name + L".";
            })
            .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
        }
    })
    .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
}

Array<unsigned char>^ MainPage::BuildPrg()
{
    struct Line { int number; std::vector<unsigned char> code; };

    const std::vector<std::pair<std::string, unsigned char>> tokenList =
    {
        {"INPUT#",0x84},{"PRINT#",0x98},{"RIGHT$",0xC9},{"LEFT$",0xC8},{"MID$",0xCA},
        {"RETURN",0x8E},{"RESTORE",0x8C},{"VERIFY",0x95},{"GOSUB",0x8D},{"CLOSE",0xA0},
        {"CONT",0x9A},{"INPUT",0x85},{"LOAD",0x93},{"SAVE",0x94},{"PRINT",0x99},{"LIST",0x9B},
        {"CLR",0x9C},{"CMD",0x9D},{"SYS",0x9E},{"OPEN",0x9F},{"GET",0xA1},{"NEW",0xA2},
        {"TAB(",0xA3},{"THEN",0xA7},{"STEP",0xA9},{"NEXT",0x82},{"DATA",0x83},{"DIM",0x86},
        {"READ",0x87},{"LET",0x88},{"GOTO",0x89},{"RUN",0x8A},{"IF",0x8B},{"REM",0x8F},
        {"STOP",0x90},{"ON",0x91},{"WAIT",0x92},{"DEF",0x96},{"POKE",0x97},{"FN",0xA5},
        {"TO",0xA4},{"SPC(",0xA6},{"END",0x80},{"FOR",0x81},{"NOT",0xA8},{"AND",0xAF},
        {"OR",0xB0},{"SGN",0xB4},{"INT",0xB5},{"ABS",0xB6},{"USR",0xB7},{"FRE",0xB8},
        {"POS",0xB9},{"SQR",0xBA},{"RND",0xBB},{"LOG",0xBC},{"EXP",0xBD},{"COS",0xBE},
        {"SIN",0xBF},{"TAN",0xC0},{"ATN",0xC1},{"PEEK",0xC2},{"LEN",0xC3},{"STR$",0xC4},
        {"VAL",0xC5},{"ASC",0xC6},{"CHR$",0xC7},{"<",0xB3},{">",0xB1},{"=",0xB2},
        {"^",0xAE},{"/",0xAD},{"*",0xAC},{"+",0xAA},{"-",0xAB}
    };

    auto tokens = tokenList;
    std::sort(tokens.begin(), tokens.end(),
        [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });

    std::string source = ToStd(ProgramEditor->Text);
    std::stringstream stream(source);
    std::string raw;
    std::vector<Line> lines;
    int autoNo = 10;

    while (std::getline(stream, raw))
    {
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();

        size_t p = 0;
        while (p < raw.size() && raw[p] == ' ') ++p;
        size_t start = p;
        while (p < raw.size() && std::isdigit(static_cast<unsigned char>(raw[p]))) ++p;

        bool numbered = p > start;
        int number = numbered ?
            std::atoi(raw.substr(start, p - start).c_str()) : autoNo;
        autoNo = numbered ? number + 10 : autoNo + 10;

        while (p < raw.size() && raw[p] == ' ') ++p;

        std::string body = raw.substr(p);
        std::string upper = Upper(body);
        std::vector<unsigned char> code;
        bool inString = false;

        for (size_t i = 0; i < body.size();)
        {
            char ch = body[i];
            if (ch == '"')
            {
                code.push_back(static_cast<unsigned char>(ch));
                inString = !inString;
                ++i;
                continue;
            }
            if (inString)
            {
                code.push_back(static_cast<unsigned char>(ch));
                ++i;
                continue;
            }

            bool matched = false;
            for (const auto& kv : tokens)
            {
                const std::string& keyword = kv.first;
                if (i + keyword.size() > upper.size()) continue;
                if (upper.compare(i, keyword.size(), keyword) != 0) continue;

                char previous = i ? upper[i - 1] : ' ';
                char next = i + keyword.size() < upper.size() ? upper[i + keyword.size()] : ' ';
                bool word = std::isalnum(static_cast<unsigned char>(keyword[0]));
                bool prevBoundary = std::isalnum(static_cast<unsigned char>(previous)) || previous == '$';
                bool nextBoundary = std::isalnum(static_cast<unsigned char>(next)) || next == '$';
                if (word && (prevBoundary || nextBoundary)) continue;

                code.push_back(kv.second);
                i += keyword.size();
                matched = true;

                if (keyword == "REM")
                    while (i < body.size()) code.push_back(static_cast<unsigned char>(body[i++]));
                break;
            }

            if (!matched)
            {
                code.push_back(static_cast<unsigned char>(
                    std::toupper(static_cast<unsigned char>(ch))));
                ++i;
            }
        }

        lines.push_back({ number, code });
    }

    std::sort(lines.begin(), lines.end(),
        [](const Line& a, const Line& b) { return a.number < b.number; });

    std::vector<Line> unique;
    for (const auto& line : lines)
    {
        if (!unique.empty() && unique.back().number == line.number)
            unique.back() = line;
        else
            unique.push_back(line);
    }

    std::vector<unsigned char> prg = { 0x01, 0x08 };
    unsigned current = 0x0801;

    for (const auto& line : unique)
    {
        unsigned next = current + 4u +
            static_cast<unsigned>(line.code.size()) + 1u;
        prg.push_back(static_cast<unsigned char>(next & 255));
        prg.push_back(static_cast<unsigned char>((next >> 8) & 255));
        prg.push_back(static_cast<unsigned char>(line.number & 255));
        prg.push_back(static_cast<unsigned char>((line.number >> 8) & 255));
        prg.insert(prg.end(), line.code.begin(), line.code.end());
        prg.push_back(0);
        current = next;
    }

    prg.push_back(0);
    prg.push_back(0);

    auto result = ref new Array<unsigned char>(
        static_cast<unsigned>(prg.size()));
    for (unsigned i = 0; i < result->Length; ++i)
        result[i] = prg[i];

    return result;
}

void MainPage::ExportPrg_Click(Object^, RoutedEventArgs^)
{
    FileSavePicker^ picker = ref new FileSavePicker();
    picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;

    std::string base = BaseName(ProgramName->Text);
    std::wstring wide(base.begin(), base.end());
    picker->SuggestedFileName = ref new String((wide + L".prg").c_str());

    auto types = ref new Platform::Collections::Vector<String^>();
    types->Append(L".prg");
    picker->FileTypeChoices->Insert(L"C64 Program", types);

    create_task(picker->PickSaveFileAsync()).then([this](StorageFile^ file)
    {
        if (!file) return;

        auto data = BuildPrg();
        create_task(FileIO::WriteBytesAsync(file, data))
        .then([this, file](void)
        {
            StatusText->Text = L"Exported C64 PRG: " + file->Name;
        })
        .then([](task<void> t) { try { t.get(); } catch (Exception^) {} });
    });
}

void MainPage::ExportPrgSd_Click(Object^, RoutedEventArgs^)
{
    if (sdFolder) SaveProgramTo(sdFolder, true);
    else SdText->Text = L"Enable SD first.";
}

}
