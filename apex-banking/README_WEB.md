# APEX Banking - Web Dashboard


## Co gi moi?

- Backend van la C++ va van dung cac class san co: `Bank`, `Account`, `Savings`, `Checking`, `CurrencyRegistry`, `Transaction`.
- Frontend la HTML/CSS/JavaScript thuan, chay truc tiep tren trinh duyet.
- Server HTTP duoc viet bang C++ socket va chay tai `http://localhost:8080`.
- Ho tro Windows MinGW/Dev-C++ va Linux/macOS. Tren Windows server dung WinSock2.

## File duoc them/sua

```txt
include/Bank.h          # them Bank::allAccounts()
include/Currency.h      # them CurrencyRegistry::allRates()
src/BankWebAccess.cpp   # cai dat getter cho danh sach account
src/web_main.cpp        # C++ web server + trang dashboard
Makefile                # them target web/run-web, link ws2_32 tren Windows
CMakeLists.txt          # build apex va apex_web
```

## Cach copy vao project goc

Copy **noi dung ben trong** thu muc `apex_web_patch/` vao thu muc `apex-banking/` cua project goc.

Dung:

```txt
apex-banking/
├── include/
├── src/
├── Makefile
├── CMakeLists.txt
└── README_WEB.md
```

Sai:

```txt
apex-banking/
└── apex_web_patch/
```

## Build bang Makefile

```bash
make clean
make web
make run-web
```

Sau do mo trinh duyet:

```txt
http://localhost:8080
```

Neu dang dung Windows/MinGW va gap loi lien quan socket, Makefile ban nay da tu dong them `-lws2_32`. Loi `arpa/inet.h: No such file or directory` da duoc sua bang WinSock2.

## Build bang CMake

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
./apex_web
```

Tren Windows, file chay co the la:

```bash
./apex_web.exe
```

## Ghi chu

- Mac dinh server chay port 8080.
- Co the doi port bang cach truyen tham so:

```bash
./apex_web 9090
```

roi mo:

```txt
http://localhost:9090
```
