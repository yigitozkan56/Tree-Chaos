#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define M_PI 3.14159265358979323846f

struct Vector3 {
    float x, y, z;
};

inline Vector3 Add(const Vector3& a, const Vector3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vector3 Sub(const Vector3& a, const Vector3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vector3 Scale(const Vector3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline float LengthSq(const Vector3& a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
inline float Length(const Vector3& a) { return std::sqrt(LengthSq(a)); }

inline Vector3 Normalize(const Vector3& a) {
    float len = Length(a);
    if (len < 0.00001f) return { 0, 0, 0 };
    return { a.x / len, a.y / len, a.z / len };
}

inline Vector3 Cross(const Vector3& a, const Vector3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

struct Dal3D {
    Vector3 p1, p2;
    Vector3 dir;
    float uzunluk;
    int tur;
    int ebeveyn_idx;
};

const int GENISLIK = 1280;
const int YUKSEKLIK = 720;
const int GRID_BOYUTU = 3;

uint32_t pixels[GENISLIK * YUKSEKLIK];

std::vector<Dal3D> tumDallar;
std::vector<int> aktifDalIndeksleri;
std::unordered_map<uint64_t, std::vector<int>> izgara3D;

Vector3 camPos = { 0.0f, 0.0f, -45.0f };
float camYaw = 0.0f;
float camPitch = 0.0f;

bool draggingRot = false;
POINT lastMousePos;

int mevcutTur = 1;
int sonUretilenDal = 2;
bool otomatikOynat = false;
int secilenDalIdx = -1;

bool ayarlarAcik = true;
bool dugumleriGoster = true;
int renkPaleti = 0;
float dagilmaOrani = 0.50f;
int kisalmaAraligi = 15;
float kisalmaOrani = 0.50f;
float kokUzunlugu = 8.0f;

uint32_t HSVtoRGB(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;

    if (h < 60) { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else { r = c; b = x; }

    uint8_t R = (uint8_t)((r + m) * 255);
    uint8_t G = (uint8_t)((g + m) * 255);
    uint8_t B = (uint8_t)((b + m) * 255);

    return (R << 16) | (G << 8) | B;
}

uint32_t GetDalRengi(int tur, int palet) {
    float t = (float)tur;
    float tonVaryasyon = 0.15f * std::sin(t * 0.4f) + 0.1f * std::cos(t * 0.8f);

    switch (palet) {
    case 0: {
        float hue = std::fmod(t * 14.0f, 360.0f);
        float sat = std::clamp(0.80f + tonVaryasyon, 0.50f, 1.0f);
        float val = std::clamp(0.85f + 0.15f * std::sin(t * 0.3f), 0.40f, 1.0f);
        return HSVtoRGB(hue, sat, val);
    }
    case 1: {
        float hue = 175.0f + std::fmod(t * 4.5f, 55.0f);
        float sat = std::clamp(0.85f + tonVaryasyon, 0.40f, 1.0f);
        float val = std::clamp(0.40f + 0.60f * ((tur % 7) / 7.0f), 0.30f, 1.0f);
        return HSVtoRGB(hue, sat, val);
    }
    case 2: {
        float hue = std::fmod(t * 3.5f, 42.0f);
        float sat = std::clamp(0.90f + tonVaryasyon, 0.60f, 1.0f);
        float val = std::clamp(0.35f + 0.65f * ((tur % 6) / 6.0f), 0.35f, 1.0f);
        return HSVtoRGB(hue, sat, val);
    }
    case 3: {
        float hue = 80.0f + std::fmod(t * 5.0f, 65.0f);
        float sat = std::clamp(0.80f + tonVaryasyon, 0.45f, 1.0f);
        float val = std::clamp(0.30f + 0.70f * ((tur % 5) / 5.0f), 0.25f, 1.0f);
        return HSVtoRGB(hue, sat, val);
    }
    case 4: {
        float hue = 265.0f + std::fmod(t * 6.0f, 65.0f);
        float sat = std::clamp(0.88f + tonVaryasyon, 0.50f, 1.0f);
        float val = std::clamp(0.45f + 0.55f * std::sin(t * 0.5f), 0.35f, 1.0f);
        return HSVtoRGB(hue, sat, val);
    }
    default:
        return 0x00FFFF;
    }
}

inline uint64_t IzgaraKey3D(int gx, int gy, int gz) {
    uint64_t h1 = (uint64_t)(gx + 10000) * 73856093;
    uint64_t h2 = (uint64_t)(gy + 10000) * 19349663;
    uint64_t h3 = (uint64_t)(gz + 10000) * 83492791;
    return h1 ^ h2 ^ h3;
}

void IzgarayaEkle3D(int dalIdx) {
    const Vector3& p = tumDallar[dalIdx].p2;
    int gx = (int)(p.x / GRID_BOYUTU);
    int gy = (int)(p.y / GRID_BOYUTU);
    int gz = (int)(p.z / GRID_BOYUTU);
    izgara3D[IzgaraKey3D(gx, gy, gz)].push_back(dalIdx);
}

bool CakisiyorMu3D(const Vector3& p, float dinamikMesafeKare) {
    int gx = (int)(p.x / GRID_BOYUTU);
    int gy = (int)(p.y / GRID_BOYUTU);
    int gz = (int)(p.z / GRID_BOYUTU);

    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            for (int k = -1; k <= 1; ++k) {
                uint64_t key = IzgaraKey3D(gx + i, gy + j, gz + k);
                auto it = izgara3D.find(key);
                if (it != izgara3D.end()) {
                    for (int idx : it->second) {
                        Vector3 dp = Sub(p, tumDallar[idx].p2);
                        if (LengthSq(dp) < dinamikMesafeKare) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void CizCizgi(int x0, int y0, int x1, int y1, uint32_t renk) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if ((unsigned int)x0 < (unsigned int)GENISLIK && (unsigned int)y0 < (unsigned int)YUKSEKLIK) {
            pixels[y0 * GENISLIK + x0] = renk;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void CizDugumNoktasi(int cx, int cy, int r, uint32_t renk) {
    int r2 = r * r;
    for (int dy = -r; dy <= r; ++dy) {
        int py = cy + dy;
        if ((unsigned int)py >= (unsigned int)YUKSEKLIK) continue;
        for (int dx = -r; dx <= r; ++dx) {
            int px = cx + dx;
            if ((unsigned int)px >= (unsigned int)GENISLIK) continue;
            if (dx * dx + dy * dy <= r2) {
                pixels[py * GENISLIK + px] = renk;
            }
        }
    }
}

void EkranSifirla() {
    std::fill(pixels, pixels + (GENISLIK * YUKSEKLIK), 0x0A0A0C);
}

bool Dünya2Ekran3D(const Vector3& w, int& sx, int& sy) {
    Vector3 t = Sub(w, camPos);

    float cosY = std::cos(camYaw);
    float sinY = std::sin(camYaw);
    float x1 = t.x * cosY - t.z * sinY;
    float z1 = t.x * sinY + t.z * cosY;
    float y1 = t.y;

    float cosP = std::cos(camPitch);
    float sinP = std::sin(camPitch);
    float y2 = y1 * cosP - z1 * sinP;
    float z2 = y1 * sinP + z1 * cosP;
    float x2 = x1;

    if (z2 < 0.2f) return false;

    float fov = 600.0f;
    sx = (int)((x2 * fov) / z2 + GENISLIK / 2.0f);
    sy = (int)((y2 * fov) / z2 + YUKSEKLIK / 2.0f);

    return (sx >= 0 && sx < GENISLIK && sy >= 0 && sy < YUKSEKLIK);
}

void KameraKlavyeKontrolu() {
    float moveSpeed = 0.8f;

    float cosY = std::cos(camYaw);
    float sinY = std::sin(camYaw);
    float cosP = std::cos(camPitch);
    float sinP = std::sin(camPitch);

    Vector3 fwd = { sinY * cosP, sinP, cosY * cosP };
    Vector3 rgt = { cosY, 0.0f, -sinY };

    if (GetAsyncKeyState('W') & 0x8000) camPos = Add(camPos, Scale(fwd, moveSpeed));
    if (GetAsyncKeyState('S') & 0x8000) camPos = Sub(camPos, Scale(fwd, moveSpeed));
    if (GetAsyncKeyState('A') & 0x8000) camPos = Sub(camPos, Scale(rgt, moveSpeed));
    if (GetAsyncKeyState('D') & 0x8000) camPos = Add(camPos, Scale(rgt, moveSpeed));
    if (GetAsyncKeyState('E') & 0x8000) camPos.y -= moveSpeed;
    if (GetAsyncKeyState('Q') & 0x8000) camPos.y += moveSpeed;
}

Vector3 SapmaEkle3D(const Vector3& dir, float aciMiktari) {
    Vector3 arbitrary = (std::abs(dir.z) < 0.9f) ? Vector3{ 0, 0, 1 } : Vector3{ 1, 0, 0 };
    Vector3 right = Normalize(Cross(dir, arbitrary));
    Vector3 up = Cross(right, dir);

    float randAngle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
    Vector3 randDir = Add(Scale(right, std::cos(randAngle)), Scale(up, std::sin(randAngle)));

    Vector3 newDir = Add(Scale(dir, std::cos(aciMiktari)), Scale(randDir, std::sin(aciMiktari)));
    return Normalize(newDir);
}

void TurHesapla3D() {
    if (aktifDalIndeksleri.empty()) return;

    mevcutTur++;
    std::vector<int> yeniAktifler;
    yeniAktifler.reserve(aktifDalIndeksleri.size() * 2);
    int buTurUretilen = 0;

    float scaleFactor = (mevcutTur % kisalmaAraligi == 0) ? kisalmaOrani : 1.0f;

    for (int idx : aktifDalIndeksleri) {
        Dal3D ata = tumDallar[idx];
        float zar = (float)rand() / RAND_MAX;

        if (zar < dagilmaOrani) {
            for (int i = 0; i < 2; ++i) {
                Vector3 yDir = SapmaEkle3D(ata.dir, 0.45f);
                float yUzunluk = ata.uzunluk * scaleFactor;
                Vector3 p2 = Add(ata.p2, Scale(yDir, yUzunluk));

                float dinamikMesafe = yUzunluk * 0.35f;
                if (!CakisiyorMu3D(p2, dinamikMesafe * dinamikMesafe)) {
                    Dal3D yeniDal = { ata.p2, p2, yDir, yUzunluk, mevcutTur, idx };
                    tumDallar.push_back(yeniDal);
                    int yeniIdx = (int)tumDallar.size() - 1;
                    IzgarayaEkle3D(yeniIdx);
                    yeniAktifler.push_back(yeniIdx);
                    buTurUretilen++;
                }
            }
        }
        else {
            Vector3 yDir = SapmaEkle3D(ata.dir, 0.1f);
            float yUzunluk = ata.uzunluk * scaleFactor;
            Vector3 p2 = Add(ata.p2, Scale(yDir, yUzunluk));

            float dinamikMesafe = yUzunluk * 0.35f;
            if (!CakisiyorMu3D(p2, dinamikMesafe * dinamikMesafe)) {
                Dal3D yeniDal = { ata.p2, p2, yDir, yUzunluk, mevcutTur, idx };
                tumDallar.push_back(yeniDal);
                int yeniIdx = (int)tumDallar.size() - 1;
                IzgarayaEkle3D(yeniIdx);
                yeniAktifler.push_back(yeniIdx);
                buTurUretilen++;
            }
        }
    }
    aktifDalIndeksleri = std::move(yeniAktifler);
    sonUretilenDal = buTurUretilen;
}

void AgaciSifirla3D() {
    tumDallar.clear();
    aktifDalIndeksleri.clear();
    izgara3D.clear();

    tumDallar.reserve(1000000);
    mevcutTur = 1;
    sonUretilenDal = 2;
    secilenDalIdx = -1;

    Dal3D kokYukari = { {0, 0, 0}, {0, -kokUzunlugu, 0}, {0, -1, 0}, kokUzunlugu, 1, -1 };
    Dal3D kokAsagi = { {0, 0, 0}, {0,  kokUzunlugu, 0}, {0,  1, 0}, kokUzunlugu, 1, -1 };

    tumDallar.push_back(kokYukari);
    tumDallar.push_back(kokAsagi);

    IzgarayaEkle3D(0);
    IzgarayaEkle3D(1);

    aktifDalIndeksleri.push_back(0);
    aktifDalIndeksleri.push_back(1);

    camPos = { 0.0f, 0.0f, -45.0f };
    camYaw = 0.0f;
    camPitch = 0.0f;
}

void Render3D() {
    EkranSifirla();

    std::vector<bool> yolAgi;
    if (secilenDalIdx != -1) {
        yolAgi.resize(tumDallar.size(), false);
        int cur = secilenDalIdx;
        while (cur != -1) {
            yolAgi[cur] = true;
            cur = tumDallar[cur].ebeveyn_idx;
        }
    }

    size_t sz = tumDallar.size();
    for (size_t i = 0; i < sz; ++i) {
        const Dal3D& d = tumDallar[i];
        int sx1, sy1, sx2, sy2;

        bool vis1 = Dünya2Ekran3D(d.p1, sx1, sy1);
        bool vis2 = Dünya2Ekran3D(d.p2, sx2, sy2);

        if (vis1 || vis2) {
            uint32_t renk;
            if (secilenDalIdx != -1) {
                renk = yolAgi[i] ? 0x00FF80 : 0x222222;
            }
            else {
                renk = GetDalRengi(d.tur, renkPaleti);
            }

            CizCizgi(sx1, sy1, sx2, sy2, renk);

            if (dugumleriGoster && vis2) {
                uint32_t dugumRengi = (secilenDalIdx != -1 && yolAgi[i]) ? 0xFFFFFF : 0x00FFFF;
                CizDugumNoktasi(sx2, sy2, 1, dugumRengi);
            }
        }
    }
}

void CizArayuz(HDC hdc) {
    SetBkMode(hdc, TRANSPARENT);

    HFONT hFontBold = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    HFONT hFontNormal = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontBold);

    if (ayarlarAcik) {
        HBRUSH hBrush = CreateSolidBrush(RGB(18, 22, 30));
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 200, 255));
        SelectObject(hdc, hBrush);
        SelectObject(hdc, hPen);
        Rectangle(hdc, 20, 20, 500, 310);
        DeleteObject(hBrush);
        DeleteObject(hPen);

        SetTextColor(hdc, RGB(0, 255, 200));
        TextOutA(hdc, 35, 30, "=== KAOS MOTORU AYARLAR SEKMESI ===", 35);

        SelectObject(hdc, hFontNormal);
        SetTextColor(hdc, RGB(220, 220, 220));

        const char* paletIsimleri[] = { "1: Spektrum", "2: Okyanus/Cyan", "3: Ates/Amber", "4: Orman Yesili", "5: Neon/Mor" };
        std::string strPalet = "Dal Renk Paleti  : " + std::string(paletIsimleri[renkPaleti]);
        std::string strDugum = "Kirilma Noktalari: " + std::string(dugumleriGoster ? "[ ACIK ] (N)" : "[ KAPALI ] (N)");
        std::string strDagilma = "Dagilma Ihtimali : %" + std::to_string((int)(dagilmaOrani * 100)) + " (O / L)";
        std::string strAralik = "KisaliK Sikligi  : Her " + std::to_string(kisalmaAraligi) + " Turda Bir (Yukari/Asagi)";

        std::ostringstream ssKok;
        ssKok << std::fixed << std::setprecision(1) << kokUzunlugu;

        std::string strOran = "KisaliK CarpanI  : %" + std::to_string((int)(kisalmaOrani * 100)) + " (Sol/Sag)";
        std::string strKok = "Kok Dal Uzunlugu : " + ssKok.str() + " Birim (U/J)";

        TextOutA(hdc, 35, 60, strPalet.c_str(), (int)strPalet.length());
        TextOutA(hdc, 35, 90, strDugum.c_str(), (int)strDugum.length());
        TextOutA(hdc, 35, 120, strDagilma.c_str(), (int)strDagilma.length());
        TextOutA(hdc, 35, 150, strAralik.c_str(), (int)strAralik.length());
        TextOutA(hdc, 35, 180, strOran.c_str(), (int)strOran.length());
        TextOutA(hdc, 35, 210, strKok.c_str(), (int)strKok.length());

        SetTextColor(hdc, RGB(150, 150, 150));
        TextOutA(hdc, 35, 260, "[TAB]: Menuyu Gizle | [R]: Agaci Sifirla", 39);
    }
    else {
        SetTextColor(hdc, RGB(0, 200, 255));
        TextOutA(hdc, 20, 20, "[TAB]: Ayarlar Menusunu Ac", 27);
    }

    SelectObject(hdc, hFontBold);

    std::string strSonUretilen = "Son Uretilen Dal: " + std::to_string(sonUretilenDal);
    std::string strTur = "Tur: " + std::to_string(mevcutTur);
    std::string strAktif = "Ucu Acik Dal: " + std::to_string(aktifDalIndeksleri.size());

    RECT rectSonUretilen = { GENISLIK - 400, YUKSEKLIK - 92, GENISLIK - 20, YUKSEKLIK - 65 };
    RECT rectTur = { GENISLIK - 400, YUKSEKLIK - 65, GENISLIK - 20, YUKSEKLIK - 38 };
    RECT rectAktif = { GENISLIK - 400, YUKSEKLIK - 38, GENISLIK - 20, YUKSEKLIK - 10 };

    SetTextColor(hdc, RGB(200, 150, 255));
    DrawTextA(hdc, strSonUretilen.c_str(), -1, &rectSonUretilen, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

    SetTextColor(hdc, RGB(0, 255, 200));
    DrawTextA(hdc, strTur.c_str(), -1, &rectTur, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

    SetTextColor(hdc, RGB(255, 200, 0));
    DrawTextA(hdc, strAktif.c_str(), -1, &rectAktif, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFontBold);
    DeleteObject(hFontNormal);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN: {
        draggingRot = true;
        lastMousePos.x = LOWORD(lParam);
        lastMousePos.y = HIWORD(lParam);
        SetCapture(hwnd);
        break;
    }
    case WM_LBUTTONUP:
        draggingRot = false;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE: {
        if (draggingRot) {
            int mouseX = LOWORD(lParam);
            int mouseY = HIWORD(lParam);
            int dx = mouseX - lastMousePos.x;
            int dy = mouseY - lastMousePos.y;

            camYaw += dx * 0.003f;
            camPitch += dy * 0.003f;
            camPitch = (std::max)(-M_PI / 2.2f, (std::min)((float)M_PI / 2.2f, camPitch));

            lastMousePos.x = mouseX;
            lastMousePos.y = mouseY;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        float step = 2.5f;

        float cosY = std::cos(camYaw);
        float sinY = std::sin(camYaw);
        float cosP = std::cos(camPitch);
        float sinP = std::sin(camPitch);

        Vector3 fwd = { sinY * cosP, sinP, cosY * cosP };

        if (zDelta > 0) camPos = Add(camPos, Scale(fwd, step));
        else camPos = Sub(camPos, Scale(fwd, step));

        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    case WM_KEYDOWN:
        if (wParam == VK_TAB) {
            ayarlarAcik = !ayarlarAcik;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam >= '1' && wParam <= '5') {
            renkPaleti = (int)(wParam - '1');
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == 'N') {
            dugumleriGoster = !dugumleriGoster;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == 'O') {
            dagilmaOrani = (std::min)(1.0f, dagilmaOrani + 0.05f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == 'L') {
            dagilmaOrani = (std::max)(0.05f, dagilmaOrani - 0.05f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_UP) {
            kisalmaAraligi = (std::min)(50, kisalmaAraligi + 1);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_DOWN) {
            kisalmaAraligi = (std::max)(1, kisalmaAraligi - 1);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_RIGHT) {
            kisalmaOrani = (std::min)(1.0f, kisalmaOrani + 0.05f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_LEFT) {
            kisalmaOrani = (std::max)(0.1f, kisalmaOrani - 0.05f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == 'U') {
            kokUzunlugu += 1.0f;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == 'J') {
            kokUzunlugu = (std::max)(1.0f, kokUzunlugu - 1.0f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_SPACE) {
            TurHesapla3D();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VK_INSERT) {
            otomatikOynat = !otomatikOynat;
        }
        else if (wParam == 'R') {
            AgaciSifirla3D();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == 'C') {
            secilenDalIdx = -1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_TIMER:
        if (otomatikOynat) {
            TurHesapla3D();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    srand((unsigned int)time(NULL));

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "KaosAgaci3DClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA("KaosAgaci3DClass", "3D Kaos Motoru",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        GENISLIK, YUKSEKLIK, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    SetTimer(hwnd, 1, 30, NULL);

    AgaciSifirla3D();

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = GENISLIK;
    bmi.bmiHeader.biHeight = -YUKSEKLIK;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(hwnd);
    MSG msg;

    while (true) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto cikis;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        KameraKlavyeKontrolu();

        Render3D();

        StretchDIBits(hdc, 0, 0, GENISLIK, YUKSEKLIK,
            0, 0, GENISLIK, YUKSEKLIK,
            pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

        CizArayuz(hdc);

        std::string baslik = "3D Kaos Motoru | [TAB]: Menu | [O/L]: Dagilma %" + std::to_string((int)(dagilmaOrani * 100));
        SetWindowTextA(hwnd, baslik.c_str());
    }

cikis:
    ReleaseDC(hwnd, hdc);
    return 0;
}
