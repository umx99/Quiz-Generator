#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

// --- DATA STRUCTURES (Preserved) ---
struct Question { //read questions
    string text; 
    vector<string> options;
    int correctIdx;
};

struct Chapter { //connect chapter with files
    string name;
    string file;
};

// --- LOAD FUNCTION (Preserved) ---
vector<Question> LoadQuestionsFromFile(string filename) { //reads options and correct options for output
    vector<Question> bank;
    ifstream file(filename.c_str());
    if (!file.is_open()) return bank;
    string line;
    while (getline(file, line)) {
        if (line.length() < 5) continue; 
        stringstream ss(line);
        Question q;
        getline(ss, q.text, '|');
        for (int i = 0; i < 4; i++) {
            string opt; getline(ss, opt, '|');
            q.options.push_back(opt);
        }
        string ansIdx; getline(ss, ansIdx);
        q.correctIdx = atoi(ansIdx.c_str());
        bank.push_back(q);
    }
    file.close();
    return bank;
}

enum GameState { MENU, TOPIC_SELECT, MODULE_SELECT, QUIZ, FEEDBACK, RESULTS }; //states

int main() {
    InitWindow(1000, 650, "UAN QUIZ MASTER");
    InitAudioDevice();

    Font agencyFont = LoadFontEx("AGENCYR.TTF", 96, 0, 0);
    SetTextureFilter(agencyFont.texture, TEXTURE_FILTER_BILINEAR);
    
    // sounds triggered
    Sound sndClick = LoadSound("click.wav");
    Sound sndCorrect = LoadSound("cheer.wav");
    Sound sndWrong = LoadSound("wrong.wav");
    Sound sndTease = LoadSound("tease.wav");     // Restored
    Sound sndCheer = LoadSound("correct.wav");     // Celebration/Bonus
    Sound sndCheer1 = LoadSound("cheer1.wav");

    GameState state = MENU; //menu state
    vector<Question> activeQuiz;
    vector<Chapter> chapterList; 
    int currentQ = 0, score = 0, maxQuestions = 15, lastSelected = -1; //initialization of varaibles
    int correctStreak = 0, wrongStreak = 0;      // Streak Tracking
    float feedbackTimer = 0.0f, quizTimer = 0.0f;
    string currentSubject = "";
    
    string subjects[] = {"Calculus", "Info Security", "Programming", "Pak Studies", "Physics", "English"}; //

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        if (state == MENU) {
            for (int i = 0; i < 6; i++) {
                int col = i % 2, row = i / 2;
                Rectangle btn = { 150.0f + (col * 380), 180.0f + (row * 80), 320, 60 };
                if (CheckCollisionPointRec(mouse, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    PlaySound(sndClick);
                    currentSubject = subjects[i];
                    chapterList.clear();
                    if (currentSubject == "Calculus") {
                        chapterList.push_back({"Limits & Continuity", "c1.txt"});
                        chapterList.push_back({"Derivatives", "c2.txt"});
                        chapterList.push_back({"Integrals", "c3.txt"});
                    } else if (currentSubject == "Info Security") {
                        chapterList.push_back({"Cryptography", "s1.txt"});
                        chapterList.push_back({"Network Security", "s2.txt"});
                        chapterList.push_back({"Risk Management", "s3.txt"});
                    } else if (currentSubject == "Programming") {
                        chapterList.push_back({"Basics & Syntax", "p1.txt"});
                        chapterList.push_back({"Functions & Scope", "p2.txt"});
                        chapterList.push_back({"OOP Principles", "p3.txt"});
                    } else if (currentSubject == "Pak Studies") {
                        chapterList.push_back({"Pre-Independence", "pk1.txt"});
                        chapterList.push_back({"Post-Independence", "pk2.txt"});
                        chapterList.push_back({"Geography", "pk3.txt"});
                    } else if (currentSubject == "Physics") {
                        chapterList.push_back({"Mechanics", "phy1.txt"});
                        chapterList.push_back({"Electromagnetism", "phy2.txt"});
                        chapterList.push_back({"Modern Physics", "phy3.txt"});
                    } else if (currentSubject == "English") {
                        chapterList.push_back({"Grammar", "e1.txt"});
                        chapterList.push_back({"Vocabulary", "e2.txt"});
                        chapterList.push_back({"Comprehension", "e3.txt"});
                    }
                    state = TOPIC_SELECT;
                }
            }
        }
        else if (state == TOPIC_SELECT) { //further topic page
            for (int i = 0; i < (int)chapterList.size(); i++) {
                Rectangle btn = { 300, 180.0f + (i * 75), 400, 60 };
                if (CheckCollisionPointRec(mouse, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    PlaySound(sndClick);
                    activeQuiz = LoadQuestionsFromFile(chapterList[i].file);
                    if (!activeQuiz.empty()) state = MODULE_SELECT;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;
        }
        else if (state == MODULE_SELECT) { // mcqs number
            int sizes[] = {15, 20, 30};
            for(int i = 0; i < 3; i++) {
                Rectangle btn = { 350, 220.0f + (i * 80), 300, 60 };
                if (CheckCollisionPointRec(mouse, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    PlaySound(sndClick);
                    maxQuestions = sizes[i];
                    auto rng = default_random_engine((unsigned int)time(NULL));
                    shuffle(activeQuiz.begin(), activeQuiz.end(), rng);
                    if(activeQuiz.size() > (size_t)maxQuestions) activeQuiz.resize(maxQuestions);
                    currentQ = 0; score = 0; quizTimer = 0; correctStreak = 0; wrongStreak = 0;
                    state = QUIZ;
                }
            }
        }
        else if (state == QUIZ) { //quiz started
            quizTimer += GetFrameTime();
            for (int i = 0; i < 4; i++) {
                Rectangle btn = { 200, 280.0f + (i * 75), 600, 60 };
                if (CheckCollisionPointRec(mouse, btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    lastSelected = i;
                    if (i == activeQuiz[currentQ].correctIdx) { 
                        score += 1; 
                        correctStreak++; wrongStreak = 0;
                        if (correctStreak >=6) {PlaySound(sndCheer1); /*score += 1;*/ } 
                        if (correctStreak == 4) {PlaySound(sndCheer); /*score += 1;*/ } 
                        else {PlaySound(sndCorrect); }
                    } 
                    else {
                        wrongStreak++; correctStreak = 0;
                        if (wrongStreak >= 4) { PlaySound(sndTease); }
                        else { PlaySound(sndWrong); }
                    }
                    feedbackTimer = (float)GetTime();
                    state = FEEDBACK;
                }
            }
        }
        else if (state == FEEDBACK) { //results
            if (GetTime() - feedbackTimer > 1.2f) {
                currentQ++;
                if (currentQ >= (int)activeQuiz.size()) state = RESULTS;
                else state = QUIZ;
            }
        }
        else if (state == RESULTS) { //click to go back to menu
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) state = MENU;
        }

        // --- DRAWING ---
        BeginDrawing();
        ClearBackground({ 245, 247, 250, 255 }); // Professional Light Theme

        if (state == MENU) {
            DrawTextEx(agencyFont, "UAN ACADEMIC QUIZ", { 270, 60 }, 55, 2, DARKGRAY);
            for (int i = 0; i < 6; i++) {
                int col = i % 2, row = i / 2;
                Rectangle btn = { 150.0f + (col * 380), 180.0f + (row * 80), 320, 60 };
                bool h = CheckCollisionPointRec(mouse, btn);
                DrawRectangleRounded(btn, 0.2f, 8, h ? SKYBLUE : WHITE);
                DrawRectangleRoundedLines(btn, 0.2f, 8, h ? BLUE : LIGHTGRAY);
                DrawTextEx(agencyFont, subjects[i].c_str(), { btn.x + 20, btn.y + 18 }, 24, 1, h ? WHITE : DARKGRAY);
            }
        }
        else if (state == TOPIC_SELECT) {
            DrawTextEx(agencyFont, TextFormat("%s: MODULES", currentSubject.c_str()), { 350, 80 }, 40, 2, DARKGRAY);
            for (int i = 0; i < (int)chapterList.size(); i++) {
                Rectangle btn = { 300, 180.0f + (i * 75), 400, 60 };
                bool h = CheckCollisionPointRec(mouse, btn);
                DrawRectangleRounded(btn, 0.2f, 8, h ? SKYBLUE : WHITE);
                DrawRectangleRoundedLines(btn, 0.2f, 8, h ? BLUE : LIGHTGRAY);
                DrawTextEx(agencyFont, chapterList[i].name.c_str(), { btn.x + 20, btn.y + 18 }, 24, 1, h ? WHITE : DARKGRAY);
            }
        }
        else if (state == MODULE_SELECT) {
            DrawTextEx(agencyFont, "Select MCQ COUNT", { 340, 120 }, 40, 2, DARKGRAY);
            int labels[] = {15, 20, 30};
            for(int i=0; i<3; i++) {
                Rectangle btn = { 350, 220.0f + (i * 80), 300, 60 };
                bool h = CheckCollisionPointRec(mouse, btn);
                DrawRectangleRounded(btn, 0.2f, 8, h ? SKYBLUE : WHITE);
                DrawRectangleRoundedLines(btn, 0.2f, 8, h ? BLUE : LIGHTGRAY);
                DrawTextEx(agencyFont, TextFormat("%d QUESTIONS", labels[i]), { btn.x + 85, btn.y + 18 }, 24, 1, h ? WHITE : DARKGRAY);
            }
        }
        else if (state == QUIZ || state == FEEDBACK) {
            // 1. TOP LEFT: TIMER
            DrawTextEx(agencyFont, TextFormat("TIME: %.1fs", quizTimer), { 30, 30 }, 25, 1, DARKGRAY);
            
            // 2. TOP RIGHT: QUESTION NO
            DrawTextEx(agencyFont, TextFormat("MCQ: %d/%d", currentQ + 1, (int)activeQuiz.size()), { 880, 30 }, 25, 1, DARKGRAY);
            
            // 3. TOP CENTER: STREAKS
            if (correctStreak > 0) DrawTextEx(agencyFont, TextFormat("Correct Streak: %d :)", correctStreak), { 400, 30 }, 25, 1, LIME);
            else if (wrongStreak > 0) DrawTextEx(agencyFont, TextFormat("Incorrect Streak: %d :(", wrongStreak), { 410, 30 }, 25, 1, MAROON);

            // 4. PROGRESS BAR (Motivation)
            float prog = (float)(currentQ + 1) / (float)activeQuiz.size();
            DrawRectangle(50, 70, 900, 12, LIGHTGRAY); // Bar Background
            DrawRectangle(50, 70, (int)(prog * 900), 12, SKYBLUE);   // Bar Fill

            // Question Card
            DrawRectangleRounded({ 100, 130, 800, 110 }, 0.1f, 8, WHITE);
            DrawRectangleRoundedLines({ 100, 130, 800, 110 }, 0.1f, 8, LIGHTGRAY);
            DrawTextEx(agencyFont, activeQuiz[currentQ].text.c_str(), { 130, 165 }, 28, 1, BLACK);

            for (int i = 0; i < 4; i++) {
                Rectangle btn = { 200, 280.0f + (i * 75), 600, 60 };
                Color bCol = WHITE;
                if (state == FEEDBACK) {
                    if (i == activeQuiz[currentQ].correctIdx) bCol = LIME;
                    else if (i == lastSelected) bCol = MAROON;
                } else if (CheckCollisionPointRec(mouse, btn)) bCol = GetColor(0xF5F5F5FF);

                DrawRectangleRounded(btn, 0.2f, 8, bCol);
                DrawRectangleRoundedLines(btn, 0.2f, 8, LIGHTGRAY);
                DrawTextEx(agencyFont, activeQuiz[currentQ].options[i].c_str(), { btn.x + 30, btn.y + 18 }, 22, 1, DARKGRAY);
            }
        }
        else if (state == RESULTS) {
            DrawTextEx(agencyFont, "SESSION COMPLETED", { 330, 200 }, 60, 2, DARKGRAY);
            DrawTextEx(agencyFont, TextFormat("Result (Correct MCQs): %d", score), { 385, 290 }, 30, 1, DARKGREEN);
            DrawTextEx(agencyFont, "CLICK ANYWHERE TO RESTART", { 380, 450 }, 20, 1, GRAY);
        }

        EndDrawing();
    }

    UnloadSound(sndTease); UnloadSound(sndCheer);
    UnloadFont(agencyFont);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}