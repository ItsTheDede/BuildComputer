#include "raylib.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;

// ==================== ESTRUTURAS ====================

struct Component {
    string id, tipo, nome, specs;
    double preco;
};

struct Account {
    string username, password, email, phone;
    bool isAdmin;
};

struct Card {
    Rectangle bounds;
    string title;
    string description;
    Color iconColor;
    bool isHovered;
    int targetScreen;
    string iconPath;
    Texture2D iconTexture;
    bool iconLoaded;
    
    Card(Rectangle b, const string& t, const string& d, Color c, bool h, int s, const string& icon = "")
        : bounds(b), title(t), description(d), iconColor(c), isHovered(h), targetScreen(s),
          iconPath(icon), iconLoaded(false) {
        iconTexture.id = 0;
    }
};

struct PCSlot {
    Rectangle bounds;
    string tipo;
    string label;
    bool filled;
    Component component;
    
    PCSlot(Rectangle b, const string& t, const string& l)
        : bounds(b), tipo(t), label(l), filled(false) {}
};

// ==================== ENUMERAÇÕES ====================

enum Screen { 
    LOGIN, REGISTER, MENU, ESCOLHER_PECAS, VER_PRECOS, 
    COMO_MONTAR, ADMIN_PANEL, ADMIN_ADD, ADMIN_EDIT, ADMIN_DELETE 
};

// ==================== VARIÁVEIS GLOBAIS ====================

vector<Component> catalog, buildAtual;
vector<Account> accounts;
vector<PCSlot> pcSlots;
vector<Card> menuCards;
string currentUser = "";
bool isCurrentUserAdmin = false;

Screen currentScreen = LOGIN;

string loginUser = "", loginPass = "";
bool loginFocusUser = true;
string registerUser = "", registerPass = "", registerPassConfirm = "", registerEmail = "", registerPhone = "", registerMessage = "";
int registerFocusField = 0;

string adminId = "", adminTipo = "", adminNome = "", adminSpecs = "", adminPreco = "", adminMessage = "";
int adminFocusField = 0, adminSelectedIndex = -1, adminScrollOffset = 0;

string filtroCategoria = "Todos", feedbackMessage = "";
int pecasScrollOffset = 0, feedbackTimer = 0;
Font arialFont;
Texture2D titleImage;

Component* draggedComponent = nullptr;
Vector2 dragOffset = {0, 0};

// ==================== FUNÇÕES AUXILIARES ====================

static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

int getCenterX() { return GetScreenWidth() / 2; }
int getCenterY() { return GetScreenHeight() / 2; }

void DrawWolfLogo(float x, float y, float size) {
    static Texture2D logo = { 0 };
    static bool logoLoaded = false;
    if (!logoLoaded) {
        logo = LoadTexture("resourcer/logo.png");
        if (logo.id == 0) {
            const char* altPaths[] = {
                "resources/Logo Build Computer.png",
                "resources/logo.png",
                "logo.png"
            };
            for (const char* p : altPaths) {
                if (logo.id != 0) break;
                logo = LoadTexture(p);
            }
        }
        logoLoaded = true;
    }
 
    if (logo.id != 0) {
        Rectangle src = { 0.0f, 0.0f, (float)logo.width, (float)logo.height };
        Rectangle dest = { x, y, size, size };
        Vector2 origin = { 0.0f, 0.0f };
        DrawTexturePro(logo, src, dest, origin, 0.0f, WHITE);
        return;
    }
 
    float cx = x + size * 0.5f;
    float cy = y + size * 0.5f;
    DrawCircle(cx, cy, size * 0.45f, DARKGRAY);
    DrawCircleLines(cx, cy, size * 0.45f, WHITE);
}

// ==================== DATABASE ====================

void saveAccounts() {
    ofstream f("accounts.db");
    if (!f.is_open()) return;
    for (auto &acc : accounts)
        f << acc.username << ";" << acc.password << ";" << acc.email << ";" << acc.phone << ";" << (acc.isAdmin ? "1" : "0") << "\n";
    f.close();
}

void loadAccounts() {
    ifstream f("accounts.db");
    if (!f.is_open()) {
        accounts.push_back({"admin", "Programador", "admin@buildpc.com", "912345678", true});
        accounts.push_back({"lima", "programador", "lima@buildpc.com", "913456789", true});
        saveAccounts();
        return;
    }
    
    accounts.clear();
    string line;
    while (getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        vector<string> parts;
        string cur;
        for (char c : line) {
            if (c == ';') { parts.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        parts.push_back(cur);
        
        if (parts.size() >= 5) {
            Account acc;
            acc.username = trim(parts[0]);
            acc.password = trim(parts[1]);
            acc.email = trim(parts[2]);
            acc.phone = trim(parts[3]);
            acc.isAdmin = (trim(parts[4]) == "1");
            accounts.push_back(acc);
        }
    }
    f.close();
}

bool validateEmail(const string &email) {
    if (email.empty()) return false;
    size_t atPos = email.find('@');
    if (atPos == string::npos || atPos == 0 || atPos == email.length() - 1) return false;
    size_t dotPos = email.find('.', atPos);
    return !(dotPos == string::npos || dotPos == email.length() - 1);
}

bool validatePhone(const string &phone) {
    if (phone.length() < 9) return false;
    for (char c : phone)
        if (!isdigit(c) && c != '+' && c != ' ' && c != '-') return false;
    return true;
}

void saveComponents() {
    ofstream f("componentes.realyb");
    if (!f.is_open()) return;
    for (auto &c : catalog) 
        f << c.id << ";" << c.tipo << ";" << c.nome << ";" << c.specs << ";" << c.preco << "\n";
    f.close();
}

bool loadComponents(const string &path) {
    ifstream f(path);
    if (!f.is_open()) return false;
    catalog.clear();
    string line;
    while (getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        vector<string> parts;
        string cur;
        for (char c : line) {
            if (c == ';') { parts.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        parts.push_back(cur);
        
        if (parts.size() < 5) continue;
        
        Component c;
        c.id = trim(parts[0]);
        c.tipo = trim(parts[1]);
        c.nome = trim(parts[2]);
        c.specs = trim(parts[3]);
        try { c.preco = stod(trim(parts[4])); } 
        catch(...) { c.preco = 0.0; }
        catalog.push_back(c);
    }
    f.close();
    return true;
}

string formatBR(double v) {
    stringstream ss;
    ss << fixed << setprecision(2) << v;
    string s = ss.str();
    size_t dotPos = s.find('.');
    if (dotPos != string::npos) s[dotPos] = ',';
    return "€ " + s;
}

double calcularTotal() {
    double total = 0.0;
    for (auto &c : buildAtual) total += c.preco;
    return total;
}

void initPCSlots() {
    pcSlots.clear();
    int startX = 100;
    int startY = 200;
    
    pcSlots.push_back(PCSlot({(float)startX, (float)startY, 180, 400}, "Case", "Gabinete"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 20), 140, 50}, "Motherboard", "Placa-Mae"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 80), 140, 40}, "CPU", "Processador"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 130), 140, 60}, "GPU", "Placa Video"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 200), 140, 40}, "RAM", "Memoria RAM"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 250), 140, 40}, "Storage", "Armazenamento"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 300), 140, 40}, "PSU", "Fonte"));
    pcSlots.push_back(PCSlot({(float)(startX + 20), (float)(startY + 350), 140, 40}, "Cooler", "Cooler"));
}

