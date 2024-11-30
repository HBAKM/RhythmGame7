// RhythmGame7.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "RhythmGame7.h"
#include <chrono>
#include <vector>
#include <string>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // Windows 멀티미디어 라이브러리 링크
#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
int gametimer;                                
// 게임시작하는 순간 타이머 
// std::chrono: 현재시간 가져오기
// steady_clock: 경과 시간(Elapsed Time)을 계산
// time_point: 특정시점
std::chrono::steady_clock::time_point gameStartTime; //게임시작하는 순간 타이머
HWND hButton;                                   // '시작하기' 버튼 핸들
static bool isHomeScreen = true;                // 홈 화면 표시 여부를 나타내는 플래그
bool isPlaying = false;                         // 게임 진행 여부 플래그
int mouseX, mouseY;                             // 마우스 좌표 변수
bool zPressed = false;                          // z키 누름 여부
bool xPressed = false;                          // x키 누름 여부
std::wstring hitMessage = L"";                  // 히트 메시지 (HIT 또는 MISS)
DWORD hitMessageTime = 0;                       // 메시지가 표시된 시간
const int messageDisplayDuration = 700;         // 메시지 표시 지속 시간 (0.7초)
int score = 0;                                  // 점수: 만점(80)

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RHYTHMGAME7, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RHYTHMGAME7));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RHYTHMGAME7));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RHYTHMGAME7);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}
// 노트 구조체 정의
struct Note {
    int time;                   // 노트 등장 시간
    int x, y;                   // 노트의 좌표
    float startRadius = 90.0f;  // 초기 히트 원 크기
    float currentRadius;        // 현재 반지름(시간에 따라 줄어듦)
    float innerRadius = 50.0f;  // 노트의 크기
    bool isVisible;             // 노트가 보이는지 여부

    // 생성자
    Note(int t, int xpos, int ypos) : time(t), x(xpos), y(ypos) {}
};
std::vector<Note> notes; // 노트 벡터

// 벡터에 노트를 저장하는 함수
void parseNotesFromFile(const std::string& filename) {
    std::ifstream file(filename); // 파일 열기
    std::string line;

    while (std::getline(file, line)) { // 파일의 각 줄을 읽어옴
        // 쉼표 기준으로 나누어서 파싱
        std::stringstream ss(line);
        int time, x, y;
        char comma; // 쉼표를 위한 변수

        if (ss >> time >> comma >> x >> comma >> y) {
            // 벡터에 추가
            notes.push_back(Note(time, x, y));
            //OutputDebugString(L"성공\n");
        }
    }
}
// 노트 로드 함수
void LoadNotes(const std::string& filename) {
    parseNotesFromFile(filename);  // 노트 파일을 파싱하여 노트를 로드
}

// 노트를 그리는 함수
void drawNotes(HDC hdc, int currentTime, std::vector<Note>& notes, int windowWidth, int windowHeight) {   

    for (auto& note : notes) {  // 벡터의 각 Note 객체를 순회
        // 노트가 보이지 않으면 그리지 않음
        if (!note.isVisible) {
            continue;
        }
        if (note.time <= currentTime + 500 && note.time >= currentTime - 500) {  // 약간의 오차 허용 (50ms 이내)
            // 스케일 계산
            float scaleX = static_cast<float>(windowWidth) / 512.0f;  // 기본 너비
            float scaleY = static_cast<float>(windowHeight) / 384.0f; // 기본 높이

            // 노트의 위치를 스케일에 맞게 변환
            int scaledX = static_cast<int>(note.x * scaleX);
            int scaledY = static_cast<int>(note.y * scaleY);

            // 원 크기 감소 (매 프레임마다 일정량 감소)
            float timePassed = static_cast<float>(currentTime - note.time);  // 시간 차이 계산
            float radiusDecay = 0.1f;  // 매 프레임마다 반지름이 0.1만큼 줄어듬
            note.currentRadius = note.startRadius - (timePassed * radiusDecay);

            // 최소 반지름 제한 (안쪽 원보다 작아지지 않도록)
            if (note.currentRadius < 50.0f) {
                note.currentRadius = 50.0f;
            }

            // 안쪽 원 크기 (고정 값으로 설정)
            float innerRadius = note.innerRadius;

            // 펜 생성 (바깥 원의 테두리를 그리기 위해 사용)
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen); // 현재 펜을 선택
            
            // 바깥 원 그리기 (HPEN을 사용하여 테두리만)
            SelectObject(hdc, hPen);
            Ellipse(hdc, scaledX - note.currentRadius, scaledY - note.currentRadius, scaledX + note.currentRadius, scaledY + note.currentRadius);

            SelectObject(hdc, hOldPen);  // 이전 펜으로 복원
            DeleteObject(hPen);          // 펜 삭제

            // 브러시 생성 (안쪽 원을 그리기 위해 사용)
            HBRUSH hBrush = CreateSolidBrush(RGB(61, 80, 115));
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush); // 현재 브러시를 선택
            
            // 안쪽 원 그리기 (HBRUSH로 채우기)
            SelectObject(hdc, hBrush);
            Ellipse(hdc, scaledX - innerRadius, scaledY - innerRadius, scaledX + innerRadius, scaledY + innerRadius);
            
            SelectObject(hdc, hOldBrush);  // 이전 브러시로 복원
            DeleteObject(hBrush);          // 브러시 삭제
        }
    }
}

