#include "pch.h"
#include "MainPage.xaml.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
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
    CompositionTarget::Rendering += ref new RenderingEventHandler(this, &MainPage::Frame);
    CpuSlider->Value = 1;
    CpuText->Text = L"1.0 MHz";
    StatusText->Text = L"Ready. C64 is the default.";

    auto values = ApplicationData::Current->LocalSettings->Values;
    if (values->HasKey(L"SdConsent") && safe_cast<bool>(values->Lookup(L"SdConsent")))
        PrepareSdFolder();
    else
        SdText->Text = L"SD workspace: not enabled.";
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