// ==================== FORWARD DECLARATIONS ====================
void drawTextBox(const char* label, string &text, Rectangle box, bool focused, bool maskPassword = false);
void DrawCard(Card& card);
void drawEscolherPecasScreen();

// ==================== UI COMPONENTS ====================

void DrawNavBar() {
    int sw = GetScreenWidth();
    DrawRectangle(0, 0, sw, 80, BLACK);
    DrawLine(0, 80, sw, 80, RED);
    DrawWolfLogo(20, 15, 50);
    
    if (titleImage.id != 0) {
        float desiredH = 40.0f;
        float scale = desiredH / (float)titleImage.height;
        DrawTextureEx(titleImage, {85, 20}, 0.0f, scale, WHITE);
    } else {
        DrawTextEx(arialFont, "Build", {85, 20}, 28, 1, RED);
        DrawTextEx(arialFont, "Computer", {175, 20}, 28, 1, WHITE);
    }
}

bool DrawButton(Rectangle bounds, const char* text, Color normalColor, Color hoverColor, Color textColor) {
    Vector2 mousePos = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mousePos, bounds);
    bool isClicked = isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
 
    Color currentColor = isHovered ? hoverColor : normalColor;
    DrawRectangleRounded(bounds, 0.2f, 20, currentColor);
 
    if (isHovered) {
        DrawRectangleRoundedLines(bounds, 0.2f, 20, RED);
    }
 
    Vector2 textSize = MeasureTextEx(arialFont, text, 20, 1);
    DrawTextEx(arialFont, text,
        {bounds.x + (bounds.width - textSize.x) / 2, bounds.y + (bounds.height - textSize.y) / 2},
        20, 1, textColor);
 
    return isClicked;
}

void DrawCard(Card& card) {
    Vector2 mousePos = GetMousePosition();
    card.isHovered = CheckCollisionPointRec(mousePos, card.bounds);
 
    if (card.isHovered) {
        DrawRectangleRounded(
            {card.bounds.x + 5, card.bounds.y + 5, card.bounds.width, card.bounds.height},
            0.1f, 20, Fade(RED, 0.2f)
        );
    }
 
    DrawRectangleRounded(card.bounds, 0.1f, 20, {30, 30, 30, 200});
    Color borderColor = card.isHovered ? RED : GRAY;
    DrawRectangleRoundedLines(card.bounds, 0.1f, 20, borderColor);
 
    float centerX = card.bounds.x + card.bounds.width / 2;
    float iconY = card.bounds.y + 60;
    float iconDiameter = 80.0f;
    DrawCircle((int)centerX, (int)iconY, iconDiameter * 0.5f, Fade(RED, 0.2f));
    
    if (!card.iconLoaded && !card.iconPath.empty()) {
        card.iconTexture = LoadTexture(card.iconPath.c_str());
        card.iconLoaded = true;
    }
    
    if (card.iconTexture.id != 0) {
        float texW = (float)card.iconTexture.width;
        float texH = (float)card.iconTexture.height;
        float scale = fminf(iconDiameter / texW, iconDiameter / texH);
        float drawW = texW * scale;
        float drawH = texH * scale;
        
        Rectangle src = {0, 0, texW, texH};
        Rectangle dest = {centerX - drawW * 0.5f, iconY - drawH * 0.5f, drawW, drawH};
        DrawTexturePro(card.iconTexture, src, dest, {0, 0}, 0.0f, WHITE);
    } else {
        const char* fallbackIcon = "$";
        Vector2 iconSize = MeasureTextEx(arialFont, fallbackIcon, 40, 1);
        DrawTextEx(arialFont, fallbackIcon, {centerX - iconSize.x / 2, iconY - 20}, 40, 1, RED);
    }
 
    Vector2 titleSize = MeasureTextEx(arialFont, card.title.c_str(), 24, 1);
    DrawTextEx(arialFont, card.title.c_str(), {centerX - titleSize.x / 2, iconY + 50}, 24, 1, WHITE);
 
    Vector2 descSize = MeasureTextEx(arialFont, card.description.c_str(), 16, 1);
    DrawTextEx(arialFont, card.description.c_str(), {centerX - descSize.x / 2, iconY + 85}, 16, 1, GRAY);
 
    Rectangle btnBounds = {
        card.bounds.x + 20,
        card.bounds.y + card.bounds.height - 60,
        card.bounds.width - 40,
        40
    };
 
    DrawRectangleRounded(btnBounds, 0.2f, 20, Fade(BLACK, 0.5f));
    DrawRectangleRoundedLines(btnBounds, 0.2f, 20, RED);
 
    Vector2 btnTextSize = MeasureTextEx(arialFont, "Acessar", 18, 1);
    DrawTextEx(arialFont, "Acessar",
        {btnBounds.x + (btnBounds.width - btnTextSize.x) / 2,
         btnBounds.y + (btnBounds.height - btnTextSize.y) / 2},
        18, 1, WHITE);
}

void drawTextBox(const char* label, string &text, Rectangle box, bool focused, bool maskPassword) {
    DrawTextEx(arialFont, label, {box.x, box.y - 25}, 18, 1, GRAY);
    DrawRectangleRounded(box, 0.1f, 20, focused ? Color{50, 0, 0, 255} : Color{30, 30, 30, 255});
    DrawRectangleRoundedLines(box, 0.1f, 20, focused ? RED : GRAY);
    
    string display = maskPassword ? string(text.length(), '*') : text;
    Vector2 textSize = MeasureTextEx(arialFont, display.c_str(), 18, 1);
    DrawTextEx(arialFont, display.c_str(), {box.x + 10, box.y + (box.height - textSize.y) / 2}, 18, 1, WHITE);
}

// ==================== TELAS ====================

void drawVerPrecosScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "VER PRECOS DA BUILD", {50, 120}, 32, 1, RED);
    
    if (buildAtual.empty()) {
        DrawTextEx(arialFont, "Nenhum componente selecionado ainda!", 
                  {50, 200}, 24, 1, GRAY);
        DrawTextEx(arialFont, "Va para 'Escolher Pecas' e adicione componentes.", 
                  {50, 240}, 18, 1, GRAY);
        return;
    }
    
    int listY = 180;
    
    DrawRectangle(40, listY - 10, GetScreenWidth() - 80, 40, {30, 30, 30, 255});
    DrawTextEx(arialFont, "Componente", {50, (float)listY}, 18, 1, RED);
    DrawTextEx(arialFont, "Especificacoes", {400, (float)listY}, 18, 1, RED);
    DrawTextEx(arialFont, "Preco", {(float)(GetScreenWidth() - 200), (float)listY}, 18, 1, RED);
    
    listY += 50;
    
    for (const auto &c : buildAtual) {
        DrawRectangleRounded({40, (float)listY, (float)(GetScreenWidth() - 80), 60}, 0.1f, 20, Fade(WHITE, 0.1f));
        DrawRectangleRoundedLines({40, (float)listY, (float)(GetScreenWidth() - 80), 60}, 0.1f, 20, GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s]", c.tipo.c_str()), {50, (float)(listY + 10)}, 14, 1, RED);
        DrawTextEx(arialFont, c.nome.c_str(), {50, (float)(listY + 30)}, 16, 1, WHITE);
        DrawTextEx(arialFont, c.specs.c_str(), {400, (float)(listY + 20)}, 14, 1, GRAY);
        
        string precoStr = formatBR(c.preco);
        DrawTextEx(arialFont, precoStr.c_str(), 
                  {(float)(GetScreenWidth() - 200), (float)(listY + 20)}, 18, 1, GREEN);
        
        listY += 70;
    }
    
    DrawRectangle(0, GetScreenHeight() - 100, GetScreenWidth(), 100, {20, 20, 20, 255});
    DrawLine(0, GetScreenHeight() - 100, GetScreenWidth(), GetScreenHeight() - 100, RED);
    
    DrawTextEx(arialFont, "TOTAL DA BUILD:", 
               {50, (float)(GetScreenHeight() - 70)}, 28, 1, WHITE);
    
    string totalStr = formatBR(calcularTotal());
    Vector2 totalSize = MeasureTextEx(arialFont, totalStr.c_str(), 36, 1);
    DrawTextEx(arialFont, totalStr.c_str(), 
               {GetScreenWidth() - totalSize.x - 50, (float)(GetScreenHeight() - 65)}, 36, 1, GREEN);
    
    Rectangle clearBtn = {50, (float)(GetScreenHeight() - 35), 180, 30};
    if (DrawButton(clearBtn, "Limpar Build", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        buildAtual.clear();
        feedbackMessage = "Build limpa!";
        feedbackTimer = 120;
    }
}

void drawComoMontarScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "MONTAR PC - Arraste as pecas", {50, 120}, 32, 1, RED);
    
    DrawRectangleRounded({90, 190, 200, 420}, 0.05f, 20, {40, 40, 40, 255});
    DrawRectangleRoundedLines({90, 190, 200, 420}, 0.05f, 20, RED);
    
    for (auto &slot : pcSlots) {
        if (slot.tipo == "Case") continue;
        
        Color slotColor = slot.filled ? Fade(GREEN, 0.3f) : Fade(RED, 0.2f);
        DrawRectangleRounded(slot.bounds, 0.1f, 20, slotColor);
        DrawRectangleRoundedLines(slot.bounds, 0.1f, 20, slot.filled ? GREEN : GRAY);
        
        if (slot.filled) {
            Vector2 nameSize = MeasureTextEx(arialFont, slot.component.nome.c_str(), 10, 1);
            DrawTextEx(arialFont, slot.component.nome.c_str(), 
                      {slot.bounds.x + (slot.bounds.width - nameSize.x) / 2, 
                       slot.bounds.y + slot.bounds.height / 2 - 5}, 10, 1, WHITE);
        } else {
            Vector2 labelSize = MeasureTextEx(arialFont, slot.label.c_str(), 12, 1);
            DrawTextEx(arialFont, slot.label.c_str(), 
                      {slot.bounds.x + (slot.bounds.width - labelSize.x) / 2, 
                       slot.bounds.y + slot.bounds.height / 2 - 6}, 12, 1, GRAY);
        }
    }
    
    DrawTextEx(arialFont, "Componentes Disponiveis:", {350, 180}, 20, 1, WHITE);
    
    Vector2 mousePos = GetMousePosition();
    int listY = 220;
    
    for (size_t i = 0; i < buildAtual.size(); i++) {
        Component &c = buildAtual[i];
        Rectangle compRect = {350, (float)listY, 400, 50};
        
        bool isHovered = CheckCollisionPointRec(mousePos, compRect);
        Color bgColor = isHovered ? Fade(RED, 0.3f) : Fade(WHITE, 0.1f);
        
        DrawRectangleRounded(compRect, 0.1f, 20, bgColor);
        DrawRectangleRoundedLines(compRect, 0.1f, 20, isHovered ? RED : GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s]", c.tipo.c_str()), {360, (float)(listY + 5)}, 14, 1, RED);
        DrawTextEx(arialFont, c.nome.c_str(), {360, (float)(listY + 25)}, 14, 1, WHITE);
        
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            draggedComponent = &c;
            dragOffset = {mousePos.x - compRect.x, mousePos.y - compRect.y};
        }
        
        listY += 60;
    }
    
    if (draggedComponent != nullptr) {
        Rectangle dragRect = {mousePos.x - dragOffset.x, mousePos.y - dragOffset.y, 400, 50};
        DrawRectangleRounded(dragRect, 0.1f, 20, Fade(YELLOW, 0.5f));
        DrawRectangleRoundedLines(dragRect, 0.1f, 20, YELLOW);
        DrawTextEx(arialFont, draggedComponent->nome.c_str(), 
                  {dragRect.x + 10, dragRect.y + 15}, 14, 1, WHITE);
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            bool placed = false;
            for (auto &slot : pcSlots) {
                if (slot.tipo == "Case") continue;
                
                if (CheckCollisionPointRec(mousePos, slot.bounds)) {
                    if (slot.tipo == draggedComponent->tipo || 
                        (slot.tipo == "Storage" && (draggedComponent->tipo == "SSD" || draggedComponent->tipo == "HDD"))) {
                        slot.filled = true;
                        slot.component = *draggedComponent;
                        
                        for (size_t j = 0; j < buildAtual.size(); j++) {
                            if (&buildAtual[j] == draggedComponent) {
                                buildAtual.erase(buildAtual.begin() + j);
                                break;
                            }
                        }
                        
                        placed = true;
                        feedbackMessage = "Componente instalado!";
                        feedbackTimer = 120;
                        break;
                    }
                }
            }
            
            if (!placed) {
                feedbackMessage = "Coloque no slot correto!";
                feedbackTimer = 120;
            }
            
            draggedComponent = nullptr;
        }
    }
    
    Rectangle clearBtn = {800, (float)(GetScreenHeight() - 80), 150, 40};
    if (DrawButton(clearBtn, "Limpar Tudo", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        for (auto &slot : pcSlots) {
            if (slot.filled) {
                buildAtual.push_back(slot.component);
                slot.filled = false;
            }
        }
        feedbackMessage = "Build limpa!";
        feedbackTimer = 120;
    }
    
    int totalSlots = 0, filledSlots = 0;
    for (auto &slot : pcSlots) {
        if (slot.tipo != "Case") {
            totalSlots++;
            if (slot.filled) filledSlots++;
        }
    }
    
    DrawTextEx(arialFont, TextFormat("Progresso: %d/%d componentes instalados", filledSlots, totalSlots),
               {350.0f, (float)(GetScreenHeight() - 80)}, 18, 1, filledSlots == totalSlots ? GREEN : YELLOW);
    
    if (feedbackTimer > 0) {
        Color msgColor = (feedbackMessage.find("instalado") != string::npos || 
                         feedbackMessage.find("limpa") != string::npos) ? GREEN : RED;
        Vector2 msgSize = MeasureTextEx(arialFont, feedbackMessage.c_str(), 20, 1);
        DrawTextEx(arialFont, feedbackMessage.c_str(), 
                  {(GetScreenWidth() - msgSize.x) / 2, 150}, 20, 1, msgColor);
        feedbackTimer--;
    }
}

void drawAdminPanelScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "PAINEL DE ADMINISTRACAO", {50, 120}, 32, 1, RED);
    
    Rectangle addBtn = {50, 180, 200, 50};
    Rectangle editBtn = {270, 180, 200, 50};
    Rectangle deleteBtn = {490, 180, 200, 50};
    
    if (DrawButton(addBtn, "Adicionar", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
        currentScreen = ADMIN_ADD;
        adminId = adminTipo = adminNome = adminSpecs = adminPreco = adminMessage = "";
        adminFocusField = 0;
    }
    
    if (DrawButton(editBtn, "Editar", {0, 0, 100, 255}, {0, 0, 140, 255}, WHITE)) {
        currentScreen = ADMIN_EDIT;
        adminSelectedIndex = -1;
        adminMessage = "";
    }
    
    if (DrawButton(deleteBtn, "Eliminar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        currentScreen = ADMIN_DELETE;
        adminSelectedIndex = -1;
        adminMessage = "";
    }
    
    DrawTextEx(arialFont, TextFormat("Total de componentes: %d", (int)catalog.size()), 
               {50, 260}, 20, 1, WHITE);
    
    int listY = 300 - adminScrollOffset;
    int visibleCount = 0;
    int maxVisible = 8;
    
    DrawTextEx(arialFont, "Catalogo de Componentes:", {50, 280}, 18, 1, GRAY);
    
    for (size_t i = 0; i < catalog.size() && visibleCount < maxVisible; i++) {
        if (listY < 300) {
            listY += 60;
            continue;
        }
        if (listY > GetScreenHeight() - 100) break;
        
        Component &c = catalog[i];
        Rectangle itemRect = {50, (float)listY, (float)(GetScreenWidth() - 100), 50};
        
        DrawRectangleRounded(itemRect, 0.1f, 20, Fade(WHITE, 0.05f));
        DrawRectangleRoundedLines(itemRect, 0.1f, 20, GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s] %s", c.tipo.c_str(), c.nome.c_str()), 
                  {60, (float)(listY + 10)}, 16, 1, WHITE);
        DrawTextEx(arialFont, TextFormat("%s - %s", c.specs.c_str(), formatBR(c.preco).c_str()), 
                  {60, (float)(listY + 30)}, 14, 1, GRAY);
        
        listY += 60;
        visibleCount++;
    }
    
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        adminScrollOffset -= (int)(wheelMove * 30);
        if (adminScrollOffset < 0) adminScrollOffset = 0;
        int maxScroll = max(0, (int)(catalog.size() * 60) - (GetScreenHeight() - 400));
        if (adminScrollOffset > maxScroll) adminScrollOffset = maxScroll;
    }
}

void drawAdminAddScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = ADMIN_PANEL;
    }
    
    DrawTextEx(arialFont, "ADICIONAR COMPONENTE", {50, 120}, 32, 1, RED);
    
    Rectangle idBox = {50, 200, 400, 40};
    Rectangle tipoBox = {50, 270, 400, 40};
    Rectangle nomeBox = {50, 340, 400, 40};
    Rectangle specsBox = {50, 410, 400, 40};
    Rectangle precoBox = {50, 480, 400, 40};
    
    drawTextBox("ID:", adminId, idBox, adminFocusField == 0);
    drawTextBox("Tipo (CPU/GPU/RAM/etc):", adminTipo, tipoBox, adminFocusField == 1);
    drawTextBox("Nome:", adminNome, nomeBox, adminFocusField == 2);
    drawTextBox("Especificacoes:", adminSpecs, specsBox, adminFocusField == 3);
    drawTextBox("Preco (€):", adminPreco, precoBox, adminFocusField == 4);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, idBox)) adminFocusField = 0;
        else if (CheckCollisionPointRec(mp, tipoBox)) adminFocusField = 1;
        else if (CheckCollisionPointRec(mp, nomeBox)) adminFocusField = 2;
        else if (CheckCollisionPointRec(mp, specsBox)) adminFocusField = 3;
        else if (CheckCollisionPointRec(mp, precoBox)) adminFocusField = 4;
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        adminFocusField = (adminFocusField + 1) % 5;
    }
    
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125) {
            if (adminFocusField == 0) adminId += (char)key;
            else if (adminFocusField == 1) adminTipo += (char)key;
            else if (adminFocusField == 2) adminNome += (char)key;
            else if (adminFocusField == 3) adminSpecs += (char)key;
            else if (adminFocusField == 4) adminPreco += (char)key;
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (adminFocusField == 0 && !adminId.empty()) adminId.pop_back();
        else if (adminFocusField == 1 && !adminTipo.empty()) adminTipo.pop_back();
        else if (adminFocusField == 2 && !adminNome.empty()) adminNome.pop_back();
        else if (adminFocusField == 3 && !adminSpecs.empty()) adminSpecs.pop_back();
        else if (adminFocusField == 4 && !adminPreco.empty()) adminPreco.pop_back();
    }
    
    Rectangle addBtn = {50, 550, 200, 50};
    if (DrawButton(addBtn, "Adicionar", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
        if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
            adminMessage = "Preencha todos os campos!";
        } else {
            try {
                double preco = stod(adminPreco);
                Component novo;
                novo.id = adminId;
                novo.tipo = adminTipo;
                novo.nome = adminNome;
                novo.specs = adminSpecs;
                novo.preco = preco;
                catalog.push_back(novo);
                saveComponents();
                adminMessage = "Componente adicionado!";
                adminId = adminTipo = adminNome = adminSpecs = adminPreco = "";
            } catch(...) {
                adminMessage = "Preco invalido!";
            }
        }
    }
    
    if (!adminMessage.empty()) {
        Color msgColor = (adminMessage.find("adicionado") != string::npos) ? GREEN : RED;
        DrawTextEx(arialFont, adminMessage.c_str(), {50, 620}, 18, 1, msgColor);
    }
}

void drawAdminEditScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = ADMIN_PANEL;
    }
    
    DrawTextEx(arialFont, "EDITAR COMPONENTE", {50, 120}, 32, 1, RED);
    
    if (adminSelectedIndex == -1) {
        DrawTextEx(arialFont, "Selecione um componente:", {50, 180}, 20, 1, WHITE);
        
        int listY = 220 - adminScrollOffset;
        Vector2 mp = GetMousePosition();
        
        for (size_t i = 0; i < catalog.size(); i++) {
            if (listY < 220 || listY > GetScreenHeight() - 100) {
                listY += 60;
                continue;
            }
            
            Rectangle itemRect = {50, (float)listY, (float)(GetScreenWidth() - 100), 50};
            bool isHovered = CheckCollisionPointRec(mp, itemRect);
            
            DrawRectangleRounded(itemRect, 0.1f, 20, isHovered ? Fade(BLUE, 0.3f) : Fade(WHITE, 0.05f));
            DrawRectangleRoundedLines(itemRect, 0.1f, 20, isHovered ? BLUE : GRAY);
            
            DrawTextEx(arialFont, TextFormat("[%s] %s", catalog[i].tipo.c_str(), catalog[i].nome.c_str()), 
                      {60, (float)(listY + 10)}, 16, 1, WHITE);
            DrawTextEx(arialFont, TextFormat("%s - %s", catalog[i].specs.c_str(), formatBR(catalog[i].preco).c_str()), 
                      {60, (float)(listY + 30)}, 14, 1, GRAY);
            
            if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                adminSelectedIndex = (int)i;
                adminId = catalog[i].id;
                adminTipo = catalog[i].tipo;
                adminNome = catalog[i].nome;
                adminSpecs = catalog[i].specs;
                adminPreco = to_string(catalog[i].preco);
                adminFocusField = 0;
            }
            
            listY += 60;
        }
        
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0) {
            adminScrollOffset -= (int)(wheelMove * 30);
            if (adminScrollOffset < 0) adminScrollOffset = 0;
            int maxScroll = max(0, (int)(catalog.size() * 60) - (GetScreenHeight() - 320));
            if (adminScrollOffset > maxScroll) adminScrollOffset = maxScroll;
        }
    } else {
        Rectangle idBox = {50, 200, 400, 40};
        Rectangle tipoBox = {50, 270, 400, 40};
        Rectangle nomeBox = {50, 340, 400, 40};
        Rectangle specsBox = {50, 410, 400, 40};
        Rectangle precoBox = {50, 480, 400, 40};
        
        drawTextBox("ID:", adminId, idBox, adminFocusField == 0);
        drawTextBox("Tipo:", adminTipo, tipoBox, adminFocusField == 1);
        drawTextBox("Nome:", adminNome, nomeBox, adminFocusField == 2);
        drawTextBox("Especificacoes:", adminSpecs, specsBox, adminFocusField == 3);
        drawTextBox("Preco (€):", adminPreco, precoBox, adminFocusField == 4);
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            if (CheckCollisionPointRec(mp, idBox)) adminFocusField = 0;
            else if (CheckCollisionPointRec(mp, tipoBox)) adminFocusField = 1;
            else if (CheckCollisionPointRec(mp, nomeBox)) adminFocusField = 2;
            else if (CheckCollisionPointRec(mp, specsBox)) adminFocusField = 3;
            else if (CheckCollisionPointRec(mp, precoBox)) adminFocusField = 4;
        }
        
        if (IsKeyPressed(KEY_TAB)) {
            adminFocusField = (adminFocusField + 1) % 5;
        }
        
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125) {
                if (adminFocusField == 0) adminId += (char)key;
                else if (adminFocusField == 1) adminTipo += (char)key;
                else if (adminFocusField == 2) adminNome += (char)key;
                else if (adminFocusField == 3) adminSpecs += (char)key;
                else if (adminFocusField == 4) adminPreco += (char)key;
            }
            key = GetCharPressed();
        }
        
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (adminFocusField == 0 && !adminId.empty()) adminId.pop_back();
            else if (adminFocusField == 1 && !adminTipo.empty()) adminTipo.pop_back();
            else if (adminFocusField == 2 && !adminNome.empty()) adminNome.pop_back();
            else if (adminFocusField == 3 && !adminSpecs.empty()) adminSpecs.pop_back();
            else if (adminFocusField == 4 && !adminPreco.empty()) adminPreco.pop_back();
        }
        
        Rectangle saveBtn = {50, 550, 200, 50};
        Rectangle cancelBtn = {270, 550, 200, 50};
        
        if (DrawButton(saveBtn, "Guardar", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
            if (adminId.empty() || adminTipo.empty() || adminNome.empty() || adminPreco.empty()) {
                adminMessage = "Preencha todos os campos!";
            } else {
                try {
                    double preco = stod(adminPreco);
                    catalog[adminSelectedIndex].id = adminId;
                    catalog[adminSelectedIndex].tipo = adminTipo;
                    catalog[adminSelectedIndex].nome = adminNome;
                    catalog[adminSelectedIndex].specs = adminSpecs;
                    catalog[adminSelectedIndex].preco = preco;
                    saveComponents();
                    adminMessage = "Componente atualizado!";
                    adminSelectedIndex = -1;
                } catch(...) {
                    adminMessage = "Preco invalido!";
                }
            }
        }
        
        if (DrawButton(cancelBtn, "Cancelar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
            adminSelectedIndex = -1;
            adminMessage = "";
        }
        
        if (!adminMessage.empty()) {
            Color msgColor = (adminMessage.find("atualizado") != string::npos) ? GREEN : RED;
            DrawTextEx(arialFont, adminMessage.c_str(), {50, 620}, 18, 1, msgColor);
        }
    }
}

void drawAdminDeleteScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = ADMIN_PANEL;
    }
    
    DrawTextEx(arialFont, "ELIMINAR COMPONENTE", {50, 120}, 32, 1, RED);
    DrawTextEx(arialFont, "Selecione um componente para eliminar:", {50, 180}, 20, 1, WHITE);
    
    int listY = 220 - adminScrollOffset;
    Vector2 mp = GetMousePosition();
    
    for (size_t i = 0; i < catalog.size(); i++) {
        if (listY < 220 || listY > GetScreenHeight() - 150) {
            listY += 60;
            continue;
        }
        
        Rectangle itemRect = {50, (float)listY, (float)(GetScreenWidth() - 100), 50};
        bool isHovered = CheckCollisionPointRec(mp, itemRect);
        
        DrawRectangleRounded(itemRect, 0.1f, 20, isHovered ? Fade(RED, 0.3f) : Fade(WHITE, 0.05f));
        DrawRectangleRoundedLines(itemRect, 0.1f, 20, isHovered ? RED : GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s] %s", catalog[i].tipo.c_str(), catalog[i].nome.c_str()), 
                  {60, (float)(listY + 10)}, 16, 1, WHITE);
        DrawTextEx(arialFont, TextFormat("%s - %s", catalog[i].specs.c_str(), formatBR(catalog[i].preco).c_str()), 
                  {60, (float)(listY + 30)}, 14, 1, GRAY);
        
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            adminSelectedIndex = (int)i;
        }
        
        listY += 60;
    }
    
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        adminScrollOffset -= (int)(wheelMove * 30);
        if (adminScrollOffset < 0) adminScrollOffset = 0;
        int maxScroll = max(0, (int)(catalog.size() * 60) - (GetScreenHeight() - 370));
        if (adminScrollOffset > maxScroll) adminScrollOffset = maxScroll;
    }
    
    if (adminSelectedIndex >= 0 && adminSelectedIndex < (int)catalog.size()) {
        DrawRectangle(0, GetScreenHeight() - 120, GetScreenWidth(), 120, {20, 20, 20, 255});
        DrawLine(0, GetScreenHeight() - 120, GetScreenWidth(), GetScreenHeight() - 120, RED);
        
        DrawTextEx(arialFont, TextFormat("Eliminar: %s?", catalog[adminSelectedIndex].nome.c_str()), 
                  {50, (float)(GetScreenHeight() - 90)}, 20, 1, WHITE);
        
        Rectangle confirmBtn = {50, (float)(GetScreenHeight() - 50), 200, 40};
        Rectangle cancelBtn = {270, (float)(GetScreenHeight() - 50), 200, 40};
        
        if (DrawButton(confirmBtn, "Confirmar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
            catalog.erase(catalog.begin() + adminSelectedIndex);
            saveComponents();
            adminMessage = "Componente eliminado!";
            adminSelectedIndex = -1;
        }
        
        if (DrawButton(cancelBtn, "Cancelar", {40, 40, 40, 255}, {60, 60, 60, 255}, WHITE)) {
            adminSelectedIndex = -1;
        }
    }
    
    if (!adminMessage.empty()) {
        DrawTextEx(arialFont, adminMessage.c_str(), {50, 160}, 18, 1, GREEN);
    }
}

void drawEscolherPecasScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = MENU;
    }
    
    DrawTextEx(arialFont, "ESCOLHER PECAS", {50, 120}, 32, 1, RED);
    
    vector<string> categorias = {"Todos", "CPU", "GPU", "RAM", "Motherboard", "SSD", "HDD", "PSU", "Case", "Cooler"};
    int catX = 50;
    for (const auto &cat : categorias) {
        Rectangle catBtn = {(float)catX, 170, 100, 35};
        Color btnColor = (filtroCategoria == cat) ? Color{100, 0, 0, 255} : Color{40, 40, 40, 255};
        Color hoverColor = (filtroCategoria == cat) ? Color{140, 0, 0, 255} : Color{60, 0, 0, 255};
        
        if (DrawButton(catBtn, cat.c_str(), btnColor, hoverColor, WHITE)) {
            filtroCategoria = cat;
            pecasScrollOffset = 0;
        }
        catX += 110;
    }
    
    int listY = 230 - pecasScrollOffset;
    int visibleCount = 0;
    int maxVisible = 8;
    
    for (auto &comp : catalog) {
        if (filtroCategoria != "Todos" && comp.tipo != filtroCategoria) continue;
        
        if (listY < 230) {
            listY += 80;
            continue;
        }
        if (listY > GetScreenHeight() - 100) break;
        
        Rectangle compRect = {50, (float)listY, (float)(GetScreenWidth() - 100), 70};
        Vector2 mp = GetMousePosition();
        bool isHovered = CheckCollisionPointRec(mp, compRect);
        
        DrawRectangleRounded(compRect, 0.1f, 20, isHovered ? Fade(RED, 0.2f) : Fade(WHITE, 0.05f));
        DrawRectangleRoundedLines(compRect, 0.1f, 20, isHovered ? RED : GRAY);
        
        DrawTextEx(arialFont, TextFormat("[%s]", comp.tipo.c_str()), {60, (float)(listY + 10)}, 14, 1, RED);
        DrawTextEx(arialFont, comp.nome.c_str(), {60, (float)(listY + 30)}, 18, 1, WHITE);
        DrawTextEx(arialFont, comp.specs.c_str(), {350, (float)(listY + 30)}, 14, 1, GRAY);
        
        string precoStr = formatBR(comp.preco);
        DrawTextEx(arialFont, precoStr.c_str(), {(float)(GetScreenWidth() - 250), (float)(listY + 25)}, 20, 1, GREEN);
        
        Rectangle addBtn = {(float)(GetScreenWidth() - 150), (float)(listY + 15), 100, 40};
        if (DrawButton(addBtn, "+ Add", {0, 80, 0, 255}, {0, 120, 0, 255}, WHITE)) {
            buildAtual.push_back(comp);
            feedbackMessage = "Adicionado: " + comp.nome;
            feedbackTimer = 120;
        }
        
        listY += 80;
        visibleCount++;
    }
    
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        pecasScrollOffset -= (int)(wheelMove * 40);
        if (pecasScrollOffset < 0) pecasScrollOffset = 0;
    }
    
    if (feedbackTimer > 0) {
        Vector2 msgSize = MeasureTextEx(arialFont, feedbackMessage.c_str(), 18, 1);
        DrawRectangle((GetScreenWidth() - (int)msgSize.x - 40) / 2, 90, (int)msgSize.x + 40, 40, Fade(GREEN, 0.8f));
        DrawTextEx(arialFont, feedbackMessage.c_str(), 
                  {(GetScreenWidth() - msgSize.x) / 2, 100}, 18, 1, WHITE);
        feedbackTimer--;
    }
    
    DrawRectangle(0, GetScreenHeight() - 60, GetScreenWidth(), 60, {20, 20, 20, 255});
    DrawLine(0, GetScreenHeight() - 60, GetScreenWidth(), GetScreenHeight() - 60, RED);
    DrawTextEx(arialFont, TextFormat("Pecas selecionadas: %d | Total: %s", 
                                     (int)buildAtual.size(), formatBR(calcularTotal()).c_str()),
               {50, (float)(GetScreenHeight() - 40)}, 20, 1, WHITE);
}

void drawLoginScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawWolfLogo(getCenterX() - 60, 100, 120);
    
    if (titleImage.id != 0) {
        float scale = 200.0f / (float)titleImage.width;
        float drawW = (float)titleImage.width * scale;
        float drawH = (float)titleImage.height * scale;
        DrawTextureEx(titleImage, {getCenterX() - drawW / 2, 240}, 0.0f, scale, WHITE);
    } else {
        Vector2 buildSize = MeasureTextEx(arialFont, "Build", 48, 1);
        DrawTextEx(arialFont, "Build", {getCenterX() - buildSize.x / 2, 240}, 48, 1, RED);
        Vector2 compSize = MeasureTextEx(arialFont, "Computer", 48, 1);
        DrawTextEx(arialFont, "Computer", {getCenterX() - compSize.x / 2, 290}, 48, 1, WHITE);
    }
    
    Rectangle userBox = {getCenterX() - 200, 380, 400, 50};
    Rectangle passBox = {getCenterX() - 200, 450, 400, 50};
    
    drawTextBox("Username:", loginUser, userBox, loginFocusUser, false);
    drawTextBox("Password:", loginPass, passBox, !loginFocusUser, true);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, userBox)) loginFocusUser = true;
        else if (CheckCollisionPointRec(mp, passBox)) loginFocusUser = false;
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        loginFocusUser = !loginFocusUser;
    }
    
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125) {
            if (loginFocusUser) loginUser += (char)key;
            else loginPass += (char)key;
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (loginFocusUser && !loginUser.empty()) loginUser.pop_back();
        else if (!loginFocusUser && !loginPass.empty()) loginPass.pop_back();
    }
    
    Rectangle loginBtn = {getCenterX() - 200, 530, 400, 50};
    if (DrawButton(loginBtn, "Entrar", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE) || 
        (!loginFocusUser && IsKeyPressed(KEY_ENTER))) {
        bool found = false;
        for (auto &acc : accounts) {
            if (acc.username == loginUser && acc.password == loginPass) {
                currentUser = acc.username;
                isCurrentUserAdmin = acc.isAdmin;
                currentScreen = MENU;
                found = true;
                loginUser = loginPass = "";
                break;
            }
        }
        if (!found) {
            feedbackMessage = "Credenciais invalidas!";
            feedbackTimer = 180;
        }
    }
    
    Rectangle registerBtn = {getCenterX() - 200, 600, 400, 50};
    if (DrawButton(registerBtn, "Registrar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = REGISTER;
        registerUser = registerPass = registerPassConfirm = registerEmail = registerPhone = registerMessage = "";
        registerFocusField = 0;
    }
    
    if (feedbackTimer > 0) {
        Vector2 msgSize = MeasureTextEx(arialFont, feedbackMessage.c_str(), 18, 1);
        DrawTextEx(arialFont, feedbackMessage.c_str(), 
                  {getCenterX() - msgSize.x / 2, 680}, 18, 1, RED);
        feedbackTimer--;
    }
}

void drawRegisterScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
DrawTextEx(arialFont, "REGISTRAR CONTA", {static_cast<float>(getCenterX() - 150), 80.0f}, 36, 1, RED);
    
    Rectangle userBox = {getCenterX() - 200, 150, 400, 45};
    Rectangle passBox = {getCenterX() - 200, 220, 400, 45};
    Rectangle passConfBox = {getCenterX() - 200, 290, 400, 45};
    Rectangle emailBox = {getCenterX() - 200, 360, 400, 45};
    Rectangle phoneBox = {getCenterX() - 200, 430, 400, 45};
    
    drawTextBox("Username:", registerUser, userBox, registerFocusField == 0);
    drawTextBox("Password:", registerPass, passBox, registerFocusField == 1, true);
    drawTextBox("Confirmar Password:", registerPassConfirm, passConfBox, registerFocusField == 2, true);
    drawTextBox("Email:", registerEmail, emailBox, registerFocusField == 3);
    drawTextBox("Telefone:", registerPhone, phoneBox, registerFocusField == 4);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, userBox)) registerFocusField = 0;
        else if (CheckCollisionPointRec(mp, passBox)) registerFocusField = 1;
        else if (CheckCollisionPointRec(mp, passConfBox)) registerFocusField = 2;
        else if (CheckCollisionPointRec(mp, emailBox)) registerFocusField = 3;
        else if (CheckCollisionPointRec(mp, phoneBox)) registerFocusField = 4;
    }
    
    if (IsKeyPressed(KEY_TAB)) {
        registerFocusField = (registerFocusField + 1) % 5;
    }
    
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125) {
            if (registerFocusField == 0) registerUser += (char)key;
            else if (registerFocusField == 1) registerPass += (char)key;
            else if (registerFocusField == 2) registerPassConfirm += (char)key;
            else if (registerFocusField == 3) registerEmail += (char)key;
            else if (registerFocusField == 4) registerPhone += (char)key;
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (registerFocusField == 0 && !registerUser.empty()) registerUser.pop_back();
        else if (registerFocusField == 1 && !registerPass.empty()) registerPass.pop_back();
        else if (registerFocusField == 2 && !registerPassConfirm.empty()) registerPassConfirm.pop_back();
        else if (registerFocusField == 3 && !registerEmail.empty()) registerEmail.pop_back();
        else if (registerFocusField == 4 && !registerPhone.empty()) registerPhone.pop_back();
    }
    
    Rectangle registerBtn = {getCenterX() - 200, 510, 400, 50};
    if (DrawButton(registerBtn, "Criar Conta", {0, 100, 0, 255}, {0, 140, 0, 255}, WHITE)) {
        if (registerUser.empty() || registerPass.empty() || registerEmail.empty() || registerPhone.empty()) {
            registerMessage = "Preencha todos os campos!";
        } else if (registerPass != registerPassConfirm) {
            registerMessage = "As passwords nao coincidem!";
        } else if (!validateEmail(registerEmail)) {
            registerMessage = "Email invalido!";
        } else if (!validatePhone(registerPhone)) {
            registerMessage = "Telefone invalido!";
        } else {
            bool userExists = false;
            for (auto &acc : accounts) {
                if (acc.username == registerUser) {
                    userExists = true;
                    break;
                }
            }
            
            if (userExists) {
                registerMessage = "Username ja existe!";
            } else {
                Account newAcc;
                newAcc.username = registerUser;
                newAcc.password = registerPass;
                newAcc.email = registerEmail;
                newAcc.phone = registerPhone;
                newAcc.isAdmin = false;
                accounts.push_back(newAcc);
                saveAccounts();
                
                registerMessage = "Conta criada com sucesso!";
                feedbackTimer = 180;
                currentScreen = LOGIN;
            }
        }
    }
    
    Rectangle backBtn = {getCenterX() - 200, 580, 400, 50};
    if (DrawButton(backBtn, "← Voltar", {40, 40, 40, 255}, {60, 0, 0, 255}, WHITE)) {
        currentScreen = LOGIN;
    }
    
    if (!registerMessage.empty()) {
        Color msgColor = (registerMessage.find("sucesso") != string::npos) ? GREEN : RED;
        Vector2 msgSize = MeasureTextEx(arialFont, registerMessage.c_str(), 18, 1);
        DrawTextEx(arialFont, registerMessage.c_str(), 
                  {getCenterX() - msgSize.x / 2, 660}, 18, 1, msgColor);
    }
}