// 히트타입에 따른 메시지 결정 함수
void HitMessage(bool hitType) {
    if (hitType == true) {
        hitMessage = L"HIT";
        hitMessageTime = GetTickCount();  // 메시지 표시 시간 설정
        score += 1;
    } else {  // hitType == false
        hitMessage = L"MISS";
        hitMessageTime = GetTickCount();  // 메시지 표시 시간 설정
    }
}

// 노트 히트 여부 검사 함수
bool checkHit(Note& note, int mouseX, int mouseY, int windowWidth, int windowHeight) {  
    // 화면 크기에 맞춰 노트의 위치를 비율로 변환
    //512.0f와 384.0f는 게임의 기본 크기
    float scaleX = static_cast<float>(windowWidth) / 512.0f;
    float scaleY = static_cast<float>(windowHeight) / 384.0f;
    // note.x와 note.y는 게임 내에서 사용되는 노트의 좌표
    // 이를 화면 크기 비율에 맞게 변환하여 실제 화면 좌표로 바꿈
    int scaledX = static_cast<int>(note.x * scaleX); // 노트의 가로 좌표를 화면 크기에 맞게 변환
    int scaledY = static_cast<int>(note.y * scaleY); // 노트의 세로 좌표를 화면 크기에 맞게 변환

    // 바깥 원 크기 계산 (시간에 따라 크기가 줄어듬)
    float timePassed = gametimer - note.time;  // 시간 차이 계산
    float radiusDecay = 0.1f;  // 매 프레임마다 반지름이 줄어드는 비율
    float outerRadius = note.startRadius - (timePassed * radiusDecay);  // 바깥 원 크기
    // 안쪽 원 크기
    float innerRadius = note.innerRadius;

    // 마우스가 히트 원 안에 있는지 확인
    int distanceSquared = (mouseX - scaledX) * (mouseX - scaledX) + (mouseY - scaledY) * (mouseY - scaledY);

    // 히트 원 안에 있으면 히트
    if (distanceSquared <= innerRadius * innerRadius) {
        if (note.isVisible) {  // 이미 히트된 노트는 다시 히트되지 않도록
            note.isVisible = false;  // 히트되었으므로 노트를 숨김
            HitMessage(true);  // HIT 처리
            return true;  // 히트했으므로 true 반환
        }
    }
    // 바깥 원이 안쪽 원과 같은 크기일 때
    if (outerRadius <= innerRadius) {
        if (note.isVisible) {  // 이미 히트된 노트는 다시 MISS로 처리하지 않도록
            note.isVisible = false;  // 바깥 원 크기가 안쪽 원과 같아지면 노트를 숨김
            HitMessage(false);  // MISS 처리
            return false;  // MISS 처리되었으므로 false 반환
        }
    }
}

// 모든 노트를 처리했는지 확인하는 함수
bool allNotesHitOrMiss() {
    for (auto& note : notes) {
        if (note.isVisible) {
            return false;  // 아직 처리되지 않은 노트가 있으면 false
        }
    }
    return true;  // 모든 노트가 처리되었으면 true
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // 버튼의 크기 구하기 (너비 200px, 높이 50px)
        int buttonWidth = 200;
        int buttonHeight = 50;
        // 창의 크기 구하기
        RECT rect;
        GetClientRect(hWnd, &rect); // 클라이언트 영역의 크기 얻기
        // 창의 가운데 좌표 계산
        int x = (rect.right - rect.left - buttonWidth) / 2; // 수평 가운데
        int y = (rect.bottom - rect.top - buttonHeight) / 2 + 200; // 수직 가운데보다 아래로 이동
        // '시작하기' 버튼 만들기
        hButton = CreateWindow(L"BUTTON", L"시작하기", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, buttonWidth, buttonHeight, hWnd, (HMENU)1, (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE), NULL);
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (LOWORD(wParam) == 1) { // '시작하기' 버튼 클릭
                ShowWindow(hButton, SW_HIDE); // 버튼 숨기기
                isPlaying = true; // 게임 시작
                gameStartTime = std::chrono::steady_clock::now(); // 게임시간 시간 측정 시작
                PlaySound(TEXT("audio.wav"), 0, SND_FILENAME | SND_ASYNC); // 배경 음악 재생
                isHomeScreen = false; // 홈 화면 비활성화
                LoadNotes("Notes.txt"); // 노트 로드 
                SetTimer(hWnd, 1, 10, NULL);  // // 10ms마다 WM_TIMER 메시지 발생
                InvalidateRect(hWnd, NULL, TRUE); // 화면 무효화
            }
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_TIMER:
    {
        if (isPlaying) {
            // 타이머 갱신
            auto duration = std::chrono::steady_clock::now() - gameStartTime;
            gametimer = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            // 모든 노트를 처리했으면 게임 종료
            if (allNotesHitOrMiss()) {
                KillTimer(hWnd, 1);  // 타이머 멈추기
                // 점수를 포함한 메시지 생성
                wchar_t scoreMessage[256];
                swprintf(scoreMessage, 256, L"게임이 종료되었습니다!\n최종 점수: %d/80", score);
                MessageBox(hWnd, scoreMessage, L"게임 종료", MB_OK | MB_ICONINFORMATION);
                PostQuitMessage(0);  // 게임 종료
            }

            InvalidateRect(hWnd, NULL, TRUE);  // 화면 갱신
        }
    }
    break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // 클라이언트 영역 크기를 가져와 창 크기에 맞게 설정
            RECT rect;
            GetClientRect(hWnd, &rect);
            int width = rect.right;
            int height = rect.bottom;

            if (isPlaying) {//게임 화면
                // 노트 그리기
                drawNotes(hdc, gametimer, notes, width, height);
                
                // HIT, MISS 메시지 표시
                if (!hitMessage.empty() && (GetTickCount() - hitMessageTime < messageDisplayDuration)) {

                    // 화면 중앙 상단 위치 계산
                    int centerX = rect.right / 2;
                    int centerY = rect.bottom / 4;

                    if (hitMessage == L"HIT") {
                        SetTextColor(hdc, RGB(0, 160, 0));  // 녹색
                    }
                    else if (hitMessage == L"MISS") {
                        SetTextColor(hdc, RGB(200, 0, 0));  // 빨간색
                    }

                    SetBkMode(hdc, TRANSPARENT);           // 배경 투명 설정
                    
                    // 폰트 설정
                    HFONT hFont = CreateFont(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"));
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                    // 텍스트 출력
                    TextOut(hdc, centerX - 50, centerY, hitMessage.c_str(), hitMessage.length());

                    // 정리
                    SelectObject(hdc, hOldFont);
                    DeleteObject(hFont);
                }
                // 점수 표시 
                SetTextColor(hdc, RGB(0, 0, 0));  // 검은색
                SetBkMode(hdc, TRANSPARENT); // 배경 투명 설정

                HFONT scoreFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
                HFONT oldFont = (HFONT)SelectObject(hdc, scoreFont);

                // 화면 하단에 점수 표시
                int scoreX = (width - 200) / 2; // 화면 중앙에 위치
                int scoreY = height - 50;

                // 텍스트 출력
                std::wstring scoreText = L"Score: " + std::to_wstring(score);
                TextOut(hdc, scoreX, scoreY, scoreText.c_str(), scoreText.length());

                // 정리
                SelectObject(hdc, oldFont);
                DeleteObject(scoreFont);
            }

            if (isHomeScreen) { // 홈 화면
                // 게임 제목 및 설명 텍스트 그리기
                SetBkMode(hdc, TRANSPARENT); // 투명 배경으로 설정

                HFONT hFont = CreateFont(100, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                // 텍스트를 중앙에 위치시키기 위한 좌표 계산
                int textX = (width - 700) / 2;
                int textY = (height - 100) / 2 - 200;  // 수직 중앙보다 위로 이동

                RECT textRect = { textX, textY, textX + 700, textY + 100 };

                // 텍스트 그리기
                SetTextColor(hdc, RGB(1, 45, 106)); // 텍스트 색상 설정
                DrawText(hdc, L"RhythmGame", -1, &textRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

                // 정리
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);

                // 게임 방법 텍스트 넣기
                HFONT instructionFont = CreateFont(30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
                HFONT oldFont = (HFONT)SelectObject(hdc, instructionFont);

                RECT instructionRect = { textX, textY + 200, textX + 700, textY + 250 };

                // 게임 방법 텍스트 그리기
                DrawText(hdc, L"!노트에 마우스를 가져가세요! (사운드 있음)", -1, &instructionRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

                // 정리
                SelectObject(hdc, oldFont);
                DeleteObject(instructionFont);
                DeleteObject(instructionFont);
            }

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_MOUSEMOVE:
    {
        // 클라이언트 영역 크기를 가져와 창 크기에 맞게 설정
        RECT rect;
        GetClientRect(hWnd, &rect);
        int width = rect.right;
        int height = rect.bottom;

        // 마우스 위치 추적
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        // 노트 히트 여부 검사 (마우스 위치 확인)
        for (auto& note : notes) {
            checkHit(note, mouseX, mouseY, width, height);   
        }
        break;
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