void drawMenuScreen() {
    Color gradTop = {10, 10, 10, 255};
    Color gradBottom = {40, 0, 0, 255};
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), gradTop, gradBottom);
    
    DrawNavBar();
    
    Rectangle exitBtn = {(float)(GetScreenWidth() - 120), 25, 100, 35};
    if (DrawButton(exitBtn, "Sair", {100, 0, 0, 255}, {140, 0, 0, 255}, WHITE)) {
        currentScreen = LOGIN;
        currentUser = "";
        isCurrentUserAdmin = false;
        buildAtual.clear();
    }
    
    string welcomeText = "Bem-vindo, " + currentUser + "!";
    Vector2 welcomeSize = MeasureTextEx(arialFont, welcomeText.c_str(), 28, 1);
    DrawTextEx(arialFont, welcomeText.c_str(), {getCenterX() - welcomeSize.x / 2, 120}, 28, 1, WHITE);
    
    if (isCurrentUserAdmin) {
        Vector2 adminSize = MeasureTextEx(arialFont, "[Administrador]", 18, 1);
        DrawTextEx(arialFont, "[Administrador]", {getCenterX() - adminSize.x / 2, 155}, 18, 1, RED);
    }
    
    Vector2 mousePos = GetMousePosition();
    
    for (auto &card : menuCards) {
        if (card.targetScreen == ADMIN_PANEL && !isCurrentUserAdmin) continue;
        
        DrawCard(card);
        
        Rectangle btnBounds = {
            card.bounds.x + 20,
            card.bounds.y + card.bounds.height - 60,
            card.bounds.width - 40,
            40
        };
        
        if (CheckCollisionPointRec(mousePos, btnBounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            currentScreen = (Screen)card.targetScreen;
        }
    }
}

// ==================== MAIN ====================

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "Build Computer - PC Builder");
    SetTargetFPS(60);
    
    arialFont = LoadFontEx("resources/arial.ttf", 32, 0, 0);
    if (arialFont.texture.id == 0) {
        arialFont = GetFontDefault();
    }
    
    titleImage = LoadTexture("resources/title.png");
    
    loadAccounts();
    
    if (!loadComponents("componentes.realyb")) {
        catalog.push_back({"CPU001", "CPU", "Intel i9-13900K", "24 cores, 5.8GHz", 589.99});
        catalog.push_back({"CPU002", "CPU", "AMD Ryzen 9 7950X", "16 cores, 5.7GHz", 549.99});
        catalog.push_back({"GPU001", "GPU", "NVIDIA RTX 4090", "24GB GDDR6X", 1599.99});
        catalog.push_back({"GPU002", "GPU", "AMD RX 7900 XTX", "24GB GDDR6", 999.99});
        catalog.push_back({"RAM001", "RAM", "Corsair Vengeance 32GB", "DDR5 6000MHz", 149.99});
        catalog.push_back({"MB001", "Motherboard", "ASUS ROG STRIX Z790", "ATX, PCIe 5.0", 399.99});
        catalog.push_back({"SSD001", "SSD", "Samsung 990 Pro 2TB", "NVMe, 7450MB/s", 199.99});
        catalog.push_back({"HDD001", "HDD", "Seagate 4TB", "7200RPM", 89.99});
        catalog.push_back({"PSU001", "PSU", "Corsair RM1000x", "1000W, 80+ Gold", 179.99});
        catalog.push_back({"CASE001", "Case", "NZXT H510", "ATX Mid Tower", 89.99});
        catalog.push_back({"COOL001", "Cooler", "Noctua NH-D15", "Dual Tower", 99.99});
        saveComponents();
    }
    
    initPCSlots();
    
    menuCards.clear();
    menuCards.push_back(Card(
        {100, 250, 250, 300},
        "Escolher Pecas",
        "Monte seu PC ideal",
        RED, false, ESCOLHER_PECAS,
        "resources/icon_parts.png"
    ));
    menuCards.push_back(Card(
        {400, 250, 250, 300},
        "Ver Precos",
        "Veja o custo total",
        BLUE, false, VER_PRECOS,
        "resources/icon_price.png"
    ));
    menuCards.push_back(Card(
        {700, 250, 250, 300},
        "Como Montar",
        "Guia de montagem",
        GREEN, false, COMO_MONTAR,
        "resources/icon_build.png"
    ));
    menuCards.push_back(Card(
        {1000, 250, 250, 300},
        "Admin Panel",
        "Gerir componentes",
        ORANGE, false, ADMIN_PANEL,
        "resources/icon_admin.png"
    ));
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        
        switch (currentScreen) {
            case LOGIN: drawLoginScreen(); break;
            case REGISTER: drawRegisterScreen(); break;
            case MENU: drawMenuScreen(); break;
            case ESCOLHER_PECAS: drawEscolherPecasScreen(); break;
            case VER_PRECOS: drawVerPrecosScreen(); break;
            case COMO_MONTAR: drawComoMontarScreen(); break;
            case ADMIN_PANEL: drawAdminPanelScreen(); break;
            case ADMIN_ADD: drawAdminAddScreen(); break;
            case ADMIN_EDIT: drawAdminEditScreen(); break;
            case ADMIN_DELETE: drawAdminDeleteScreen(); break;
        }
        
        EndDrawing();
    }
    
    for (auto &card : menuCards) {
        if (card.iconLoaded && card.iconTexture.id != 0) {
            UnloadTexture(card.iconTexture);
        }
    }
    
    if (titleImage.id != 0) UnloadTexture(titleImage);
    if (arialFont.texture.id != 0 && arialFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(arialFont);
    }
    
    CloseWindow();
    return 0;
}